// Copyright 2024 Google LLC
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

#include "tink/jwt/jwt_rsa_ssa_pkcs1_private_key.h"

#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "tink/internal/rsa_util.h"
#ifdef OPENSSL_IS_BORINGSSL
#include "openssl/base.h"
#endif
#include "openssl/rsa.h"
#include "tink/big_integer.h"
#include "tink/insecure_secret_key_access.h"
#include "tink/internal/ssl_unique_ptr.h"
#include "tink/jwt/jwt_rsa_ssa_pkcs1_public_key.h"
#include "tink/key.h"
#include "tink/partial_key_access_token.h"
#include "tink/restricted_big_integer.h"

namespace crypto {
namespace tink {
namespace {

absl::Status ValidateKeyPair(
    const BigInteger& public_exponent, const BigInteger& modulus,
    const RestrictedBigInteger& p, const RestrictedBigInteger& q,
    const RestrictedBigInteger& d, const RestrictedBigInteger& dp,
    const RestrictedBigInteger& dq, const RestrictedBigInteger& q_inv) {
  absl::StatusOr<internal::SslUniquePtr<RSA>> rsa =
      internal::RsaPrivateKeyToRsa(internal::RsaPrivateKey{
          /*n=*/std::string(modulus.GetValue()),
          /*e=*/std::string(public_exponent.GetValue()),
          /*d=*/d.GetSecretData(InsecureSecretKeyAccess::Get()),
          /*p=*/p.GetSecretData(InsecureSecretKeyAccess::Get()),
          /*q=*/q.GetSecretData(InsecureSecretKeyAccess::Get()),
          /*dp=*/dp.GetSecretData(InsecureSecretKeyAccess::Get()),
          /*dq=*/dq.GetSecretData(InsecureSecretKeyAccess::Get()),
          /*crt=*/q_inv.GetSecretData(InsecureSecretKeyAccess::Get()),
      });
  return rsa.status();
}

}  // namespace

JwtRsaSsaPkcs1PrivateKey::Builder&
JwtRsaSsaPkcs1PrivateKey::Builder::SetPublicKey(
    const JwtRsaSsaPkcs1PublicKey& public_key) {
  public_key_ = public_key;
  return *this;
}

JwtRsaSsaPkcs1PrivateKey::Builder& JwtRsaSsaPkcs1PrivateKey::Builder::SetPrimeP(
    const RestrictedBigInteger& p) {
  p_ = p;
  return *this;
}

JwtRsaSsaPkcs1PrivateKey::Builder& JwtRsaSsaPkcs1PrivateKey::Builder::SetPrimeQ(
    const RestrictedBigInteger& q) {
  q_ = q;
  return *this;
}

JwtRsaSsaPkcs1PrivateKey::Builder&
JwtRsaSsaPkcs1PrivateKey::Builder::SetPrimeExponentP(
    const RestrictedBigInteger& dp) {
  dp_ = dp;
  return *this;
}

JwtRsaSsaPkcs1PrivateKey::Builder&
JwtRsaSsaPkcs1PrivateKey::Builder::SetPrimeExponentQ(
    const RestrictedBigInteger& dq) {
  dq_ = dq;
  return *this;
}

JwtRsaSsaPkcs1PrivateKey::Builder&
JwtRsaSsaPkcs1PrivateKey::Builder::SetPrivateExponent(
    const RestrictedBigInteger& d) {
  d_ = d;
  return *this;
}

JwtRsaSsaPkcs1PrivateKey::Builder&
JwtRsaSsaPkcs1PrivateKey::Builder::SetCrtCoefficient(
    const RestrictedBigInteger& q_inv) {
  q_inv_ = q_inv;
  return *this;
}

absl::StatusOr<JwtRsaSsaPkcs1PrivateKey>
JwtRsaSsaPkcs1PrivateKey::Builder::Build(PartialKeyAccessToken token) {
  if (!public_key_.has_value()) {
    return absl::Status(absl::StatusCode::kInvalidArgument,
                        "Cannot build without setting the public key");
  }

  if (!p_.has_value() || !q_.has_value()) {
    return absl::Status(absl::StatusCode::kInvalidArgument,
                        "Cannot build without setting both prime factors");
  }

  if (!dp_.has_value() || !dq_.has_value()) {
    return absl::Status(absl::StatusCode::kInvalidArgument,
                        "Cannot build without setting both prime exponents");
  }

  if (!d_.has_value()) {
    return absl::Status(absl::StatusCode::kInvalidArgument,
                        "Cannot build without setting the private exponent");
  }

  if (!q_inv_.has_value()) {
    return absl::Status(absl::StatusCode::kInvalidArgument,
                        "Cannot build without setting the CRT coefficient");
  }

  // Validate key pair.
  absl::Status key_pair_validation = ValidateKeyPair(
      public_key_->GetParameters().GetPublicExponent(),
      public_key_->GetModulus(token), *p_, *q_, *d_, *dp_, *dq_, *q_inv_);
  if (!key_pair_validation.ok()) {
    return key_pair_validation;
  }

  return JwtRsaSsaPkcs1PrivateKey(*public_key_, *p_, *q_, *dp_, *dq_, *d_,
                                  *q_inv_);
}

bool JwtRsaSsaPkcs1PrivateKey::operator==(const Key& other) const {
  const JwtRsaSsaPkcs1PrivateKey* that =
      dynamic_cast<const JwtRsaSsaPkcs1PrivateKey*>(&other);
  if (that == nullptr) {
    return false;
  }

  return public_key_ == that->public_key_ && p_ == that->p_ && q_ == that->q_ &&
         dp_ == that->dp_ && dq_ == that->dq_ && d_ == that->d_ &&
         q_inv_ == that->q_inv_;
}

}  // namespace tink
}  // namespace crypto
