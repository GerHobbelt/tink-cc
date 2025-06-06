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

#include "tink/signature/internal/ecdsa_proto_serialization_impl.h"

#include <cstdint>
#include <string>

#include "absl/base/no_destructor.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "tink/big_integer.h"
#include "tink/ec_point.h"
#include "tink/insecure_secret_key_access.h"
#include "tink/internal/bn_encoding_util.h"
#include "tink/internal/common_proto_enums.h"
#include "tink/internal/key_parser.h"
#include "tink/internal/key_serializer.h"
#include "tink/internal/mutable_serialization_registry.h"
#include "tink/internal/parameters_parser.h"
#include "tink/internal/parameters_serializer.h"
#include "tink/internal/proto_key_serialization.h"
#include "tink/internal/proto_parameters_serialization.h"
#include "tink/internal/proto_parser.h"
#include "tink/internal/serialization_registry.h"
#include "tink/internal/tink_proto_structs.h"
#include "tink/partial_key_access.h"
#include "tink/restricted_big_integer.h"
#include "tink/restricted_data.h"
#include "tink/secret_key_access_token.h"
#include "tink/signature/ecdsa_parameters.h"
#include "tink/signature/ecdsa_private_key.h"
#include "tink/signature/ecdsa_public_key.h"
#include "tink/util/secret_data.h"

namespace crypto {
namespace tink {
namespace internal {
namespace {

using ::crypto::tink::internal::ProtoParser;
using ::crypto::tink::internal::ProtoParserBuilder;
using ::crypto::tink::util::SecretData;
using ::crypto::tink::util::SecretDataAsStringView;

bool EcdsaSignatureEncodingValid(uint32_t c) { return 0 <= c && c <= 2; }

// Enum representing the proto enum `google.crypto.tink.EcdsaSignatureEncoding`.
enum class EcdsaSignatureEncodingEnum : uint32_t {
  kUnknownEncoding = 0,
  kIeeeP1363,
  kDer,
};

struct EcdsaParamsStruct {
  HashTypeEnum hash_type;
  EllipticCurveTypeEnum curve;
  EcdsaSignatureEncodingEnum encoding;

  static ProtoParser<EcdsaParamsStruct> CreateParser() {
    return ProtoParserBuilder<EcdsaParamsStruct>()
        .AddEnumField(1, &EcdsaParamsStruct::hash_type, &HashTypeEnumIsValid)
        .AddEnumField(2, &EcdsaParamsStruct::curve,
                      &EllipticCurveTypeEnumIsValid)
        .AddEnumField(3, &EcdsaParamsStruct::encoding,
                      &EcdsaSignatureEncodingValid)
        .BuildOrDie();
  }

  static const ProtoParser<EcdsaParamsStruct>& GetParser() {
    static absl::NoDestructor<ProtoParser<EcdsaParamsStruct>> parser{
        CreateParser()};
    return *parser;
  }
};

struct EcdsaPublicKeyStruct {
  uint32_t version;
  EcdsaParamsStruct params;
  std::string x;
  std::string y;

  static ProtoParser<EcdsaPublicKeyStruct> CreateParser() {
    return ProtoParserBuilder<EcdsaPublicKeyStruct>()
        .AddUint32Field(1, &EcdsaPublicKeyStruct::version)
        .AddMessageField(2, &EcdsaPublicKeyStruct::params,
                         EcdsaParamsStruct::CreateParser())
        .AddBytesStringField(3, &EcdsaPublicKeyStruct::x)
        .AddBytesStringField(4, &EcdsaPublicKeyStruct::y)
        .BuildOrDie();
  }

  static const ProtoParser<EcdsaPublicKeyStruct>& GetParser() {
    static absl::NoDestructor<ProtoParser<EcdsaPublicKeyStruct>> parser{
        CreateParser()};
    return *parser;
  }
};

struct EcdsaPrivateKeyStruct {
  uint32_t version;
  EcdsaPublicKeyStruct public_key;
  SecretData key_value;

  static ProtoParser<EcdsaPrivateKeyStruct> CreateParser() {
    return ProtoParserBuilder<EcdsaPrivateKeyStruct>()
        .AddUint32Field(1, &EcdsaPrivateKeyStruct::version)
        .AddMessageField(2, &EcdsaPrivateKeyStruct::public_key,
                         EcdsaPublicKeyStruct::CreateParser())
        .AddBytesSecretDataField(3, &EcdsaPrivateKeyStruct::key_value)
        .BuildOrDie();
  }

  static const ProtoParser<EcdsaPrivateKeyStruct>& GetParser() {
    static absl::NoDestructor<ProtoParser<EcdsaPrivateKeyStruct>> parser{
        CreateParser()};
    return *parser;
  }
};

struct EcdsaKeyFormatStruct {
  EcdsaParamsStruct params;
  uint32_t version;

  static ProtoParser<EcdsaKeyFormatStruct> CreateParser() {
    return ProtoParserBuilder<EcdsaKeyFormatStruct>()
        .AddMessageField(2, &EcdsaKeyFormatStruct::params,
                         EcdsaParamsStruct::CreateParser())
        .AddUint32Field(3, &EcdsaKeyFormatStruct::version)
        .BuildOrDie();
  }

  static const ProtoParser<EcdsaKeyFormatStruct>& GetParser() {
    static absl::NoDestructor<ProtoParser<EcdsaKeyFormatStruct>> parser{
        CreateParser()};
    return *parser;
  }
};

using EcdsaProtoParametersParserImpl =
    ParametersParserImpl<ProtoParametersSerialization, EcdsaParameters>;
using EcdsaProtoParametersSerializerImpl =
    ParametersSerializerImpl<EcdsaParameters, ProtoParametersSerialization>;
using EcdsaProtoPublicKeyParserImpl =
    KeyParserImpl<ProtoKeySerialization, EcdsaPublicKey>;
using EcdsaProtoPublicKeySerializerImpl =
    KeySerializerImpl<EcdsaPublicKey, ProtoKeySerialization>;
using EcdsaProtoPrivateKeyParserImpl =
    KeyParserImpl<ProtoKeySerialization, EcdsaPrivateKey>;
using EcdsaProtoPrivateKeySerializerImpl =
    KeySerializerImpl<EcdsaPrivateKey, ProtoKeySerialization>;

const absl::string_view kPublicTypeUrl =
    "type.googleapis.com/google.crypto.tink.EcdsaPublicKey";
const absl::string_view kPrivateTypeUrl =
    "type.googleapis.com/google.crypto.tink.EcdsaPrivateKey";

absl::StatusOr<EcdsaParameters::Variant> ToVariant(
    OutputPrefixTypeEnum output_prefix_type) {
  switch (output_prefix_type) {
    case OutputPrefixTypeEnum::kLegacy:
      return EcdsaParameters::Variant::kLegacy;
    case OutputPrefixTypeEnum::kCrunchy:
      return EcdsaParameters::Variant::kCrunchy;
    case OutputPrefixTypeEnum::kRaw:
      return EcdsaParameters::Variant::kNoPrefix;
    case OutputPrefixTypeEnum::kTink:
      return EcdsaParameters::Variant::kTink;
    default:
      return absl::InvalidArgumentError(
          "Could not determine output prefix type");
  }
}

absl::StatusOr<OutputPrefixTypeEnum> ToOutputPrefixType(
    EcdsaParameters::Variant variant) {
  switch (variant) {
    case EcdsaParameters::Variant::kLegacy:
      return OutputPrefixTypeEnum::kLegacy;
    case EcdsaParameters::Variant::kCrunchy:
      return OutputPrefixTypeEnum::kCrunchy;
    case EcdsaParameters::Variant::kNoPrefix:
      return OutputPrefixTypeEnum::kRaw;
    case EcdsaParameters::Variant::kTink:
      return OutputPrefixTypeEnum::kTink;
    default:
      return absl::InvalidArgumentError(
          "Could not determine EcdsaParameters::Variant");
  }
}

absl::StatusOr<EcdsaParameters::HashType> ToHashType(HashTypeEnum hash_type) {
  switch (hash_type) {
    case HashTypeEnum::kSha256:
      return EcdsaParameters::HashType::kSha256;
    case HashTypeEnum::kSha384:
      return EcdsaParameters::HashType::kSha384;
    case HashTypeEnum::kSha512:
      return EcdsaParameters::HashType::kSha512;
    default:
      return absl::InvalidArgumentError("Could not determine HashType");
  }
}

absl::StatusOr<HashTypeEnum> ToProtoHashType(
    EcdsaParameters::HashType hash_type) {
  switch (hash_type) {
    case EcdsaParameters::HashType::kSha256:
      return HashTypeEnum::kSha256;
    case EcdsaParameters::HashType::kSha384:
      return HashTypeEnum::kSha384;
    case EcdsaParameters::HashType::kSha512:
      return HashTypeEnum::kSha512;
    default:
      return absl::InvalidArgumentError(
          "Could not determine EcdsaParameters::HashType");
  }
}

absl::StatusOr<EcdsaParameters::CurveType> ToCurveType(
    EllipticCurveTypeEnum curve_type) {
  switch (curve_type) {
    case EllipticCurveTypeEnum::kNistP256:
      return EcdsaParameters::CurveType::kNistP256;
    case EllipticCurveTypeEnum::kNistP384:
      return EcdsaParameters::CurveType::kNistP384;
    case EllipticCurveTypeEnum::kNistP521:
      return EcdsaParameters::CurveType::kNistP521;
    default:
      return absl::InvalidArgumentError(
          "Could not determine EllipticCurveType");
  }
}

absl::StatusOr<EllipticCurveTypeEnum> ToProtoCurveType(
    EcdsaParameters::CurveType curve_type) {
  switch (curve_type) {
    case EcdsaParameters::CurveType::kNistP256:
      return EllipticCurveTypeEnum::kNistP256;
    case EcdsaParameters::CurveType::kNistP384:
      return EllipticCurveTypeEnum::kNistP384;
    case EcdsaParameters::CurveType::kNistP521:
      return EllipticCurveTypeEnum::kNistP521;
    default:
      return absl::InvalidArgumentError(
          "Could not determine EcdsaParameters::CurveType");
  }
}

absl::StatusOr<EcdsaParameters::SignatureEncoding> ToSignatureEncoding(
    EcdsaSignatureEncodingEnum signature_encoding) {
  switch (signature_encoding) {
    case EcdsaSignatureEncodingEnum::kDer:
      return EcdsaParameters::SignatureEncoding::kDer;
    case EcdsaSignatureEncodingEnum::kIeeeP1363:
      return EcdsaParameters::SignatureEncoding::kIeeeP1363;
    default:
      return absl::InvalidArgumentError(
          "Could not determine EcdsaSignatureEncoding");
  }
}

absl::StatusOr<EcdsaSignatureEncodingEnum> ToProtoSignatureEncoding(
    EcdsaParameters::SignatureEncoding signature_encoding) {
  switch (signature_encoding) {
    case EcdsaParameters::SignatureEncoding::kDer:
      return EcdsaSignatureEncodingEnum::kDer;
    case EcdsaParameters::SignatureEncoding::kIeeeP1363:
      return EcdsaSignatureEncodingEnum::kIeeeP1363;
    default:
      return absl::InvalidArgumentError(
          "Could not determine EcdsaParameters::SignatureEncoding");
  }
}

absl::StatusOr<int> getEncodingLength(EcdsaParameters::CurveType curveType) {
  // We currently encode with one extra 0 byte at the beginning, to make sure
  // that parsing is correct. See also b/264525021.
  switch (curveType) {
    case EcdsaParameters::CurveType::kNistP256:
      return 33;
    case EcdsaParameters::CurveType::kNistP384:
      return 49;
    case EcdsaParameters::CurveType::kNistP521:
      return 67;
    default:
      return absl::InvalidArgumentError("Unable to serialize CurveType");
  }
}

absl::StatusOr<EcdsaParameters> ToParameters(
    OutputPrefixTypeEnum output_prefix_type, const EcdsaParamsStruct& params) {
  absl::StatusOr<EcdsaParameters::Variant> variant =
      ToVariant(output_prefix_type);
  if (!variant.ok()) {
    return variant.status();
  }

  absl::StatusOr<EcdsaParameters::HashType> hash_type =
      ToHashType(params.hash_type);
  if (!hash_type.ok()) {
    return hash_type.status();
  }

  absl::StatusOr<EcdsaParameters::CurveType> curve_type =
      ToCurveType(params.curve);
  if (!curve_type.ok()) {
    return curve_type.status();
  }

  absl::StatusOr<EcdsaParameters::SignatureEncoding> encoding =
      ToSignatureEncoding(params.encoding);
  if (!encoding.ok()) {
    return encoding.status();
  }

  return EcdsaParameters::Builder()
      .SetVariant(*variant)
      .SetHashType(*hash_type)
      .SetCurveType(*curve_type)
      .SetSignatureEncoding(*encoding)
      .Build();
}

absl::StatusOr<EcdsaParamsStruct> FromParameters(
    const EcdsaParameters& parameters) {
  absl::StatusOr<EllipticCurveTypeEnum> curve =
      ToProtoCurveType(parameters.GetCurveType());
  if (!curve.ok()) {
    return curve.status();
  }

  absl::StatusOr<HashTypeEnum> hash_type =
      ToProtoHashType(parameters.GetHashType());
  if (!hash_type.ok()) {
    return hash_type.status();
  }

  absl::StatusOr<EcdsaSignatureEncodingEnum> encoding =
      ToProtoSignatureEncoding(parameters.GetSignatureEncoding());
  if (!encoding.ok()) {
    return encoding.status();
  }

  EcdsaParamsStruct params;
  params.curve = *curve;
  params.hash_type = *hash_type;
  params.encoding = *encoding;

  return params;
}

absl::StatusOr<EcdsaParameters> ParseParameters(
    const ProtoParametersSerialization& serialization) {
  const internal::KeyTemplateStruct& key_template =
      serialization.GetKeyTemplateStruct();
  if (key_template.type_url != kPrivateTypeUrl) {
    return absl::InvalidArgumentError(
        "Wrong type URL when parsing EcdsaParameters.");
  }

  absl::StatusOr<EcdsaKeyFormatStruct> proto_key_format =
      EcdsaKeyFormatStruct::GetParser().Parse(key_template.value);
  if (!proto_key_format.ok()) {
    return proto_key_format.status();
  }
  if (proto_key_format->version != 0) {
    return absl::InvalidArgumentError("Only version 0 keys are accepted.");
  }

  return ToParameters(serialization.GetKeyTemplateStruct().output_prefix_type,
                      proto_key_format->params);
}

absl::StatusOr<EcdsaPublicKey> ParsePublicKey(
    const ProtoKeySerialization& serialization,
    absl::optional<SecretKeyAccessToken> token) {
  if (serialization.TypeUrl() != kPublicTypeUrl) {
    return absl::InvalidArgumentError(
        "Wrong type URL when parsing EcdsaPublicKey.");
  }

  absl::StatusOr<EcdsaPublicKeyStruct> proto_key =
      EcdsaPublicKeyStruct::GetParser().Parse(
          serialization.SerializedKeyProto().GetSecret(
              InsecureSecretKeyAccess::Get()));
  if (!proto_key.ok()) {
    return absl::InvalidArgumentError("Failed to parse EcdsaPublicKey proto");
  }
  if (proto_key->version != 0) {
    return absl::InvalidArgumentError("Only version 0 keys are accepted.");
  }

  absl::StatusOr<EcdsaParameters> parameters =
      ToParameters(serialization.GetOutputPrefixTypeEnum(), proto_key->params);
  if (!parameters.ok()) {
    return parameters.status();
  }

  EcPoint public_point(BigInteger(proto_key->x), BigInteger(proto_key->y));
  return EcdsaPublicKey::Create(*parameters, public_point,
                                serialization.IdRequirement(),
                                GetPartialKeyAccess());
}

absl::StatusOr<EcdsaPrivateKey> ParsePrivateKey(
    const ProtoKeySerialization& serialization,
    absl::optional<SecretKeyAccessToken> token) {
  if (serialization.TypeUrl() != kPrivateTypeUrl) {
    return absl::InvalidArgumentError(
        "Wrong type URL when parsing EcdsaPrivateKey.");
  }
  if (!token.has_value()) {
    return absl::PermissionDeniedError("SecretKeyAccess is required");
  }
  absl::StatusOr<EcdsaPrivateKeyStruct> proto_key =
      EcdsaPrivateKeyStruct::GetParser().Parse(SecretDataAsStringView(
          serialization.SerializedKeyProto().Get(*token)));
  if (!proto_key.ok()) {
    return absl::InvalidArgumentError("Failed to parse EcdsaPrivateKey proto");
  }
  if (proto_key->version != 0) {
    return absl::InvalidArgumentError("Only version 0 keys are accepted.");
  }
  if (proto_key->public_key.version != 0) {
    return absl::InvalidArgumentError(
        "Only version 0 public keys are accepted.");
  }

  OutputPrefixTypeEnum output_prefix_type =
      serialization.GetOutputPrefixTypeEnum();

  absl::StatusOr<EcdsaParameters::Variant> variant =
      ToVariant(output_prefix_type);
  if (!variant.ok()) {
    return variant.status();
  }

  absl::StatusOr<EcdsaParameters> parameters =
      ToParameters(output_prefix_type, proto_key->public_key.params);
  if (!parameters.ok()) {
    return parameters.status();
  }

  EcPoint public_point(BigInteger(proto_key->public_key.x),
                       BigInteger(proto_key->public_key.y));
  absl::StatusOr<EcdsaPublicKey> public_key = EcdsaPublicKey::Create(
      *parameters, public_point, serialization.IdRequirement(),
      GetPartialKeyAccess());

  RestrictedBigInteger private_key_value =
      RestrictedBigInteger(proto_key->key_value, *token);
  return EcdsaPrivateKey::Create(*public_key, private_key_value,
                                 GetPartialKeyAccess());
}

absl::StatusOr<ProtoParametersSerialization> SerializeParameters(
    const EcdsaParameters& parameters) {
  absl::StatusOr<OutputPrefixTypeEnum> output_prefix_type =
      ToOutputPrefixType(parameters.GetVariant());
  if (!output_prefix_type.ok()) {
    return output_prefix_type.status();
  }

  absl::StatusOr<EcdsaParamsStruct> params = FromParameters(parameters);
  if (!params.ok()) {
    return params.status();
  }

  EcdsaKeyFormatStruct proto_key_format;
  proto_key_format.params = *params;
  proto_key_format.version = 0;

  absl::StatusOr<std::string> serialized_proto =
      EcdsaKeyFormatStruct::GetParser().SerializeIntoString(proto_key_format);
  if (!serialized_proto.ok()) {
    return serialized_proto.status();
  }

  return ProtoParametersSerialization::Create(
      kPrivateTypeUrl, *output_prefix_type, *serialized_proto);
}

absl::StatusOr<ProtoKeySerialization> SerializePublicKey(
    const EcdsaPublicKey& key, absl::optional<SecretKeyAccessToken> token) {
  absl::StatusOr<EcdsaParamsStruct> params =
      FromParameters(key.GetParameters());
  if (!params.ok()) {
    return params.status();
  }

  absl::StatusOr<int> enc_length =
      getEncodingLength(key.GetParameters().GetCurveType());
  if (!enc_length.ok()) {
    return enc_length.status();
  }

  absl::StatusOr<std::string> x = GetValueOfFixedLength(
      key.GetPublicPoint(GetPartialKeyAccess()).GetX().GetValue(),
      enc_length.value());
  if (!x.ok()) {
    return x.status();
  }

  absl::StatusOr<std::string> y = GetValueOfFixedLength(
      key.GetPublicPoint(GetPartialKeyAccess()).GetY().GetValue(),
      enc_length.value());
  if (!y.ok()) {
    return y.status();
  }

  EcdsaPublicKeyStruct proto_key;
  proto_key.version = 0;
  proto_key.params = *params;
  proto_key.x = *x;
  proto_key.y = *y;

  absl::StatusOr<std::string> serialized_proto =
      EcdsaPublicKeyStruct::GetParser().SerializeIntoString(proto_key);
  if (!serialized_proto.ok()) {
    return serialized_proto.status();
  }

  absl::StatusOr<OutputPrefixTypeEnum> output_prefix_type =
      ToOutputPrefixType(key.GetParameters().GetVariant());
  if (!output_prefix_type.ok()) {
    return output_prefix_type.status();
  }

  RestrictedData restricted_output =
      RestrictedData(*serialized_proto, InsecureSecretKeyAccess::Get());
  return ProtoKeySerialization::Create(
      kPublicTypeUrl, restricted_output, KeyMaterialTypeEnum::kAsymmetricPublic,
      *output_prefix_type, key.GetIdRequirement());
}

absl::StatusOr<ProtoKeySerialization> SerializePrivateKey(
    const EcdsaPrivateKey& key, absl::optional<SecretKeyAccessToken> token) {
  absl::StatusOr<RestrictedBigInteger> restricted_input =
      key.GetPrivateKeyValue(GetPartialKeyAccess());
  if (!restricted_input.ok()) {
    return restricted_input.status();
  }
  if (!token.has_value()) {
    return absl::PermissionDeniedError("SecretKeyAccess is required");
  }

  absl::StatusOr<EcdsaParamsStruct> params =
      FromParameters(key.GetPublicKey().GetParameters());
  if (!params.ok()) {
    return params.status();
  }

  absl::StatusOr<int> enc_length =
      getEncodingLength(key.GetPublicKey().GetParameters().GetCurveType());
  if (!enc_length.ok()) {
    return enc_length.status();
  }

  absl::StatusOr<std::string> x =
      GetValueOfFixedLength(key.GetPublicKey()
                                .GetPublicPoint(GetPartialKeyAccess())
                                .GetX()
                                .GetValue(),
                            enc_length.value());
  if (!x.ok()) {
    return x.status();
  }

  absl::StatusOr<std::string> y =
      GetValueOfFixedLength(key.GetPublicKey()
                                .GetPublicPoint(GetPartialKeyAccess())
                                .GetY()
                                .GetValue(),
                            enc_length.value());
  if (!y.ok()) {
    return y.status();
  }

  EcdsaPrivateKeyStruct proto_private_key;
  proto_private_key.version = 0;
  proto_private_key.public_key.version = 0;
  proto_private_key.public_key.params = *params;
  proto_private_key.public_key.x = *x;
  proto_private_key.public_key.y = *y;
  absl::StatusOr<SecretData> fixed_length_key =
      GetSecretValueOfFixedLength(*restricted_input, *enc_length, *token);
  if (!fixed_length_key.ok()) {
    return fixed_length_key.status();
  }
  proto_private_key.key_value = *fixed_length_key;

  absl::StatusOr<OutputPrefixTypeEnum> output_prefix_type =
      ToOutputPrefixType(key.GetPublicKey().GetParameters().GetVariant());
  if (!output_prefix_type.ok()) {
    return output_prefix_type.status();
  }

  absl::StatusOr<SecretData> serialized_proto =
      EcdsaPrivateKeyStruct::GetParser().SerializeIntoSecretData(
          proto_private_key);
  if (!serialized_proto.ok()) {
    return serialized_proto.status();
  }
  return ProtoKeySerialization::Create(
      kPrivateTypeUrl, RestrictedData(*serialized_proto, *token),
      KeyMaterialTypeEnum::kAsymmetricPrivate, *output_prefix_type,
      key.GetIdRequirement());
}

EcdsaProtoParametersParserImpl& EcdsaProtoParametersParser() {
  static auto* parser =
      new EcdsaProtoParametersParserImpl(kPrivateTypeUrl, ParseParameters);
  return *parser;
}

EcdsaProtoParametersSerializerImpl& EcdsaProtoParametersSerializer() {
  static auto* serializer = new EcdsaProtoParametersSerializerImpl(
      kPrivateTypeUrl, SerializeParameters);
  return *serializer;
}

EcdsaProtoPublicKeyParserImpl& EcdsaProtoPublicKeyParser() {
  static auto* parser =
      new EcdsaProtoPublicKeyParserImpl(kPublicTypeUrl, ParsePublicKey);
  return *parser;
}

EcdsaProtoPublicKeySerializerImpl& EcdsaProtoPublicKeySerializer() {
  static auto* serializer =
      new EcdsaProtoPublicKeySerializerImpl(SerializePublicKey);
  return *serializer;
}

EcdsaProtoPrivateKeyParserImpl& EcdsaProtoPrivateKeyParser() {
  static auto* parser =
      new EcdsaProtoPrivateKeyParserImpl(kPrivateTypeUrl, ParsePrivateKey);
  return *parser;
}

EcdsaProtoPrivateKeySerializerImpl& EcdsaProtoPrivateKeySerializer() {
  static auto* serializer =
      new EcdsaProtoPrivateKeySerializerImpl(SerializePrivateKey);
  return *serializer;
}
}  // namespace

absl::Status RegisterEcdsaProtoSerializationWithMutableRegistry(
    MutableSerializationRegistry& registry) {
  absl::Status status =
      registry.RegisterParametersParser(&EcdsaProtoParametersParser());
  if (!status.ok()) {
    return status;
  }

  status =
      registry.RegisterParametersSerializer(&EcdsaProtoParametersSerializer());
  if (!status.ok()) {
    return status;
  }

  status = registry.RegisterKeyParser(&EcdsaProtoPublicKeyParser());
  if (!status.ok()) {
    return status;
  }

  status = registry.RegisterKeySerializer(&EcdsaProtoPublicKeySerializer());
  if (!status.ok()) {
    return status;
  }

  status = registry.RegisterKeyParser(&EcdsaProtoPrivateKeyParser());
  if (!status.ok()) {
    return status;
  }

  return registry.RegisterKeySerializer(&EcdsaProtoPrivateKeySerializer());
}

absl::Status RegisterEcdsaProtoSerializationWithRegistryBuilder(
    SerializationRegistry::Builder& builder) {
  absl::Status status =
      builder.RegisterParametersParser(&EcdsaProtoParametersParser());
  if (!status.ok()) {
    return status;
  }

  status =
      builder.RegisterParametersSerializer(&EcdsaProtoParametersSerializer());
  if (!status.ok()) {
    return status;
  }

  status = builder.RegisterKeyParser(&EcdsaProtoPublicKeyParser());
  if (!status.ok()) {
    return status;
  }

  status = builder.RegisterKeySerializer(&EcdsaProtoPublicKeySerializer());
  if (!status.ok()) {
    return status;
  }

  status = builder.RegisterKeyParser(&EcdsaProtoPrivateKeyParser());
  if (!status.ok()) {
    return status;
  }

  return builder.RegisterKeySerializer(&EcdsaProtoPrivateKeySerializer());
}

}  // namespace internal
}  // namespace tink
}  // namespace crypto
