// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////////////

#include "tink/subtle/aes_eax_boringssl.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/base/config.h"
#include "absl/base/nullability.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openssl/err.h"
#include "openssl/evp.h"
#include "tink/aead.h"
#include "tink/internal/aes_util.h"
#include "tink/internal/call_with_core_dump_protection.h"
#include "tink/internal/dfsan_forwarders.h"
#include "tink/internal/fips_utils.h"
#include "tink/internal/secret_buffer.h"
#include "tink/internal/util.h"
#include "tink/subtle/random.h"
#include "tink/subtle/subtle_util.h"
#include "tink/util/errors.h"
#include "tink/util/secret_data.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"

namespace crypto {
namespace tink {
namespace subtle {

namespace {

// Loads and stores 8 bytes. The endianness of the two routines
// does not matter, as long as the two routines use the same order.
uint64_t Load64(const uint8_t src[8]) {
  uint64_t res;
  std::memcpy(&res, src, 8);
  return res;
}

void Store64(uint64_t val, uint8_t dst[8]) { std::memcpy(dst, &val, 8); }

uint64_t ByteSwap(uint64_t val) {
  val = ((val & 0xffffffff00000000) >> 32) | ((val & 0x00000000ffffffff) << 32);
  val = ((val & 0xffff0000ffff0000) >> 16) | ((val & 0x0000ffff0000ffff) << 16);
  val = ((val & 0xff00ff00ff00ff00) >> 8) | ((val & 0x00ff00ff00ff00ff) << 8);
  return val;
}

uint64_t BigEndianLoad64(const uint8_t src[8]) {
#if defined(ABSL_IS_LITTLE_ENDIAN)
  return ByteSwap(Load64(src));
#elif defined(ABSL_IS_BIG_ENDIAN)
  return Load64(src);
#else
#error Unknown endianness
#endif
}

void BigEndianStore64(uint64_t val, uint8_t dst[8]) {
#if defined(ABSL_IS_LITTLE_ENDIAN)
  return Store64(ByteSwap(val), dst);
#elif defined(ABSL_IS_BIG_ENDIAN)
  return Store64(val, dst);
#else
#error Unknown endianness
#endif
}

absl::StatusOr<util::SecretUniquePtr<AES_KEY>> InitAesKey(
    const SecretData& key) {
  auto aeskey = util::MakeSecretUniquePtr<AES_KEY>();
  int status = AES_set_encrypt_key(key.data(), key.size() * 8, aeskey.get());
  // status != 0 happens if key_value is invalid.
  if (status != 0) {
    return absl::Status(absl::StatusCode::kInvalidArgument,
                        "Invalid key value");
  }
  return std::move(aeskey);
}

}  // namespace

void AesEaxBoringSsl::XorBlock(const uint8_t x[kBlockSize], Block* y) {
  uint64_t r0 = Load64(x) ^ Load64(y->data());
  uint64_t r1 = Load64(x + 8) ^ Load64(y->data() + 8);
  Store64(r0, y->data());
  Store64(r1, y->data() + 8);
}

void AesEaxBoringSsl::MultiplyByX(const uint8_t in[kBlockSize],
                                  uint8_t out[kBlockSize]) {
  uint64_t in_high = BigEndianLoad64(in);
  uint64_t in_low = BigEndianLoad64(in + 8);
  uint64_t out_high = (in_high << 1) ^ (in_low >> 63);
  // If the most significant bit is set then the result has to
  // be reduced by x^128 + x^7 + x^2 + x + 1.
  // The representation of x^7 + x^2 + x + 1 is 0x87.
  uint64_t out_low = (in_low << 1) ^ (0x87 & -(in_high >> 63));
  BigEndianStore64(out_high, out);
  BigEndianStore64(out_low, out + 8);
}

bool AesEaxBoringSsl::EqualBlocks(const uint8_t x[kBlockSize],
                                  const uint8_t y[kBlockSize]) {
  uint64_t res = Load64(x) ^ Load64(y);
  res |= Load64(x + 8) ^ Load64(y + 8);
  return res == 0;
}

bool AesEaxBoringSsl::IsValidKeySize(size_t key_size_in_bytes) {
  return key_size_in_bytes == 16 || key_size_in_bytes == 32;
}

bool AesEaxBoringSsl::IsValidNonceSize(size_t nonce_size_in_bytes) {
  return nonce_size_in_bytes == 12 || nonce_size_in_bytes == 16;
}

SecretData AesEaxBoringSsl::ComputeB() const {
  internal::SecretBuffer block(kBlockSize, 0);
  EncryptBlock(&block);
  MultiplyByX(block.data(), block.data());
  return util::internal::AsSecretData(std::move(block));
}

SecretData AesEaxBoringSsl::ComputeP() const {
  internal::SecretBuffer rv(kBlockSize, 0);
  MultiplyByX(B_.data(), rv.data());
  return util::internal::AsSecretData(std::move(rv));
}

absl::StatusOr<std::unique_ptr<Aead>> AesEaxBoringSsl::New(
    const SecretData& key, size_t nonce_size_in_bytes) {
  auto status = internal::CheckFipsCompatibility<AesEaxBoringSsl>();
  if (!status.ok()) return status;

  if (!IsValidKeySize(key.size())) {
    return absl::Status(absl::StatusCode::kInvalidArgument, "Invalid key size");
  }
  if (!IsValidNonceSize(nonce_size_in_bytes)) {
    return absl::Status(absl::StatusCode::kInvalidArgument,
                        "Invalid nonce size");
  }
  return internal::CallWithCoreDumpProtection(
      [&]() -> absl::StatusOr<std::unique_ptr<Aead>> {
        auto aeskey_or = InitAesKey(key);
        if (!aeskey_or.ok()) {
          return aeskey_or.status();
        }
        return absl::WrapUnique(new AesEaxBoringSsl(
            std::move(aeskey_or).value(), nonce_size_in_bytes));
      });
}

AesEaxBoringSsl::Block AesEaxBoringSsl::Pad(
    absl::Span<const uint8_t> data) const {
  // TODO(bleichen): What are we using in tink to encode assertions?
  // The caller must ensure that data is no longer than a block.
  // CHECK(0 <= len && len <= kBlockSize) << "Invalid data size";
  Block padded_block;
  padded_block.fill(0);
  absl::c_copy(data, padded_block.begin());
  if (data.size() == kBlockSize) {
    XorBlock(B_.data(), &padded_block);
  } else {
    padded_block[data.size()] = 0x80;
    XorBlock(P_.data(), &padded_block);
  }
  return padded_block;
}

void AesEaxBoringSsl::EncryptBlock(
    internal::SecretBuffer* /*absl_nonnull - not yet supported*/ block) const {
  AES_encrypt(block->data(), block->data(), aeskey_.get());
}

void AesEaxBoringSsl::EncryptBlock(Block* /*absl_nonnull - not yet supported*/ block) const {
  AES_encrypt(block->data(), block->data(), aeskey_.get());
}

AesEaxBoringSsl::Block AesEaxBoringSsl::Omac(absl::string_view blob,
                                             int tag) const {
  return Omac(absl::MakeSpan(reinterpret_cast<const uint8_t*>(blob.data()),
                             blob.size()),
              tag);
}

AesEaxBoringSsl::Block AesEaxBoringSsl::Omac(absl::Span<const uint8_t> data,
                                             int tag) const {
  Block mac;
  mac.fill(0);
  mac[15] = tag;
  if (data.empty()) {
    XorBlock(B_.data(), &mac);
    EncryptBlock(&mac);
    return mac;
  }
  EncryptBlock(&mac);
  int idx = 0;
  while (data.size() - idx > kBlockSize) {
    XorBlock(&data[idx], &mac);
    EncryptBlock(&mac);
    idx += kBlockSize;
  }
  const Block padded_block = Pad(absl::MakeSpan(data).subspan(idx));
  XorBlock(padded_block.data(), &mac);
  EncryptBlock(&mac);
  return mac;
}

absl::Status AesEaxBoringSsl::CtrCrypt(const Block& N, absl::string_view in,
                                       absl::Span<char> out) const {
  // Make a copy of N, since BoringSsl changes ctr.
  uint8_t ctr[kBlockSize];
  std::copy_n(N.begin(), kBlockSize, ctr);
  return internal::AesCtr128Crypt(in, ctr, aeskey_.get(), out);
}

absl::StatusOr<std::string> AesEaxBoringSsl::Encrypt(
    absl::string_view plaintext, absl::string_view associated_data) const {
  // BoringSSL expects a non-null pointer for plaintext and associated_data,
  // regardless of whether the size is 0.
  plaintext = internal::EnsureStringNonNull(plaintext);
  associated_data = internal::EnsureStringNonNull(associated_data);

  size_t ciphertext_size = plaintext.size() + nonce_size_ + kTagSize;
  std::string ciphertext;
  ResizeStringUninitialized(&ciphertext, ciphertext_size);
  return internal::CallWithCoreDumpProtection(
      [&]() -> absl::StatusOr<std::string> {
        // The ciphertext region is allowed to leak: this never fails and
        // the ciphertext can afterwards be given to the adversary.
        crypto::tink::internal::ScopedAssumeRegionCoreDumpSafe scope_object(
            ciphertext.data(), ciphertext.size());
        const std::string nonce = Random::GetRandomBytes(nonce_size_);
        const Block N = Omac(nonce, 0);
        const Block H = Omac(associated_data, 1);
        uint8_t* ct_start =
            reinterpret_cast<uint8_t*>(&ciphertext[nonce_size_]);
        absl::Status res = CtrCrypt(
            N, plaintext, absl::MakeSpan(ciphertext).subspan(nonce_size_));
        if (!res.ok()) {
          return res;
        }
        Block mac = Omac(absl::MakeSpan(ct_start, plaintext.size()), 2);
        XorBlock(N.data(), &mac);
        XorBlock(H.data(), &mac);
        absl::c_copy(nonce, ciphertext.begin());
        std::copy_n(mac.begin(), kTagSize,
                    &ciphertext[ciphertext_size - kTagSize]);
        // Declassify the ciphertext: this is now safe to give to the adversary.
        // (Note: we currently do not propagate labels of the associated data).
        crypto::tink::internal::DfsanClearLabel(ciphertext.data(),
                                                ciphertext_size);
        return ciphertext;
      });
}

absl::StatusOr<std::string> AesEaxBoringSsl::Decrypt(
    absl::string_view ciphertext, absl::string_view associated_data) const {
  // BoringSSL expects a non-null pointer for associated_data,
  // regardless of whether the size is 0.
  associated_data = internal::EnsureStringNonNull(associated_data);

  size_t ct_size = ciphertext.size();
  if (ct_size < nonce_size_ + kTagSize) {
    return absl::Status(absl::StatusCode::kInvalidArgument,
                        "Ciphertext too short");
  }
  size_t out_size = ct_size - kTagSize - nonce_size_;
  absl::string_view nonce = ciphertext.substr(0, nonce_size_);
  absl::string_view encrypted = ciphertext.substr(nonce_size_, out_size);
  absl::string_view tag = ciphertext.substr(ct_size - kTagSize, kTagSize);
  return internal::CallWithCoreDumpProtection(
      [&]() -> absl::StatusOr<std::string> {
        const Block N = Omac(nonce, 0);
        const Block H = Omac(associated_data, 1);
        Block mac = Omac(encrypted, 2);
        XorBlock(N.data(), &mac);
        XorBlock(H.data(), &mac);
        const uint8_t* sig = reinterpret_cast<const uint8_t*>(tag.data());
        if (!EqualBlocks(mac.data(), sig)) {
          return absl::Status(absl::StatusCode::kInvalidArgument,
                              "Tag mismatch");
        }
        std::string plaintext;
        ResizeStringUninitialized(&plaintext, out_size);
        // The plaintext region is allowed to leak. In successful decryptions,
        // the adversary can already get the plaintext via core dumps (since
        // the API specifies that the plaintext is in a std::string, so this is
        // the users responsibility). Hence, this gives adversaries access to
        // data which is stored *during* the computation, and data which would
        // be erased because the tag is wrong. Since EAX is a counter mode, this
        // means that the adversary can potentially obtain key streams for IVs
        // for which he does either not know a valid tag (which seems useless if
        // he didn't see a valid ciphertext) or without querying the actual
        // ciphertext (which does not seem useful). Hence, we declare this to be
        // sufficiently safe at the moment.
        char* plaintext_start = &plaintext[0];
        crypto::tink::internal::ScopedAssumeRegionCoreDumpSafe scope_object(
            plaintext_start, out_size);
        absl::Status res = CtrCrypt(N, encrypted, absl::MakeSpan(plaintext));
        if (!res.ok()) {
          return res;
        }
        // Declassify the plaintext: this is now safe to give to the adversary
        // (since the API specifies that the plaintext is in a std::string which
        // can leak so the user is responsible for this).
        crypto::tink::internal::DfsanClearLabel(plaintext_start, out_size);
        return plaintext;
      });
}

}  // namespace subtle
}  // namespace tink
}  // namespace crypto
