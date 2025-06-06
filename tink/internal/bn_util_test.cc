// Copyright 2021 Google LLC
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
#include "tink/internal/bn_util.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "openssl/bn.h"
#include "tink/internal/ssl_unique_ptr.h"
#include "tink/util/secret_data.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"
#include "tink/util/test_matchers.h"
#include "tink/util/test_util.h"

namespace crypto {
namespace tink {
namespace internal {
namespace {

using ::crypto::tink::test::IsOk;
using ::testing::Not;

absl::StatusOr<internal::SslUniquePtr<BIGNUM>> HexToBignum(
    absl::string_view bn_hex) {
  BIGNUM* bn = nullptr;
  BN_hex2bn(&bn, bn_hex.data());
  return internal::SslUniquePtr<BIGNUM>(bn);
}

TEST(BnUtil, StringToBignum) {
  std::vector<std::string> bn_str = {"0000000000000000", "0000000000000001",
                                     "1000000000000000", "ffffffffffffffff",
                                     "0fffffffffffffff", "00ffffffffffffff"};
  for (const std::string& s : bn_str) {
    const std::string bn_bytes = test::HexDecodeOrDie(s);
    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> bn =
        StringToBignum(bn_bytes);
    ASSERT_THAT(bn, IsOk());

    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> expected_bn = HexToBignum(s);
    ASSERT_THAT(expected_bn, IsOk());
    EXPECT_EQ(BN_cmp(expected_bn->get(), bn->get()), 0);
  }
}

TEST(StringToBignum, IgnoresLeadingZeros) {
  std::string encoded = test::HexDecodeOrDie("0102");
  std::string encoded_with_leading_zeros = test::HexDecodeOrDie("0000000102");

  absl::StatusOr<internal::SslUniquePtr<BIGNUM>> num = StringToBignum(encoded);
  ASSERT_THAT(num, IsOk());

  absl::StatusOr<internal::SslUniquePtr<BIGNUM>> num2 =
      StringToBignum(encoded_with_leading_zeros);
  ASSERT_THAT(num2, IsOk());

  EXPECT_EQ(BN_cmp(num2->get(), num->get()), 0);
}

TEST(BnUtil, BignumToString) {
  std::vector<std::string> bn_strs = {"0000000000000000", "0000000000000001",
                                      "1000000000000000", "ffffffffffffffff",
                                      "0fffffffffffffff", "00ffffffffffffff"};
  for (const std::string& s : bn_strs) {
    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> expected_bn = HexToBignum(s);
    ASSERT_THAT(expected_bn, IsOk());

    const std::string bn_bytes = test::HexDecodeOrDie(s);
    absl::StatusOr<std::string> result =
        BignumToString(expected_bn->get(), bn_bytes.size());
    ASSERT_THAT(result, IsOk());
    EXPECT_EQ(bn_bytes, *result);
  }
}

TEST(BignumToStringWithBNNumBytes, NoLeadingZeros) {
  {
    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> bn0 =
        StringToBignum(test::HexDecodeOrDie("000000"));
    ASSERT_THAT(bn0, IsOk());

    absl::StatusOr<std::string> encoded0 =
        internal::BignumToString(bn0->get(), BN_num_bytes(bn0->get()));
    ASSERT_THAT(encoded0, IsOk());
    EXPECT_EQ(*encoded0, test::HexDecodeOrDie(""));
  }

  {
    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> bn127 =
        StringToBignum(test::HexDecodeOrDie("00007F"));
    ASSERT_THAT(bn127, IsOk());

    absl::StatusOr<std::string> encoded127 =
        internal::BignumToString(bn127->get(), BN_num_bytes(bn127->get()));
    ASSERT_THAT(encoded127, IsOk());
    EXPECT_EQ(*encoded127, test::HexDecodeOrDie("7F"));
  }

  {
    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> bn128 =
        StringToBignum(test::HexDecodeOrDie("000080"));
    ASSERT_THAT(bn128, IsOk());

    absl::StatusOr<std::string> encoded128 =
        internal::BignumToString(bn128->get(), BN_num_bytes(bn128->get()));
    ASSERT_THAT(encoded128, IsOk());
    EXPECT_EQ(*encoded128, test::HexDecodeOrDie("80"));
  }

  {
    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> bn255 =
        StringToBignum(test::HexDecodeOrDie("0000FF"));
    ASSERT_THAT(bn255, IsOk());

    absl::StatusOr<std::string> encoded255 =
        internal::BignumToString(bn255->get(), BN_num_bytes(bn255->get()));
    ASSERT_THAT(encoded255, IsOk());
    EXPECT_EQ(*encoded255, test::HexDecodeOrDie("FF"));
  }

  {
    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> bn256 =
        StringToBignum(test::HexDecodeOrDie("000100"));
    ASSERT_THAT(bn256, IsOk());

    absl::StatusOr<std::string> encoded256 =
        internal::BignumToString(bn256->get(), BN_num_bytes(bn256->get()));
    ASSERT_THAT(encoded256, IsOk());
    EXPECT_EQ(*encoded256, test::HexDecodeOrDie("0100"));
  }
}


TEST(BignumToString, PadsWithLeadingZeros) {
  absl::StatusOr<internal::SslUniquePtr<BIGNUM>> num =
      StringToBignum(test::HexDecodeOrDie("0102"));
  ASSERT_THAT(num, IsOk());

  absl::StatusOr<std::string> encoded = BignumToString(num->get(), /*len=*/2);
  ASSERT_THAT(encoded, IsOk());
  EXPECT_EQ(*encoded, test::HexDecodeOrDie("0102"));

  absl::StatusOr<std::string> encodedWithPadding =
      BignumToString(num->get(), /*len=*/5);
  ASSERT_THAT(encodedWithPadding, IsOk());
  EXPECT_EQ(*encodedWithPadding, test::HexDecodeOrDie("0000000102"));

  // try to encode with a value for len that is too short.
  ASSERT_THAT(BignumToString(num->get(), /*len=*/1), Not(IsOk()));
}

TEST(BignumToString, RejectsNegativeNumbers) {
  // create a negative BIGNUM
  absl::StatusOr<internal::SslUniquePtr<BIGNUM>> number = HexToBignum("01");
  ASSERT_THAT(number, IsOk());
  BN_set_negative(number->get(), 1);
  // Check that number is negative
  ASSERT_EQ(CompareBignumWithWord(number->get(), /*word=*/0), -1);

  ASSERT_THAT(BignumToString(number->get(), /*len=*/2), Not(IsOk()));
}

TEST(BnUtil, BignumToSecretData) {
  std::vector<std::string> bn_strs = {"0000000000000000", "0000000000000001",
                                      "1000000000000000", "ffffffffffffffff",
                                      "0fffffffffffffff", "00ffffffffffffff"};
  for (const std::string& s : bn_strs) {
    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> expected_bn = HexToBignum(s);
    ASSERT_THAT(expected_bn, IsOk());

    const std::string bn_bytes = test::HexDecodeOrDie(s);
    absl::StatusOr<SecretData> result =
        BignumToSecretData(expected_bn->get(), bn_bytes.size());
    ASSERT_THAT(result, IsOk());
    EXPECT_EQ(absl::string_view(bn_bytes),
              util::SecretDataAsStringView(*result));
  }
}

TEST(BnUtil, BignumToBinaryPadded) {
  std::vector<std::string> bn_strs = {"0000000000000000", "0000000000000001",
                                      "1000000000000000", "ffffffffffffffff",
                                      "0fffffffffffffff", "00ffffffffffffff"};
  for (const std::string& s : bn_strs) {
    absl::StatusOr<internal::SslUniquePtr<BIGNUM>> expected_bn = HexToBignum(s);
    ASSERT_THAT(expected_bn, IsOk());

    const std::string bn_bytes = test::HexDecodeOrDie(s);
    std::vector<char> buffer;
    buffer.resize(bn_bytes.size());
    absl::Status res = BignumToBinaryPadded(
        absl::MakeSpan(buffer.data(), buffer.size()), expected_bn->get());
    ASSERT_THAT(res, IsOk());
    auto buffer_data = absl::string_view(buffer.data(), buffer.size());
    EXPECT_EQ(absl::string_view(bn_bytes), buffer_data);
  }
}

// Make sure that for every buffer size that is smaller than the actual BN as a
// string, we get an error.
TEST(BnUtil, BufferToSmall) {
  const std::string bn_str = "0fffffffffffffff";
  absl::StatusOr<internal::SslUniquePtr<BIGNUM>> expected_bn =
      HexToBignum(bn_str);
  ASSERT_THAT(expected_bn, IsOk());
  const std::string bn_bytes = test::HexDecodeOrDie(bn_str);
  for (size_t buffer_size = 1; buffer_size < bn_bytes.size(); buffer_size++) {
    {
      std::vector<char> buffer;
      buffer.resize(buffer_size);
      absl::Status result = BignumToBinaryPadded(
          absl::MakeSpan(buffer.data(), buffer.size()), expected_bn->get());
      EXPECT_THAT(result, Not(IsOk()));
    }
    {
      absl::StatusOr<std::string> result =
          BignumToString(expected_bn->get(), buffer_size);
      EXPECT_THAT(result, Not(IsOk()));
    }
    {
      absl::StatusOr<SecretData> result =
          BignumToSecretData(expected_bn->get(), buffer_size);
      EXPECT_THAT(result, Not(IsOk()));
    }
  }
}

TEST(BnUtil, CompareBignumWithWord) {
  internal::SslUniquePtr<BIGNUM> bn(BN_new());
  BN_set_word(bn.get(), /*value=*/0x0fffffffffffffffUL);
  EXPECT_EQ(CompareBignumWithWord(bn.get(), /*word=*/0x0fffffffffffffffL), 0);
  std::vector<BN_ULONG> smaller_words = {
      0x0000000000000000UL, 0x0000000000000001UL, 0x00ffffffffffffffUL};
  for (const auto& word : smaller_words) {
    EXPECT_GT(CompareBignumWithWord(bn.get(), word), 0)
        << absl::StrCat("With value: 0x", absl::Hex(word));
  }
  std::vector<BN_ULONG> larger_words = {0x1000000000000000UL,
                                        0xffffffffffffffffUL};
  for (const auto& word : larger_words) {
    EXPECT_LT(CompareBignumWithWord(bn.get(), word), 0)
        << absl::StrCat("With value: 0x", absl::Hex(word));
  }
}

TEST(BnUtil, InlineBigNum) {
  InlineBignum bn;
  BN_set_word(bn.get(), /*value=*/0x0fffffffffffffffUL);
  EXPECT_EQ(CompareBignumWithWord(bn.get(), /*word=*/0x0fffffffffffffffL), 0);
}

TEST(BnUtil, InlineBigNumRelease) {
  InlineBignum bn;
  BN_set_word(bn.get(), /*value=*/0x0fffffffffffffffUL);
  BIGNUM* bn_ptr = bn.get();
  bn.release();
  EXPECT_EQ(bn.get(), nullptr);
  EXPECT_EQ(CompareBignumWithWord(bn_ptr, /*word=*/0x0fffffffffffffffL), 0);
  BN_free(bn_ptr);
}

}  // namespace
}  // namespace internal
}  // namespace tink
}  // namespace crypto
