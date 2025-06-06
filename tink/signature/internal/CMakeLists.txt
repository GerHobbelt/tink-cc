tink_module(signature::internal)

add_subdirectory(testing)

tink_cc_library(
  NAME ecdsa_raw_sign_boringssl
  SRCS
    ecdsa_raw_sign_boringssl.cc
    ecdsa_raw_sign_boringssl.h
  DEPS
    absl::memory
    absl::status
    absl::strings
    crypto
    tink::core::public_key_sign
    tink::internal::bn_util
    tink::internal::call_with_core_dump_protection
    tink::internal::dfsan_forwarders
    tink::internal::ec_util
    tink::internal::err_util
    tink::internal::fips_utils
    tink::internal::md_util
    tink::internal::ssl_unique_ptr
    tink::internal::util
    tink::subtle::common_enums
    tink::subtle::subtle_util_boringssl
    tink::util::errors
    tink::util::status
    tink::util::statusor
)

tink_cc_library(
  NAME config_v0
  SRCS
    config_v0.cc
    config_v0.h
  DEPS
    tink::signature::internal::ml_dsa_proto_serialization
    tink::signature::internal::ml_dsa_sign_boringssl
    tink::signature::internal::ml_dsa_verify_boringssl
    tink::signature::internal::slh_dsa_proto_serialization
    tink::signature::internal::slh_dsa_sign_boringssl
    tink::signature::internal::slh_dsa_verify_boringssl
    absl::memory
    tink::core::configuration
    tink::core::public_key_sign
    tink::core::public_key_verify
    tink::internal::configuration_impl
    tink::signature::ecdsa_private_key
    tink::signature::ecdsa_proto_serialization
    tink::signature::ecdsa_public_key
    tink::signature::ecdsa_sign_key_manager
    tink::signature::ecdsa_verify_key_manager
    tink::signature::ed25519_private_key
    tink::signature::ed25519_proto_serialization
    tink::signature::ed25519_public_key
    tink::signature::ed25519_sign_key_manager
    tink::signature::ed25519_verify_key_manager
    tink::signature::ml_dsa_private_key
    tink::signature::ml_dsa_public_key
    tink::signature::public_key_sign_wrapper
    tink::signature::public_key_verify_wrapper
    tink::signature::rsa_ssa_pkcs1_private_key
    tink::signature::rsa_ssa_pkcs1_proto_serialization
    tink::signature::rsa_ssa_pkcs1_public_key
    tink::signature::rsa_ssa_pkcs1_sign_key_manager
    tink::signature::rsa_ssa_pkcs1_verify_key_manager
    tink::signature::rsa_ssa_pss_private_key
    tink::signature::rsa_ssa_pss_proto_serialization
    tink::signature::rsa_ssa_pss_public_key
    tink::signature::rsa_ssa_pss_sign_key_manager
    tink::signature::rsa_ssa_pss_verify_key_manager
    tink::signature::slh_dsa_private_key
    tink::signature::slh_dsa_public_key
    tink::subtle::ecdsa_sign_boringssl
    tink::subtle::ecdsa_verify_boringssl
    tink::subtle::ed25519_sign_boringssl
    tink::subtle::ed25519_verify_boringssl
    tink::subtle::rsa_ssa_pkcs1_sign_boringssl
    tink::subtle::rsa_ssa_pkcs1_verify_boringssl
    tink::subtle::rsa_ssa_pss_sign_boringssl
    tink::subtle::rsa_ssa_pss_verify_boringssl
    tink::util::status
    tink::util::statusor
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME config_v0_openssl
  SRCS
    config_v0.cc
    config_v0.h
  DEPS
    absl::memory
    tink::core::configuration
    tink::core::public_key_sign
    tink::core::public_key_verify
    tink::internal::configuration_impl
    tink::signature::ecdsa_private_key
    tink::signature::ecdsa_proto_serialization
    tink::signature::ecdsa_public_key
    tink::signature::ecdsa_sign_key_manager
    tink::signature::ecdsa_verify_key_manager
    tink::signature::ed25519_private_key
    tink::signature::ed25519_proto_serialization
    tink::signature::ed25519_public_key
    tink::signature::ed25519_sign_key_manager
    tink::signature::ed25519_verify_key_manager
    tink::signature::public_key_sign_wrapper
    tink::signature::public_key_verify_wrapper
    tink::signature::rsa_ssa_pkcs1_private_key
    tink::signature::rsa_ssa_pkcs1_proto_serialization
    tink::signature::rsa_ssa_pkcs1_public_key
    tink::signature::rsa_ssa_pkcs1_sign_key_manager
    tink::signature::rsa_ssa_pkcs1_verify_key_manager
    tink::signature::rsa_ssa_pss_private_key
    tink::signature::rsa_ssa_pss_proto_serialization
    tink::signature::rsa_ssa_pss_public_key
    tink::signature::rsa_ssa_pss_sign_key_manager
    tink::signature::rsa_ssa_pss_verify_key_manager
    tink::subtle::ecdsa_sign_boringssl
    tink::subtle::ecdsa_verify_boringssl
    tink::subtle::ed25519_sign_boringssl
    tink::subtle::ed25519_verify_boringssl
    tink::subtle::rsa_ssa_pkcs1_sign_boringssl
    tink::subtle::rsa_ssa_pkcs1_verify_boringssl
    tink::subtle::rsa_ssa_pss_sign_boringssl
    tink::subtle::rsa_ssa_pss_verify_boringssl
    tink::util::status
    tink::util::statusor
  TAGS
    exclude_if_boringssl
)

tink_cc_library(
  NAME key_gen_config_v0
  SRCS
    key_gen_config_v0.cc
    key_gen_config_v0.h
  DEPS
    tink::signature::internal::key_creators
    tink::signature::internal::ml_dsa_proto_serialization
    tink::signature::internal::slh_dsa_proto_serialization
    absl::memory
    tink::core::key_gen_configuration
    tink::internal::key_gen_configuration_impl
    tink::signature::ecdsa_proto_serialization
    tink::signature::ecdsa_sign_key_manager
    tink::signature::ecdsa_verify_key_manager
    tink::signature::ed25519_proto_serialization
    tink::signature::ed25519_sign_key_manager
    tink::signature::ed25519_verify_key_manager
    tink::signature::ml_dsa_parameters
    tink::signature::rsa_ssa_pkcs1_proto_serialization
    tink::signature::rsa_ssa_pkcs1_sign_key_manager
    tink::signature::rsa_ssa_pkcs1_verify_key_manager
    tink::signature::rsa_ssa_pss_proto_serialization
    tink::signature::rsa_ssa_pss_sign_key_manager
    tink::signature::rsa_ssa_pss_verify_key_manager
    tink::signature::slh_dsa_parameters
    tink::util::status
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME key_gen_config_v0_openssl
  SRCS
    key_gen_config_v0.cc
    key_gen_config_v0.h
  DEPS
    absl::memory
    tink::core::key_gen_configuration
    tink::internal::key_gen_configuration_impl
    tink::signature::ecdsa_sign_key_manager
    tink::signature::ecdsa_verify_key_manager
    tink::signature::ed25519_sign_key_manager
    tink::signature::ed25519_verify_key_manager
    tink::signature::rsa_ssa_pkcs1_sign_key_manager
    tink::signature::rsa_ssa_pkcs1_verify_key_manager
    tink::signature::rsa_ssa_pss_sign_key_manager
    tink::signature::rsa_ssa_pss_verify_key_manager
    tink::util::status
  TAGS
    exclude_if_boringssl
)

tink_cc_library(
  NAME config_fips_140_2
  SRCS
    config_fips_140_2.cc
    config_fips_140_2.h
  DEPS
    absl::memory
    absl::status
    tink::core::configuration
    tink::internal::configuration_impl
    tink::internal::fips_utils
    tink::signature::ecdsa_sign_key_manager
    tink::signature::ecdsa_verify_key_manager
    tink::signature::public_key_sign_wrapper
    tink::signature::public_key_verify_wrapper
    tink::signature::rsa_ssa_pkcs1_sign_key_manager
    tink::signature::rsa_ssa_pkcs1_verify_key_manager
    tink::signature::rsa_ssa_pss_sign_key_manager
    tink::signature::rsa_ssa_pss_verify_key_manager
    tink::util::status
)

tink_cc_test(
  NAME ecdsa_raw_sign_boringssl_test
  SRCS
    ecdsa_raw_sign_boringssl_test.cc
  DEPS
    tink::signature::internal::ecdsa_raw_sign_boringssl
    gmock
    absl::status
    absl::string_view
    tink::core::public_key_sign
    tink::core::public_key_verify
    tink::internal::ec_util
    tink::internal::fips_utils
    tink::internal::md_util
    tink::subtle::common_enums
    tink::subtle::ecdsa_verify_boringssl
    tink::subtle::subtle_util_boringssl
    tink::util::status
    tink::util::statusor
    tink::util::test_matchers
)

tink_cc_test(
  NAME config_v0_openssl_test
  SRCS
    config_v0_test.cc
  DEPS
    tink::signature::internal::config_v0_openssl
    tink::signature::internal::key_gen_config_v0_openssl
    gmock
    absl::check
    tink::core::configuration
    tink::core::key_gen_configuration
    tink::core::key_status
    tink::core::keyset_handle
    tink::core::public_key_sign
    tink::core::public_key_verify
    tink::internal::configuration_impl
    tink::internal::key_gen_configuration_impl
    tink::internal::key_type_info_store
    tink::internal::keyset_wrapper_store
    tink::signature::ecdsa_verify_key_manager
    tink::signature::ed25519_verify_key_manager
    tink::signature::ml_dsa_parameters
    tink::signature::rsa_ssa_pkcs1_verify_key_manager
    tink::signature::rsa_ssa_pss_verify_key_manager
    tink::signature::signature_key_templates
    tink::signature::slh_dsa_parameters
    tink::signature::internal::testing::ecdsa_test_vectors
    tink::signature::internal::testing::ed25519_test_vectors
    tink::signature::internal::testing::rsa_ssa_pkcs1_test_vectors
    tink::signature::internal::testing::rsa_ssa_pss_test_vectors
    tink::signature::internal::testing::signature_test_vector
    tink::util::statusor
    tink::util::test_matchers
    tink::proto::tink_cc_proto
  TAGS
    exclude_if_boringssl
)

tink_cc_test(
  NAME config_fips_140_2_test
  SRCS
    config_fips_140_2_test.cc
  DEPS
    tink::signature::internal::config_fips_140_2
    tink::signature::internal::key_gen_config_v0
    gmock
    tink::core::configuration
    tink::core::key_gen_configuration
    tink::core::keyset_handle
    tink::core::public_key_sign
    tink::core::public_key_verify
    tink::internal::configuration_impl
    tink::internal::fips_utils
    tink::internal::key_type_info_store
    tink::internal::keyset_wrapper_store
    tink::signature::ecdsa_verify_key_manager
    tink::signature::ed25519_verify_key_manager
    tink::signature::rsa_ssa_pkcs1_verify_key_manager
    tink::signature::rsa_ssa_pss_verify_key_manager
    tink::signature::signature_key_templates
    tink::util::statusor
    tink::util::test_matchers
    tink::proto::tink_cc_proto
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME key_creators
  SRCS
    key_creators.cc
    key_creators.h
  DEPS
    absl::memory
    absl::status
    absl::string_view
    absl::optional
    crypto
    tink::core::big_integer
    tink::core::ec_point
    tink::core::partial_key_access
    tink::core::restricted_big_integer
    tink::core::restricted_data
    tink::internal::ec_util
    tink::internal::internal_insecure_secret_key_access
    tink::internal::secret_buffer
    tink::signature::ecdsa_parameters
    tink::signature::ecdsa_private_key
    tink::signature::ecdsa_public_key
    tink::signature::ml_dsa_parameters
    tink::signature::ml_dsa_private_key
    tink::signature::ml_dsa_public_key
    tink::signature::slh_dsa_parameters
    tink::signature::slh_dsa_private_key
    tink::signature::slh_dsa_public_key
    tink::subtle::common_enums
    tink::util::secret_data
    tink::util::status
    tink::util::statusor
  TAGS
    exclude_if_openssl
)

tink_cc_test(
  NAME key_creators_test
  SRCS
    key_creators_test.cc
  DEPS
    tink::signature::internal::key_creators
    tink::signature::internal::ml_dsa_proto_serialization
    tink::signature::internal::slh_dsa_proto_serialization
    gmock
    absl::optional
    tink::core::key_gen_configuration
    tink::core::key_status
    tink::core::keyset_handle
    tink::internal::key_gen_configuration_impl
    tink::signature::ecdsa_parameters
    tink::signature::ecdsa_private_key
    tink::signature::ecdsa_proto_serialization
    tink::signature::ml_dsa_parameters
    tink::signature::ml_dsa_private_key
    tink::signature::slh_dsa_parameters
    tink::signature::slh_dsa_private_key
    tink::util::statusor
    tink::util::test_matchers
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME ml_dsa_sign_boringssl
  SRCS
    ml_dsa_sign_boringssl.cc
    ml_dsa_sign_boringssl.h
  DEPS
    absl::memory
    absl::status
    absl::statusor
    absl::string_view
    crypto
    tink::core::insecure_secret_key_access
    tink::core::partial_key_access
    tink::core::public_key_sign
    tink::internal::call_with_core_dump_protection
    tink::internal::dfsan_forwarders
    tink::internal::fips_utils
    tink::signature::ml_dsa_parameters
    tink::signature::ml_dsa_private_key
    tink::subtle::subtle_util
    tink::util::secret_data
    tink::util::statusor
  TAGS
    exclude_if_openssl
)

tink_cc_test(
  NAME ml_dsa_sign_boringssl_test
  SRCS
    ml_dsa_sign_boringssl_test.cc
  DEPS
    tink::signature::internal::key_creators
    tink::signature::internal::ml_dsa_sign_boringssl
    gmock
    absl::status
    absl::statusor
    absl::optional
    crypto
    tink::core::public_key_sign
    tink::internal::fips_utils
    tink::signature::ml_dsa_parameters
    tink::signature::ml_dsa_private_key
    tink::util::status
    tink::util::statusor
    tink::util::test_matchers
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME ml_dsa_verify_boringssl
  SRCS
    ml_dsa_verify_boringssl.cc
    ml_dsa_verify_boringssl.h
  DEPS
    absl::status
    absl::strings
    absl::string_view
    crypto
    tink::core::partial_key_access
    tink::core::public_key_verify
    tink::internal::fips_utils
    tink::signature::ml_dsa_parameters
    tink::signature::ml_dsa_public_key
    tink::util::status
    tink::util::statusor
  TAGS
    exclude_if_openssl
)

tink_cc_test(
  NAME ml_dsa_verify_boringssl_test
  SRCS
    ml_dsa_verify_boringssl_test.cc
  DEPS
    tink::signature::internal::key_creators
    tink::signature::internal::ml_dsa_sign_boringssl
    tink::signature::internal::ml_dsa_verify_boringssl
    gmock
    absl::status
    absl::statusor
    absl::string_view
    absl::optional
    tink::core::insecure_secret_key_access
    tink::core::partial_key_access
    tink::core::public_key_sign
    tink::core::public_key_verify
    tink::core::restricted_data
    tink::internal::fips_utils
    tink::signature::ml_dsa_parameters
    tink::signature::ml_dsa_private_key
    tink::signature::ml_dsa_public_key
    tink::util::statusor
    tink::util::test_matchers
    tink::util::test_util
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME ml_dsa_proto_serialization
  SRCS
    ml_dsa_proto_serialization.cc
    ml_dsa_proto_serialization.h
  DEPS
    absl::no_destructor
    absl::status
    absl::statusor
    absl::string_view
    absl::optional
    tink::core::insecure_secret_key_access
    tink::core::partial_key_access
    tink::core::restricted_data
    tink::core::secret_data
    tink::core::secret_key_access_token
    tink::internal::key_parser
    tink::internal::key_serializer
    tink::internal::mutable_serialization_registry
    tink::internal::parameters_parser
    tink::internal::parameters_serializer
    tink::internal::proto_key_serialization
    tink::internal::proto_parameters_serialization
    tink::internal::proto_parser
    tink::internal::tink_proto_structs
    tink::signature::ml_dsa_parameters
    tink::signature::ml_dsa_private_key
    tink::signature::ml_dsa_public_key
    tink::util::secret_data
  TAGS
    exclude_if_openssl
)

tink_cc_test(
  NAME ml_dsa_proto_serialization_test
  SRCS
    ml_dsa_proto_serialization_test.cc
  DEPS
    tink::signature::internal::key_creators
    tink::signature::internal::ml_dsa_proto_serialization
    gmock
    absl::check
    absl::status
    absl::string_view
    absl::optional
    tink::core::insecure_secret_key_access
    tink::core::key
    tink::core::parameters
    tink::core::partial_key_access
    tink::core::restricted_data
    tink::internal::mutable_serialization_registry
    tink::internal::proto_key_serialization
    tink::internal::proto_parameters_serialization
    tink::internal::serialization
    tink::internal::tink_proto_structs
    tink::signature::ml_dsa_parameters
    tink::signature::ml_dsa_private_key
    tink::signature::ml_dsa_public_key
    tink::util::test_matchers
    tink::util::test_util
    tink::proto::ml_dsa_cc_proto
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME slh_dsa_proto_serialization
  SRCS
    slh_dsa_proto_serialization.cc
    slh_dsa_proto_serialization.h
  DEPS
    absl::no_destructor
    absl::status
    absl::statusor
    absl::string_view
    absl::optional
    tink::core::insecure_secret_key_access
    tink::core::partial_key_access
    tink::core::restricted_data
    tink::core::secret_key_access_token
    tink::internal::key_parser
    tink::internal::key_serializer
    tink::internal::mutable_serialization_registry
    tink::internal::parameters_parser
    tink::internal::parameters_serializer
    tink::internal::proto_key_serialization
    tink::internal::proto_parameters_serialization
    tink::internal::proto_parser
    tink::internal::tink_proto_structs
    tink::signature::slh_dsa_parameters
    tink::signature::slh_dsa_private_key
    tink::signature::slh_dsa_public_key
    tink::util::secret_data
  TAGS
    exclude_if_openssl
)

tink_cc_test(
  NAME slh_dsa_proto_serialization_test
  SRCS
    slh_dsa_proto_serialization_test.cc
  DEPS
    tink::signature::internal::slh_dsa_proto_serialization
    gmock
    absl::status
    absl::statusor
    absl::string_view
    absl::optional
    crypto
    tink::core::insecure_secret_key_access
    tink::core::key
    tink::core::parameters
    tink::core::partial_key_access
    tink::core::restricted_data
    tink::internal::mutable_serialization_registry
    tink::internal::proto_key_serialization
    tink::internal::proto_parameters_serialization
    tink::internal::serialization
    tink::internal::tink_proto_structs
    tink::signature::slh_dsa_parameters
    tink::signature::slh_dsa_private_key
    tink::signature::slh_dsa_public_key
    tink::subtle::random
    tink::util::test_matchers
    tink::proto::slh_dsa_cc_proto
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME slh_dsa_sign_boringssl
  SRCS
    slh_dsa_sign_boringssl.cc
    slh_dsa_sign_boringssl.h
  DEPS
    absl::status
    absl::statusor
    absl::string_view
    crypto
    tink::core::insecure_secret_key_access
    tink::core::partial_key_access
    tink::core::public_key_sign
    tink::internal::call_with_core_dump_protection
    tink::internal::dfsan_forwarders
    tink::internal::fips_utils
    tink::signature::slh_dsa_private_key
    tink::subtle::subtle_util
    tink::util::statusor
  TAGS
    exclude_if_openssl
)

tink_cc_test(
  NAME slh_dsa_sign_boringssl_test
  SRCS
    slh_dsa_sign_boringssl_test.cc
  DEPS
    tink::signature::internal::key_creators
    tink::signature::internal::slh_dsa_sign_boringssl
    gmock
    absl::status
    absl::statusor
    absl::optional
    crypto
    tink::core::public_key_sign
    tink::internal::fips_utils
    tink::signature::slh_dsa_parameters
    tink::signature::slh_dsa_private_key
    tink::util::status
    tink::util::statusor
    tink::util::test_matchers
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME slh_dsa_verify_boringssl
  SRCS
    slh_dsa_verify_boringssl.cc
    slh_dsa_verify_boringssl.h
  DEPS
    absl::status
    absl::strings
    absl::string_view
    crypto
    tink::core::partial_key_access
    tink::core::public_key_verify
    tink::internal::fips_utils
    tink::signature::slh_dsa_public_key
    tink::util::status
    tink::util::statusor
  TAGS
    exclude_if_openssl
)

tink_cc_test(
  NAME slh_dsa_verify_boringssl_test
  SRCS
    slh_dsa_verify_boringssl_test.cc
  DEPS
    tink::signature::internal::key_creators
    tink::signature::internal::slh_dsa_sign_boringssl
    tink::signature::internal::slh_dsa_verify_boringssl
    gmock
    absl::status
    absl::statusor
    absl::string_view
    absl::optional
    crypto
    tink::core::insecure_secret_key_access
    tink::core::partial_key_access
    tink::core::public_key_sign
    tink::core::public_key_verify
    tink::core::restricted_data
    tink::internal::fips_utils
    tink::signature::slh_dsa_parameters
    tink::signature::slh_dsa_private_key
    tink::signature::slh_dsa_public_key
    tink::util::status
    tink::util::statusor
    tink::util::test_matchers
    tink::util::test_util
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME ed25519_proto_serialization_impl
  SRCS
    ed25519_proto_serialization_impl.cc
    ed25519_proto_serialization_impl.h
  DEPS
    absl::no_destructor
    absl::status
    absl::statusor
    absl::string_view
    absl::optional
    tink::core::insecure_secret_key_access
    tink::core::partial_key_access
    tink::core::restricted_data
    tink::core::secret_data
    tink::core::secret_key_access_token
    tink::internal::key_parser
    tink::internal::key_serializer
    tink::internal::mutable_serialization_registry
    tink::internal::parameters_parser
    tink::internal::parameters_serializer
    tink::internal::proto_key_serialization
    tink::internal::proto_parameters_serialization
    tink::internal::proto_parser
    tink::internal::serialization_registry
    tink::internal::tink_proto_structs
    tink::signature::ed25519_parameters
    tink::signature::ed25519_private_key
    tink::signature::ed25519_public_key
    tink::util::secret_data
)

tink_cc_test(
  NAME ed25519_proto_serialization_impl_test
  SRCS
    ed25519_proto_serialization_impl_test.cc
  DEPS
    tink::signature::internal::ed25519_proto_serialization_impl
    gmock
    absl::status
    absl::statusor
    absl::string_view
    absl::optional
    tink::core::insecure_secret_key_access_testonly
    tink::core::key
    tink::core::parameters
    tink::core::partial_key_access
    tink::core::restricted_data
    tink::internal::ec_util
    tink::internal::mutable_serialization_registry
    tink::internal::proto_key_serialization
    tink::internal::proto_parameters_serialization
    tink::internal::serialization
    tink::internal::serialization_registry
    tink::internal::tink_proto_structs
    tink::signature::ed25519_parameters
    tink::signature::ed25519_private_key
    tink::signature::ed25519_public_key
    tink::subtle::random
    tink::util::secret_data
    tink::util::test_matchers
    tink::proto::ed25519_cc_proto
)

tink_cc_test(
  NAME key_gen_config_v0_test
  SRCS
    key_gen_config_v0_test.cc
  DEPS
    tink::signature::internal::key_gen_config_v0
    gmock
    tink::core::key_gen_configuration
    tink::core::key_status
    tink::core::keyset_handle
    tink::signature::ml_dsa_parameters
    tink::signature::slh_dsa_parameters
    tink::util::statusor
    tink::util::test_matchers
    tink::proto::tink_cc_proto
  TAGS
    exclude_if_openssl
)

tink_cc_test(
  NAME config_v0_test
  SRCS
    config_v0_test.cc
  DEPS
    tink::signature::internal::config_v0
    tink::signature::internal::key_gen_config_v0
    gmock
    absl::check
    tink::core::configuration
    tink::core::key_gen_configuration
    tink::core::key_status
    tink::core::keyset_handle
    tink::core::public_key_sign
    tink::core::public_key_verify
    tink::internal::configuration_impl
    tink::internal::key_gen_configuration_impl
    tink::internal::key_type_info_store
    tink::internal::keyset_wrapper_store
    tink::signature::ecdsa_verify_key_manager
    tink::signature::ed25519_verify_key_manager
    tink::signature::ml_dsa_parameters
    tink::signature::rsa_ssa_pkcs1_verify_key_manager
    tink::signature::rsa_ssa_pss_verify_key_manager
    tink::signature::signature_key_templates
    tink::signature::slh_dsa_parameters
    tink::signature::internal::testing::ecdsa_test_vectors
    tink::signature::internal::testing::ed25519_test_vectors
    tink::signature::internal::testing::ml_dsa_test_vectors
    tink::signature::internal::testing::rsa_ssa_pkcs1_test_vectors
    tink::signature::internal::testing::rsa_ssa_pss_test_vectors
    tink::signature::internal::testing::signature_test_vector
    tink::util::statusor
    tink::util::test_matchers
    tink::proto::tink_cc_proto
  TAGS
    exclude_if_openssl
)

tink_cc_library(
  NAME ecdsa_proto_serialization_impl
  SRCS
    ecdsa_proto_serialization_impl.cc
    ecdsa_proto_serialization_impl.h
  DEPS
    absl::no_destructor
    absl::status
    absl::statusor
    absl::string_view
    absl::optional
    tink::core::big_integer
    tink::core::ec_point
    tink::core::insecure_secret_key_access
    tink::core::partial_key_access
    tink::core::restricted_big_integer
    tink::core::restricted_data
    tink::core::secret_key_access_token
    tink::internal::bn_encoding_util
    tink::internal::common_proto_enums
    tink::internal::key_parser
    tink::internal::key_serializer
    tink::internal::mutable_serialization_registry
    tink::internal::parameters_parser
    tink::internal::parameters_serializer
    tink::internal::proto_key_serialization
    tink::internal::proto_parameters_serialization
    tink::internal::proto_parser
    tink::internal::serialization_registry
    tink::internal::tink_proto_structs
    tink::signature::ecdsa_parameters
    tink::signature::ecdsa_private_key
    tink::signature::ecdsa_public_key
    tink::util::secret_data
)

tink_cc_test(
  NAME ecdsa_proto_serialization_impl_test
  SRCS
    ecdsa_proto_serialization_impl_test.cc
  DEPS
    tink::signature::internal::ecdsa_proto_serialization_impl
    gmock
    absl::status
    absl::statusor
    absl::string_view
    absl::optional
    crypto
    tink::core::big_integer
    tink::core::ec_point
    tink::core::insecure_secret_key_access_testonly
    tink::core::key
    tink::core::parameters
    tink::core::partial_key_access
    tink::core::restricted_big_integer
    tink::core::restricted_data
    tink::internal::ec_util
    tink::internal::mutable_serialization_registry
    tink::internal::proto_key_serialization
    tink::internal::proto_parameters_serialization
    tink::internal::serialization
    tink::internal::serialization_registry
    tink::internal::tink_proto_structs
    tink::signature::ecdsa_parameters
    tink::signature::ecdsa_private_key
    tink::signature::ecdsa_public_key
    tink::subtle::common_enums
    tink::util::secret_data
    tink::util::test_matchers
    tink::proto::common_cc_proto
    tink::proto::ecdsa_cc_proto
)
