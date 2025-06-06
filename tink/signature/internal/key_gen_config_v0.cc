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
////////////////////////////////////////////////////////////////////////////////

#include "tink/signature/internal/key_gen_config_v0.h"

#include "absl/memory/memory.h"
#include "tink/internal/key_gen_configuration_impl.h"
#include "tink/key_gen_configuration.h"
#include "tink/signature/ecdsa_proto_serialization.h"
#include "tink/signature/ecdsa_sign_key_manager.h"
#include "tink/signature/ecdsa_verify_key_manager.h"
#include "tink/signature/ed25519_proto_serialization.h"
#include "tink/signature/ed25519_sign_key_manager.h"
#include "tink/signature/ed25519_verify_key_manager.h"
#include "tink/signature/rsa_ssa_pkcs1_proto_serialization.h"
#include "tink/signature/rsa_ssa_pss_proto_serialization.h"
#ifdef OPENSSL_IS_BORINGSSL
#include "tink/signature/internal/key_creators.h"
#include "tink/signature/internal/ml_dsa_proto_serialization.h"
#include "tink/signature/internal/slh_dsa_proto_serialization.h"
#include "tink/signature/ml_dsa_parameters.h"
#include "tink/signature/slh_dsa_parameters.h"
#endif
#include "tink/signature/rsa_ssa_pkcs1_sign_key_manager.h"
#include "tink/signature/rsa_ssa_pkcs1_verify_key_manager.h"
#include "tink/signature/rsa_ssa_pss_sign_key_manager.h"
#include "tink/signature/rsa_ssa_pss_verify_key_manager.h"
#include "tink/util/status.h"

namespace crypto {
namespace tink {
namespace internal {

absl::Status AddSignatureKeyGenV0(KeyGenConfiguration& config) {
  absl::Status status = RegisterEcdsaProtoSerialization();
  if (!status.ok()) {
    return status;
  }
  status = KeyGenConfigurationImpl::AddAsymmetricKeyManagers(
      absl::make_unique<EcdsaSignKeyManager>(),
      absl::make_unique<EcdsaVerifyKeyManager>(), config);
  if (!status.ok()) {
    return status;
  }
  status = RegisterRsaSsaPssProtoSerialization();
  if (!status.ok()) {
    return status;
  }
  status = KeyGenConfigurationImpl::AddAsymmetricKeyManagers(
      absl::make_unique<RsaSsaPssSignKeyManager>(),
      absl::make_unique<RsaSsaPssVerifyKeyManager>(), config);
  if (!status.ok()) {
    return status;
  }
  status = RegisterRsaSsaPkcs1ProtoSerialization();
  if (!status.ok()) {
    return status;
  }
  status = KeyGenConfigurationImpl::AddAsymmetricKeyManagers(
      absl::make_unique<RsaSsaPkcs1SignKeyManager>(),
      absl::make_unique<RsaSsaPkcs1VerifyKeyManager>(), config);
  if (!status.ok()) {
    return status;
  }
  status = RegisterEd25519ProtoSerialization();
  if (!status.ok()) {
    return status;
  }
  status = KeyGenConfigurationImpl::AddAsymmetricKeyManagers(
      absl::make_unique<Ed25519SignKeyManager>(),
      absl::make_unique<Ed25519VerifyKeyManager>(), config);
  if (!status.ok()) {
    return status;
  }

  // Tink implements PQC signatures with BoringSSL, not OpenSSL.
#ifdef OPENSSL_IS_BORINGSSL
  // SLH-DSA
  status = RegisterSlhDsaProtoSerialization();
  if (!status.ok()) {
    return status;
  }
  status = KeyGenConfigurationImpl::AddKeyCreator<SlhDsaParameters>(
      CreateSlhDsaKey, config);
  if (!status.ok()) {
    return status;
  }
  // ML-DSA
  status = RegisterMlDsaProtoSerialization();
  if (!status.ok()) {
    return status;
  }
  status = KeyGenConfigurationImpl::AddKeyCreator<MlDsaParameters>(
      CreateMlDsaKey, config);
  if (!status.ok()) {
    return status;
  }
#endif
  return absl::OkStatus();
}

}  // namespace internal
}  // namespace tink
}  // namespace crypto
