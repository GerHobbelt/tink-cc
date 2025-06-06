// Copyright 2017 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////

#include "tink/hybrid/ecies_aead_hkdf_private_key_manager.h"
#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "tink/aead/aead_key_templates.h"
#include "tink/aead/aes_ctr_hmac_aead_key_manager.h"
#include "tink/aead/aes_gcm_key_manager.h"
#include "tink/config/global_registry.h"
#include "tink/hybrid/ecies_aead_hkdf_hybrid_encrypt.h"
#include "tink/hybrid/ecies_aead_hkdf_public_key_manager.h"
#include "tink/hybrid/hybrid_config.h"
#include "tink/hybrid/hybrid_key_templates.h"
#include "tink/hybrid/internal/testing/ecies_aead_hkdf_test_vectors.h"
#include "tink/hybrid/internal/testing/hybrid_test_vectors.h"
#include "tink/hybrid_decrypt.h"
#include "tink/hybrid_encrypt.h"
#include "tink/key_status.h"
#include "tink/keyset_handle.h"
#include "tink/registry.h"
#include "tink/subtle/hybrid_test_util.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"
#include "tink/util/test_matchers.h"
#include "tink/util/test_util.h"
#include "proto/aes_eax.pb.h"
#include "proto/common.pb.h"
#include "proto/ecies_aead_hkdf.pb.h"
#include "proto/tink.pb.h"

namespace crypto {
namespace tink {

using ::crypto::tink::internal::HybridTestVector;
using ::crypto::tink::test::IsOk;
using ::crypto::tink::test::IsOkAndHolds;
using ::crypto::tink::test::StatusIs;
using ::google::crypto::tink::EciesAeadHkdfKeyFormat;
using ::google::crypto::tink::EciesAeadHkdfPrivateKey;
using ::google::crypto::tink::EciesAeadHkdfPublicKey;
using ::google::crypto::tink::EcPointFormat;
using ::google::crypto::tink::EllipticCurveType;
using ::google::crypto::tink::HashType;
using ::google::crypto::tink::KeyData;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Not;

namespace {

TEST(EciesAeadHkdfPrivateKeyManagerTest, Basics) {
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().get_version(), Eq(0));
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().key_material_type(),
              Eq(KeyData::ASYMMETRIC_PRIVATE));
  EXPECT_THAT(
      EciesAeadHkdfPrivateKeyManager().get_key_type(),
      Eq("type.googleapis.com/google.crypto.tink.EciesAeadHkdfPrivateKey"));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateEmptyKey) {
  EXPECT_THAT(
      EciesAeadHkdfPrivateKeyManager().ValidateKey(EciesAeadHkdfPrivateKey()),
      StatusIs(absl::StatusCode::kInvalidArgument));
}

EciesAeadHkdfKeyFormat CreateValidKeyFormat() {
  EciesAeadHkdfKeyFormat key_format;
  key_format.mutable_params()->set_ec_point_format(EcPointFormat::UNCOMPRESSED);
  auto dem_params = key_format.mutable_params()->mutable_dem_params();
  *(dem_params->mutable_aead_dem()) = AeadKeyTemplates::Aes128Gcm();
  auto kem_params = key_format.mutable_params()->mutable_kem_params();
  kem_params->set_curve_type(EllipticCurveType::NIST_P256);
  kem_params->set_hkdf_hash_type(HashType::SHA256);
  kem_params->set_hkdf_salt("");
  return key_format;
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyFormat) {
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKeyFormat(
                  CreateValidKeyFormat()),
              IsOk());
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyFormatNoPoint) {
  EciesAeadHkdfKeyFormat key_format = CreateValidKeyFormat();
  key_format.mutable_params()->set_ec_point_format(
      EcPointFormat::UNKNOWN_FORMAT);
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKeyFormat(key_format),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyFormatNoDem) {
  EciesAeadHkdfKeyFormat key_format = CreateValidKeyFormat();
  key_format.mutable_params()->mutable_dem_params()->clear_aead_dem();
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKeyFormat(key_format),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyFormatNoKemCurve) {
  EciesAeadHkdfKeyFormat key_format = CreateValidKeyFormat();
  key_format.mutable_params()->mutable_kem_params()->set_curve_type(
      EllipticCurveType::UNKNOWN_CURVE);
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKeyFormat(key_format),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyFormatNoKemHash) {
  EciesAeadHkdfKeyFormat key_format = CreateValidKeyFormat();
  key_format.mutable_params()->mutable_kem_params()->set_hkdf_hash_type(
      HashType::UNKNOWN_HASH);
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKeyFormat(key_format),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, CreateKey) {
  EciesAeadHkdfKeyFormat key_format = CreateValidKeyFormat();
  ASSERT_THAT(EciesAeadHkdfPrivateKeyManager().CreateKey(key_format).status(),
              IsOk());
  EciesAeadHkdfPrivateKey key =
      EciesAeadHkdfPrivateKeyManager().CreateKey(key_format).value();
  EXPECT_THAT(key.public_key().params().kem_params().curve_type(),
              Eq(key_format.params().kem_params().curve_type()));
  EXPECT_THAT(key.public_key().params().kem_params().hkdf_hash_type(),
              Eq(key_format.params().kem_params().hkdf_hash_type()));
  EXPECT_THAT(key.public_key().params().dem_params().aead_dem().type_url(),
              Eq(key_format.params().dem_params().aead_dem().type_url()));
  EXPECT_THAT(key.public_key().params().dem_params().aead_dem().value(),
              Eq(key_format.params().dem_params().aead_dem().value()));
  EXPECT_THAT(
      key.public_key().params().dem_params().aead_dem().output_prefix_type(),
      Eq(key_format.params().dem_params().aead_dem().output_prefix_type()));

  EXPECT_THAT(key.public_key().x(), Not(IsEmpty()));
  EXPECT_THAT(key.public_key().y(), Not(IsEmpty()));
  EXPECT_THAT(key.key_value(), Not(IsEmpty()));
}

EciesAeadHkdfPrivateKey CreateValidKey() {
  return EciesAeadHkdfPrivateKeyManager()
      .CreateKey(CreateValidKeyFormat())
      .value();
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyEmpty) {
  EXPECT_THAT(
      EciesAeadHkdfPrivateKeyManager().ValidateKey(EciesAeadHkdfPrivateKey()),
      StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKey) {
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKey(CreateValidKey()),
              IsOk());
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyWrongVersion) {
  EciesAeadHkdfPrivateKey key = CreateValidKey();
  key.set_version(1);
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKey(key),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyNoPoint) {
  EciesAeadHkdfPrivateKey key = CreateValidKey();
  key.mutable_public_key()->mutable_params()->set_ec_point_format(
      EcPointFormat::UNKNOWN_FORMAT);
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKey(key),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyNoDem) {
  EciesAeadHkdfPrivateKey key = CreateValidKey();
  key.mutable_public_key()
      ->mutable_params()
      ->mutable_dem_params()
      ->clear_aead_dem();
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKey(key),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyNoKemCurve) {
  EciesAeadHkdfPrivateKey key = CreateValidKey();
  key.mutable_public_key()
      ->mutable_params()
      ->mutable_kem_params()
      ->set_curve_type(EllipticCurveType::UNKNOWN_CURVE);
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKey(key),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, ValidateKeyNoKemHash) {
  EciesAeadHkdfPrivateKey key = CreateValidKey();
  key.mutable_public_key()
      ->mutable_params()
      ->mutable_kem_params()
      ->set_hkdf_hash_type(HashType::UNKNOWN_HASH);
  EXPECT_THAT(EciesAeadHkdfPrivateKeyManager().ValidateKey(key),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, GetPublicKey) {
  EciesAeadHkdfPrivateKey key = CreateValidKey();
  ASSERT_THAT(EciesAeadHkdfPrivateKeyManager().GetPublicKey(key).status(),
              IsOk());
  EciesAeadHkdfPublicKey public_key =
      EciesAeadHkdfPrivateKeyManager().GetPublicKey(key).value();
  EXPECT_THAT(public_key.params().kem_params().curve_type(),
              Eq(key.public_key().params().kem_params().curve_type()));
  EXPECT_THAT(public_key.params().kem_params().hkdf_hash_type(),
              Eq(key.public_key().params().kem_params().hkdf_hash_type()));
  EXPECT_THAT(public_key.params().dem_params().aead_dem().type_url(),
              Eq(key.public_key().params().dem_params().aead_dem().type_url()));
  EXPECT_THAT(public_key.params().dem_params().aead_dem().value(),
              Eq(key.public_key().params().dem_params().aead_dem().value()));
  EXPECT_THAT(public_key.params().dem_params().aead_dem().output_prefix_type(),
              Eq(key.public_key()
                     .params()
                     .dem_params()
                     .aead_dem()
                     .output_prefix_type()));

  EXPECT_THAT(public_key.x(), Not(IsEmpty()));
  EXPECT_THAT(public_key.y(), Not(IsEmpty()));
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, Create) {
  ASSERT_THAT(Registry::RegisterKeyTypeManager(
      absl::make_unique<AesGcmKeyManager>(), true), IsOk());

  EciesAeadHkdfPrivateKey private_key = CreateValidKey();
  EciesAeadHkdfPublicKey public_key =
      EciesAeadHkdfPrivateKeyManager().GetPublicKey(private_key).value();

  auto decrypt_or =
      EciesAeadHkdfPrivateKeyManager().GetPrimitive<HybridDecrypt>(private_key);
  ASSERT_THAT(decrypt_or, IsOk());
  auto encrypt_or = EciesAeadHkdfHybridEncrypt::New(public_key);
  ASSERT_THAT(encrypt_or, IsOk());

  ASSERT_THAT(HybridEncryptThenDecrypt(encrypt_or.value().get(),
                                       decrypt_or.value().get(), "some text",
                                       "some aad"),
              IsOk());
}

TEST(EciesAeadHkdfPrivateKeyManagerTest, CreateDifferentKey) {
  ASSERT_THAT(Registry::RegisterKeyTypeManager(
      absl::make_unique<AesGcmKeyManager>(), true), IsOk());

  EciesAeadHkdfPrivateKey private_key = CreateValidKey();
  // Note: we create a new private key in the next line.
  EciesAeadHkdfPublicKey public_key =
      EciesAeadHkdfPrivateKeyManager().GetPublicKey(CreateValidKey()).value();

  auto decrypt_or =
      EciesAeadHkdfPrivateKeyManager().GetPrimitive<HybridDecrypt>(private_key);
  ASSERT_THAT(decrypt_or, IsOk());
  auto encrypt_or = EciesAeadHkdfHybridEncrypt::New(public_key);
  ASSERT_THAT(encrypt_or, IsOk());

  ASSERT_THAT(HybridEncryptThenDecrypt(encrypt_or.value().get(),
                                       decrypt_or.value().get(), "some text",
                                       "some aad"),
              Not(IsOk()));
}

using EciesTestVectorTest = testing::TestWithParam<HybridTestVector>;

TEST_P(EciesTestVectorTest, DecryptWorks) {
  ASSERT_THAT(HybridConfig::Register(), IsOk());
  const HybridTestVector& param = GetParam();
  absl::StatusOr<KeysetHandle> handle =
      KeysetHandleBuilder()
          .AddEntry(KeysetHandleBuilder::Entry::CreateFromKey(
              param.hybrid_private_key, KeyStatus::kEnabled,
              /*is_primary=*/true))
          .Build();
  ASSERT_THAT(handle, IsOk());
  absl::StatusOr<std::unique_ptr<HybridDecrypt>> decrypter =
      handle->GetPrimitive<HybridDecrypt>(ConfigGlobalRegistry());
  ASSERT_THAT(decrypter, IsOk());
  EXPECT_THAT((*decrypter)->Decrypt(param.ciphertext, param.context_info),
              IsOkAndHolds(Eq(param.plaintext)));
}

TEST_P(EciesTestVectorTest, DecryptDifferentContextInfoFails) {
  ASSERT_THAT(HybridConfig::Register(), IsOk());
  const HybridTestVector& param = GetParam();
  absl::StatusOr<KeysetHandle> handle =
      KeysetHandleBuilder()
          .AddEntry(KeysetHandleBuilder::Entry::CreateFromKey(
              param.hybrid_private_key, KeyStatus::kEnabled,
              /*is_primary=*/true))
          .Build();
  ASSERT_THAT(handle, IsOk());
  absl::StatusOr<std::unique_ptr<HybridDecrypt>> decrypter =
      handle->GetPrimitive<HybridDecrypt>(ConfigGlobalRegistry());
  ASSERT_THAT(decrypter, IsOk());
  EXPECT_THAT(
      (*decrypter)
          ->Decrypt(param.ciphertext, absl::StrCat(param.context_info, "x")),
      Not(IsOk()));
}

TEST_P(EciesTestVectorTest, EncryptThenDecryptWorks) {
  ASSERT_THAT(HybridConfig::Register(), IsOk());
  const HybridTestVector& param = GetParam();
  absl::StatusOr<KeysetHandle> handle =
      KeysetHandleBuilder()
          .AddEntry(KeysetHandleBuilder::Entry::CreateFromKey(
              param.hybrid_private_key, KeyStatus::kEnabled,
              /*is_primary=*/true))
          .Build();
  ASSERT_THAT(handle, IsOk());
  absl::StatusOr<std::unique_ptr<HybridDecrypt>> decrypter =
      handle->GetPrimitive<HybridDecrypt>(ConfigGlobalRegistry());
  ASSERT_THAT(decrypter, IsOk());

  absl::StatusOr<std::unique_ptr<KeysetHandle>> public_handle =
      handle->GetPublicKeysetHandle(KeyGenConfigGlobalRegistry());
  ASSERT_THAT(public_handle, IsOk());
  absl::StatusOr<std::unique_ptr<HybridEncrypt>> encrypter =
      (*public_handle)->GetPrimitive<HybridEncrypt>(ConfigGlobalRegistry());
  ASSERT_THAT(encrypter, IsOk());

  absl::StatusOr<std::string> ciphertext =
      (*encrypter)->Encrypt(param.plaintext, param.context_info);
  ASSERT_THAT(ciphertext, IsOk());
  EXPECT_THAT((*decrypter)->Decrypt(*ciphertext, param.context_info),
              IsOkAndHolds(Eq(param.plaintext)));
}

INSTANTIATE_TEST_SUITE_P(EciesTestVectorTest, EciesTestVectorTest,
                         testing::ValuesIn(internal::CreateEciesTestVectors()));

}  // namespace
}  // namespace tink
}  // namespace crypto
