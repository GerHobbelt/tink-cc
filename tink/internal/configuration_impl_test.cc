// Copyright 2023 Google LLC
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

#include "tink/internal/configuration_impl.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "tink/aead/aes_gcm_key.h"
#include "tink/aead/aes_gcm_proto_serialization.h"
#include "tink/configuration.h"
#include "tink/core/key_manager_impl.h"
#include "tink/core/key_type_manager.h"
#include "tink/core/private_key_type_manager.h"
#include "tink/core/template_util.h"
#include "tink/input_stream.h"
#include "tink/internal/key_type_info_store.h"
#include "tink/internal/keyset_wrapper.h"
#include "tink/internal/keyset_wrapper_store.h"
#include "tink/key_manager.h"
#include "tink/keyset_handle.h"
#include "tink/primitive_set.h"
#include "tink/primitive_wrapper.h"
#include "tink/public_key_sign.h"
#include "tink/public_key_verify.h"
#include "tink/registry.h"
#include "tink/subtle/random.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"
#include "tink/util/test_matchers.h"
#include "tink/util/test_util.h"
#include "proto/aes_gcm.pb.h"
#include "proto/rsa_ssa_pss.pb.h"
#include "proto/tink.pb.h"

namespace crypto {
namespace tink {
namespace internal {
namespace {

using ::crypto::tink::test::IsOk;
using ::crypto::tink::test::StatusIs;
using ::google::crypto::tink::AesGcmKeyFormat;
using ::google::crypto::tink::KeyData;
using ::google::crypto::tink::Keyset;
using ::google::crypto::tink::KeyStatusType;
using ::google::crypto::tink::OutputPrefixType;
using ::google::crypto::tink::RsaSsaPssKeyFormat;
using ::google::crypto::tink::RsaSsaPssParams;
using ::google::crypto::tink::RsaSsaPssPrivateKey;
using ::google::crypto::tink::RsaSsaPssPublicKey;

class FakePrimitive {
 public:
  explicit FakePrimitive(absl::string_view s) : s_(s) {}
  std::string get() { return s_; }

 private:
  std::string s_;
};

class FakePrimitive2 {
 public:
  explicit FakePrimitive2(absl::string_view s) : s_(s) {}
  std::string get() { return s_ + "2"; }

 private:
  std::string s_;
};

// Transforms AesGcmKey into FakePrimitive.
class FakeKeyTypeManager
    : public KeyTypeManager<google::crypto::tink::AesGcmKey, AesGcmKeyFormat,
                            List<FakePrimitive>> {
 public:
  class FakePrimitiveFactory : public PrimitiveFactory<FakePrimitive> {
   public:
    absl::StatusOr<std::unique_ptr<FakePrimitive>> Create(
        const google::crypto::tink::AesGcmKey& key) const override {
      return absl::make_unique<FakePrimitive>(key.key_value());
    }
  };

  FakeKeyTypeManager()
      : KeyTypeManager(absl::make_unique<FakePrimitiveFactory>()) {}

  KeyData::KeyMaterialType key_material_type() const override {
    return KeyData::SYMMETRIC;
  }

  uint32_t get_version() const override { return 0; }

  const std::string& get_key_type() const override { return key_type_; }

  absl::Status ValidateKey(
      const google::crypto::tink::AesGcmKey& key) const override {
    return absl::OkStatus();
  }

  absl::Status ValidateKeyFormat(
      const AesGcmKeyFormat& key_format) const override {
    return absl::OkStatus();
  }

  absl::StatusOr<google::crypto::tink::AesGcmKey> CreateKey(
      const AesGcmKeyFormat& key_format) const override {
    return google::crypto::tink::AesGcmKey();
  }

  absl::StatusOr<google::crypto::tink::AesGcmKey> DeriveKey(
      const AesGcmKeyFormat& key_format,
      InputStream* input_stream) const override {
    return google::crypto::tink::AesGcmKey();
  }

 private:
  const std::string key_type_ =
      "type.googleapis.com/google.crypto.tink.AesGcmKey";
};

// Transforms FakePrimitive into FakePrimitive.
class FakePrimitiveWrapper
    : public PrimitiveWrapper<FakePrimitive, FakePrimitive> {
 public:
  absl::StatusOr<std::unique_ptr<FakePrimitive>> Wrap(
      std::unique_ptr<PrimitiveSet<FakePrimitive>> primitive_set)
      const override {
    return absl::make_unique<FakePrimitive>(
        primitive_set->get_primary()->get_primitive().get());
  }
};

// Transforms FakePrimitive2 into FakePrimitive.
class FakePrimitiveWrapper2
    : public PrimitiveWrapper<FakePrimitive2, FakePrimitive> {
 public:
  absl::StatusOr<std::unique_ptr<FakePrimitive>> Wrap(
      std::unique_ptr<PrimitiveSet<FakePrimitive2>> primitive_set)
      const override {
    return absl::make_unique<FakePrimitive>(
        primitive_set->get_primary()->get_primitive().get());
  }
};

std::function<
    absl::StatusOr<std::unique_ptr<FakePrimitive>>(const AesGcmKey& key)>
FakePrimitiveGetterFromKey() {
  return [](const AesGcmKey& key) {
    return absl::make_unique<FakePrimitive>("primitive from key");
  };
}

std::string AddAesGcmKeyToKeyset(Keyset& keyset, uint32_t key_id,
                                 OutputPrefixType output_prefix_type,
                                 KeyStatusType key_status_type) {
  google::crypto::tink::AesGcmKey key;
  key.set_version(0);
  key.set_key_value(subtle::Random::GetRandomBytes(16));
  KeyData key_data;
  key_data.set_value(key.SerializeAsString());
  key_data.set_type_url("type.googleapis.com/google.crypto.tink.AesGcmKey");
  test::AddKeyData(key_data, key_id, output_prefix_type, key_status_type,
                   &keyset);
  return std::string(key.key_value());
}

TEST(ConfigurationImplTest, AddPrimitiveWrapper) {
  Configuration config;
  EXPECT_THAT((ConfigurationImpl::AddPrimitiveWrapper(
                  absl::make_unique<FakePrimitiveWrapper>(), config)),
              IsOk());
}

TEST(ConfigurationImplTest, AddKeyTypeManager) {
  Configuration config;
  EXPECT_THAT(ConfigurationImpl::AddKeyTypeManager(
                  absl::make_unique<FakeKeyTypeManager>(), config),
              IsOk());
}

TEST(ConfigurationImplTest, AddLegacyKeyManager) {
  Configuration config;
  FakeKeyTypeManager manager;
  EXPECT_THAT(ConfigurationImpl::AddLegacyKeyManager(
                  MakeKeyManager<FakePrimitive>(&manager), config),
              IsOk());
}

TEST(ConfigurationImplTest, AddPrimitiveGetter) {
  Configuration config;
  EXPECT_THAT((ConfigurationImpl::AddPrimitiveGetter<FakePrimitive, AesGcmKey>(
                  FakePrimitiveGetterFromKey(), config)),
              IsOk());
}

TEST(ConfigurationImplTest, AddPrimitiveGetterForSameTupleTwiceFails) {
  Configuration config;
  EXPECT_THAT((ConfigurationImpl::AddPrimitiveGetter<FakePrimitive, AesGcmKey>(
                  FakePrimitiveGetterFromKey(), config)),
              IsOk());
  EXPECT_THAT((ConfigurationImpl::AddPrimitiveGetter<FakePrimitive, AesGcmKey>(
                  FakePrimitiveGetterFromKey(), config)),
              StatusIs(absl::StatusCode::kAlreadyExists));
}

TEST(ConfigurationImplTest, GetKeyTypeInfoStore) {
  Configuration config;
  ASSERT_THAT(ConfigurationImpl::AddKeyTypeManager(
                  absl::make_unique<FakeKeyTypeManager>(), config),
              IsOk());

  EXPECT_THAT(ConfigurationImpl::GetKeyTypeInfoStore(config), IsOk());
}

TEST(ConfigurationImplTest, GetKeyTypeManager) {
  Configuration config;
  ASSERT_THAT(ConfigurationImpl::AddKeyTypeManager(
                  absl::make_unique<FakeKeyTypeManager>(), config),
              IsOk());

  absl::StatusOr<const KeyTypeInfoStore*> store =
      ConfigurationImpl::GetKeyTypeInfoStore(config);
  ASSERT_THAT(store, IsOk());
  std::string type_url = FakeKeyTypeManager().get_key_type();
  absl::StatusOr<const KeyTypeInfoStore::Info*> info = (*store)->Get(type_url);
  ASSERT_THAT(info, IsOk());

  absl::StatusOr<const KeyManager<FakePrimitive>*> key_manager =
      (*info)->get_key_manager<FakePrimitive>(type_url);
  ASSERT_THAT(key_manager, IsOk());
  EXPECT_EQ((*key_manager)->get_key_type(), type_url);
}

TEST(ConfigurationImplTest, GetLegacyKeyManager) {
  Configuration config;
  FakeKeyTypeManager manager;
  ASSERT_THAT(ConfigurationImpl::AddLegacyKeyManager(
                  MakeKeyManager<FakePrimitive>(&manager), config),
              IsOk());

  absl::StatusOr<const KeyTypeInfoStore*> store =
      ConfigurationImpl::GetKeyTypeInfoStore(config);
  ASSERT_THAT(store, IsOk());
  std::string type_url = FakeKeyTypeManager().get_key_type();
  absl::StatusOr<const KeyTypeInfoStore::Info*> info = (*store)->Get(type_url);
  ASSERT_THAT(info, IsOk());

  absl::StatusOr<const KeyManager<FakePrimitive>*> key_manager =
      (*info)->get_key_manager<FakePrimitive>(type_url);
  ASSERT_THAT(key_manager, IsOk());
  EXPECT_EQ((*key_manager)->get_key_type(), type_url);
}

TEST(ConfigurationImplTest, GetMissingKeyManagerFails) {
  Configuration config;
  absl::StatusOr<const KeyTypeInfoStore*> store =
      ConfigurationImpl::GetKeyTypeInfoStore(config);
  ASSERT_THAT(store, IsOk());
  EXPECT_THAT((*store)->Get("i.do.not.exist").status(),
              StatusIs(absl::StatusCode::kNotFound));
}

TEST(ConfigurationImplTest, GetKeysetWrapperStoreAndWrap) {
  Configuration config;
  ASSERT_THAT((ConfigurationImpl::AddPrimitiveWrapper(
                  absl::make_unique<FakePrimitiveWrapper>(), config)),
              IsOk());
  ASSERT_THAT(ConfigurationImpl::AddKeyTypeManager(
                  absl::make_unique<FakeKeyTypeManager>(), config),
              IsOk());

  absl::StatusOr<const KeysetWrapperStore*> store =
      ConfigurationImpl::GetKeysetWrapperStore(config);
  ASSERT_THAT(store, IsOk());
  absl::StatusOr<const KeysetWrapper<FakePrimitive>*> wrapper =
      (*store)->Get<FakePrimitive>();
  ASSERT_THAT(wrapper, IsOk());

  Keyset keyset;
  std::string raw_key = AddAesGcmKeyToKeyset(
      keyset, /*key_id=*/13, OutputPrefixType::TINK, KeyStatusType::ENABLED);
  keyset.set_primary_key_id(13);

  absl::StatusOr<std::unique_ptr<FakePrimitive>> aead =
      (*wrapper)->Wrap(keyset, /*annotations=*/{});
  ASSERT_THAT(aead, IsOk());
  EXPECT_EQ((*aead)->get(), raw_key);
}

TEST(ConfigurationImplTest, GetKeysetWrapperStoreAndWrapFromKey) {
  ASSERT_THAT(RegisterAesGcmProtoSerialization(), IsOk());

  Configuration config;
  ASSERT_THAT((ConfigurationImpl::AddPrimitiveWrapper(
                  absl::make_unique<FakePrimitiveWrapper>(), config)),
              IsOk());
  ASSERT_THAT((ConfigurationImpl::AddPrimitiveGetter<FakePrimitive, AesGcmKey>(
                  FakePrimitiveGetterFromKey(), config)),
              IsOk());

  absl::StatusOr<const KeysetWrapperStore*> store =
      ConfigurationImpl::GetKeysetWrapperStore(config);
  ASSERT_THAT(store, IsOk());
  absl::StatusOr<const KeysetWrapper<FakePrimitive>*> wrapper =
      (*store)->Get<FakePrimitive>();
  ASSERT_THAT(wrapper, IsOk());

  Keyset keyset;
  std::string raw_key = AddAesGcmKeyToKeyset(
      keyset, /*key_id=*/13, OutputPrefixType::TINK, KeyStatusType::ENABLED);
  keyset.set_primary_key_id(13);

  absl::StatusOr<std::unique_ptr<FakePrimitive>> aead =
      (*wrapper)->Wrap(keyset, /*annotations=*/{});
  ASSERT_THAT(aead, IsOk());
  EXPECT_EQ((*aead)->get(), "primitive from key");
}

TEST(ConfigurationImplTest, KeysetWrapperWrapMissingKeyTypeInfoFails) {
  Configuration config;
  ASSERT_THAT(ConfigurationImpl::AddPrimitiveWrapper(
                  absl::make_unique<FakePrimitiveWrapper>(), config),
              IsOk());

  absl::StatusOr<const KeysetWrapperStore*> store =
      ConfigurationImpl::GetKeysetWrapperStore(config);
  ASSERT_THAT(store, IsOk());
  absl::StatusOr<const KeysetWrapper<FakePrimitive>*> wrapper =
      (*store)->Get<FakePrimitive>();
  ASSERT_THAT(wrapper, IsOk());

  Keyset keyset;
  std::string raw_key = AddAesGcmKeyToKeyset(
      keyset, /*key_id=*/13, OutputPrefixType::TINK, KeyStatusType::ENABLED);
  keyset.set_primary_key_id(13);

  EXPECT_THAT((*wrapper)->Wrap(keyset, /*annotations=*/{}).status(),
              StatusIs(absl::StatusCode::kNotFound));
}

TEST(ConfigurationImplTest, KeysetWrapperWrapMissingKeyManagerFails) {
  Configuration config;
  // Transforms FakePrimitive2 to FakePrimitive.
  ASSERT_THAT((ConfigurationImpl::AddPrimitiveWrapper(
                  absl::make_unique<FakePrimitiveWrapper2>(), config)),
              IsOk());
  // Transforms KeyData to FakePrimitive.
  ASSERT_THAT(ConfigurationImpl::AddKeyTypeManager(
                  absl::make_unique<FakeKeyTypeManager>(), config),
              IsOk());

  // AesGcmKey KeyData -> FakePrimitive2 -> FakePrimitive is the success path,
  // but the AesGcmKey KeyData -> FakePrimitive2 transformation is not
  // registered.
  absl::StatusOr<const KeysetWrapperStore*> store =
      ConfigurationImpl::GetKeysetWrapperStore(config);
  ASSERT_THAT(store, IsOk());
  absl::StatusOr<const KeysetWrapper<FakePrimitive>*> wrapper =
      (*store)->Get<FakePrimitive>();
  ASSERT_THAT(wrapper, IsOk());

  Keyset keyset;
  std::string raw_key = AddAesGcmKeyToKeyset(
      keyset, /*key_id=*/13, OutputPrefixType::TINK, KeyStatusType::ENABLED);
  keyset.set_primary_key_id(13);

  // FakeKeyTypeManager cannot transform AesGcmKey KeyData -> FakePrimitive2.
  EXPECT_THAT((*wrapper)->Wrap(keyset, /*annotations=*/{}).status(),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

class FakeSignKeyManager
    : public PrivateKeyTypeManager<RsaSsaPssPrivateKey, RsaSsaPssKeyFormat,
                                   RsaSsaPssPublicKey, List<PublicKeySign>> {
 public:
  class PublicKeySignFactory : public PrimitiveFactory<PublicKeySign> {
   public:
    absl::StatusOr<std::unique_ptr<PublicKeySign>> Create(
        const RsaSsaPssPrivateKey& key) const override {
      return {absl::make_unique<test::DummyPublicKeySign>("a public key sign")};
    }
  };

  explicit FakeSignKeyManager()
      : PrivateKeyTypeManager(absl::make_unique<PublicKeySignFactory>()) {}

  KeyData::KeyMaterialType key_material_type() const override {
    return KeyData::ASYMMETRIC_PRIVATE;
  }

  uint32_t get_version() const override { return 0; }

  const std::string& get_key_type() const override { return key_type_; }

  absl::Status ValidateKey(const RsaSsaPssPrivateKey& key) const override {
    return absl::OkStatus();
  }

  absl::Status ValidateKeyFormat(
      const RsaSsaPssKeyFormat& key_format) const override {
    return absl::OkStatus();
  }

  absl::StatusOr<RsaSsaPssPrivateKey> CreateKey(
      const RsaSsaPssKeyFormat& key_format) const override {
    return RsaSsaPssPrivateKey();
  }

  absl::StatusOr<RsaSsaPssPrivateKey> DeriveKey(
      const RsaSsaPssKeyFormat& key_format,
      InputStream* input_stream) const override {
    return RsaSsaPssPrivateKey();
  }

  absl::StatusOr<RsaSsaPssPublicKey> GetPublicKey(
      const RsaSsaPssPrivateKey& private_key) const override {
    return private_key.public_key();
  }

 private:
  const std::string key_type_ = "some.sign.key.type";
};

class FakeVerifyKeyManager
    : public KeyTypeManager<RsaSsaPssPublicKey, void, List<PublicKeyVerify>> {
 public:
  class PublicKeyVerifyFactory : public PrimitiveFactory<PublicKeyVerify> {
   public:
    absl::StatusOr<std::unique_ptr<PublicKeyVerify>> Create(
        const RsaSsaPssPublicKey& key) const override {
      return {
          absl::make_unique<test::DummyPublicKeyVerify>("a public key verify")};
    }
  };

  explicit FakeVerifyKeyManager()
      : KeyTypeManager(absl::make_unique<PublicKeyVerifyFactory>()) {}

  KeyData::KeyMaterialType key_material_type() const override {
    return KeyData::ASYMMETRIC_PUBLIC;
  }

  uint32_t get_version() const override { return 0; }

  const std::string& get_key_type() const override { return key_type_; }

  absl::Status ValidateKey(const RsaSsaPssPublicKey& key) const override {
    return absl::OkStatus();
  }

  absl::Status ValidateParams(const RsaSsaPssParams& params) const {
    return absl::OkStatus();
  }

 private:
  const std::string key_type_ = "some.verify.key.type";
};

TEST(ConfigurationImplTest, AddAsymmetricKeyManagers) {
  Configuration config;
  EXPECT_THAT(ConfigurationImpl::AddAsymmetricKeyManagers(
                  absl::make_unique<FakeSignKeyManager>(),
                  absl::make_unique<FakeVerifyKeyManager>(), config),
              IsOk());
}

TEST(ConfigurationImplTest, GetAsymmetricKeyManagers) {
  Configuration config;
  ASSERT_THAT(ConfigurationImpl::AddAsymmetricKeyManagers(
                  absl::make_unique<FakeSignKeyManager>(),
                  absl::make_unique<FakeVerifyKeyManager>(), config),
              IsOk());

  {
    std::string type_url = FakeSignKeyManager().get_key_type();
    absl::StatusOr<const KeyTypeInfoStore*> store =
        ConfigurationImpl::GetKeyTypeInfoStore(config);
    ASSERT_THAT(store, IsOk());
    absl::StatusOr<const KeyTypeInfoStore::Info*> info =
        (*store)->Get(type_url);
    ASSERT_THAT(info, IsOk());

    absl::StatusOr<const KeyManager<PublicKeySign>*> key_manager =
        (*info)->get_key_manager<PublicKeySign>(type_url);
    ASSERT_THAT(key_manager, IsOk());
    EXPECT_EQ((*key_manager)->get_key_type(), type_url);
  }
  {
    std::string type_url = FakeVerifyKeyManager().get_key_type();
    absl::StatusOr<const KeyTypeInfoStore*> store =
        ConfigurationImpl::GetKeyTypeInfoStore(config);
    ASSERT_THAT(store, IsOk());
    absl::StatusOr<const KeyTypeInfoStore::Info*> info =
        (*store)->Get(type_url);
    ASSERT_THAT(info, IsOk());

    absl::StatusOr<const KeyManager<PublicKeyVerify>*> key_manager =
        (*info)->get_key_manager<PublicKeyVerify>(type_url);
    ASSERT_THAT(key_manager, IsOk());
    EXPECT_EQ((*key_manager)->get_key_type(), type_url);
  }
}

TEST(ConfigurationImplTest, GlobalRegistryMode) {
  Registry::Reset();
  Configuration config;
  ASSERT_THAT(ConfigurationImpl::SetGlobalRegistryMode(config), IsOk());
  EXPECT_TRUE(ConfigurationImpl::IsInGlobalRegistryMode(config));

  // Check that ConfigurationImpl functions return kFailedPrecondition.
  EXPECT_THAT(ConfigurationImpl::AddPrimitiveWrapper(
                  absl::make_unique<FakePrimitiveWrapper>(), config),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ConfigurationImpl::AddKeyTypeManager(
                  absl::make_unique<FakeKeyTypeManager>(), config),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ConfigurationImpl::AddAsymmetricKeyManagers(
                  absl::make_unique<FakeSignKeyManager>(),
                  absl::make_unique<FakeVerifyKeyManager>(), config),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT((ConfigurationImpl::AddPrimitiveGetter<FakePrimitive, AesGcmKey>(
                  FakePrimitiveGetterFromKey(), config)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  FakeKeyTypeManager manager;
  EXPECT_THAT(ConfigurationImpl::AddLegacyKeyManager(
                  MakeKeyManager<FakePrimitive>(&manager), config),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ConfigurationImpl::GetKeyTypeInfoStore(config).status(),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ConfigurationImpl::GetKeysetWrapperStore(config).status(),
              StatusIs(absl::StatusCode::kFailedPrecondition));

  Keyset keyset;
  std::string raw_key = AddAesGcmKeyToKeyset(
      keyset, /*key_id=*/13, OutputPrefixType::TINK, KeyStatusType::ENABLED);
  keyset.set_primary_key_id(13);
  std::unique_ptr<KeysetHandle> handle =
      CleartextKeysetHandle::GetKeysetHandle(keyset);
  EXPECT_THAT(handle->GetPrimitive<FakePrimitive>(config).status(),
              StatusIs(absl::StatusCode::kNotFound));

  ASSERT_THAT(Registry::RegisterPrimitiveWrapper(
                  absl::make_unique<FakePrimitiveWrapper>()),
              IsOk());
  ASSERT_THAT(
      Registry::RegisterKeyTypeManager(absl::make_unique<FakeKeyTypeManager>(),
                                       /*new_key_allowed=*/true),
      IsOk());
  EXPECT_THAT(handle->GetPrimitive<FakePrimitive>(config), IsOk());
}

TEST(ConfigurationImplTest, GlobalRegistryModeWithNonEmptyConfigFails) {
  Configuration config;
  ASSERT_THAT(ConfigurationImpl::AddPrimitiveWrapper(
                  absl::make_unique<FakePrimitiveWrapper>(), config),
              IsOk());
  EXPECT_THAT(ConfigurationImpl::SetGlobalRegistryMode(config),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_FALSE(ConfigurationImpl::IsInGlobalRegistryMode(config));
}

}  // namespace
}  // namespace internal
}  // namespace tink
}  // namespace crypto
