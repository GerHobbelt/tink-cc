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

#ifndef TINK_EXPERIMENTAL_PQCRYPTO_SIGNATURE_SUBTLE_SPHINCS_SUBTLE_UTILS_H_
#define TINK_EXPERIMENTAL_PQCRYPTO_SIGNATURE_SUBTLE_SPHINCS_SUBTLE_UTILS_H_

#include <cstdint>
#include <string>
#include <utility>

#include "absl/memory/memory.h"
#include "absl/strings/str_format.h"
#include "tink/util/secret_data.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"

namespace crypto {
namespace tink {
namespace subtle {

// The three possible sphincs private key sizes.
const int kSphincsPrivateKeySize64 = 64;
const int kSphincsPrivateKeySize96 = 96;
const int kSphincsPrivateKeySize128 = 128;

// The three possible sphincs public key sizes.
const int kSphincsPublicKeySize32 = 32;
const int kSphincsPublicKeySize48 = 48;
const int kSphincsPublicKeySize64 = 64;

enum SphincsHashType {
  HASH_TYPE_UNSPECIFIED = 0,
  HARAKA = 1,
  SHA256 = 2,
  SHAKE256 = 3,
};

enum SphincsVariant {
  VARIANT_UNSPECIFIED = 0,
  ROBUST = 1,
  SIMPLE = 2,
};

enum SphincsSignatureType {
  SIG_TYPE_UNSPECIFIED = 0,
  FAST_SIGNING = 1,
  SMALL_SIGNATURE = 2,
};

struct SphincsParamsPqclean {
  SphincsHashType hash_type;
  SphincsVariant variant;
  SphincsSignatureType sig_length_type;
  int32_t private_key_size;
};

// Representation of the Sphincs private key.
class SphincsPrivateKeyPqclean {
 public:
  explicit SphincsPrivateKeyPqclean(SecretData key_data,
                                    SphincsParamsPqclean params)
      : private_key_data_(std::move(key_data)), params_(std::move(params)) {}

  SphincsPrivateKeyPqclean(const SphincsPrivateKeyPqclean& other) = default;
  SphincsPrivateKeyPqclean& operator=(const SphincsPrivateKeyPqclean& other) =
      default;

  const SecretData& GetKey() const { return private_key_data_; }
  const SphincsParamsPqclean& GetParams() const { return params_; }

 private:
  const SecretData private_key_data_;
  const SphincsParamsPqclean params_;
};

// Representation of the Sphincs public key.
class SphincsPublicKeyPqclean {
 public:
  SphincsPublicKeyPqclean(std::string key_data, SphincsParamsPqclean params)
      : public_key_data_(std::move(key_data)), params_(std::move(params)) {}

  SphincsPublicKeyPqclean(const SphincsPublicKeyPqclean& other) = default;
  SphincsPublicKeyPqclean& operator=(const SphincsPublicKeyPqclean& other) =
      default;

  const std::string& GetKey() const { return public_key_data_; }
  const SphincsParamsPqclean& GetParams() const { return params_; }

 private:
  const std::string public_key_data_;
  const SphincsParamsPqclean params_;
};

class SphincsKeyPair {
 public:
  SphincsKeyPair(SphincsPrivateKeyPqclean private_key,
                 SphincsPublicKeyPqclean public_key)
      : private_key_(std::move(private_key)),
        public_key_(std::move(public_key)) {}

  SphincsKeyPair(const SphincsKeyPair& other) = default;
  SphincsKeyPair& operator=(const SphincsKeyPair& other) = default;

  const SphincsPrivateKeyPqclean& GetPrivateKey() const { return private_key_; }
  const SphincsPublicKeyPqclean& GetPublicKey() const { return public_key_; }

 private:
  const SphincsPrivateKeyPqclean private_key_;
  const SphincsPublicKeyPqclean public_key_;
};

// This is an utility function that generates a new Sphincs key pair based on
// Sphincs specific parameters. This function is expected to be called from
// a key manager class.
absl::StatusOr<SphincsKeyPair> GenerateSphincsKeyPair(
    SphincsParamsPqclean params);

// Validates whether the private key size is safe to use for sphincs signature.
absl::Status ValidatePrivateKeySize(int32_t key_size);

// Validates whether the public key size is safe to use for sphincs signature.
absl::Status ValidatePublicKeySize(int32_t key_size);

// Validates whether the parameters are safe to use for sphincs signature.
absl::Status ValidateParams(SphincsParamsPqclean params);

// Convert the sphincs private key size to the appropriate index in the
// pqclean functions array.
absl::StatusOr<int32_t> SphincsKeySizeToIndex(int32_t key_size);

}  // namespace subtle
}  // namespace tink
}  // namespace crypto

#endif  // TINK_EXPERIMENTAL_PQCRYPTO_SIGNATURE_SUBTLE_SPHINCS_SUBTLE_UTILS_H_
