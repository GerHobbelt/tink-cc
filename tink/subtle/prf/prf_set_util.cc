// Copyright 2020 Google LLC
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
#include "tink/subtle/prf/prf_set_util.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "tink/internal/dfsan_forwarders.h"
#include "tink/mac/internal/stateful_mac.h"
#include "tink/prf/prf_set.h"
#include "tink/subtle/prf/streaming_prf.h"
#include "tink/util/input_stream_util.h"
#include "tink/util/secret_data.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"

namespace crypto {
namespace tink {
namespace subtle {
namespace {

class PrfFromStreamingPrf : public Prf {
 public:
  explicit PrfFromStreamingPrf(std::unique_ptr<StreamingPrf> streaming_prf)
      : streaming_prf_(std::move(streaming_prf)) {}
  absl::StatusOr<std::string> Compute(absl::string_view input,
                                      size_t output_length) const override {
    auto inputstream = streaming_prf_->ComputePrf(input);
    auto output_result = ReadBytesFromStream(output_length, inputstream.get());
    if (!output_result.ok()) {
      return output_result.status();
    }
    std::string output = output_result.value();
    return output;
  }

 private:
  std::unique_ptr<StreamingPrf> streaming_prf_;
};

class PrfFromStatefulMacFactory : public Prf {
 public:
  explicit PrfFromStatefulMacFactory(
      std::unique_ptr<internal::StatefulMacFactory> stateful_mac_factory)
      : stateful_mac_factory_(std::move(stateful_mac_factory)) {}
  absl::StatusOr<std::string> Compute(absl::string_view input,
                                      size_t output_length) const override {
    auto stateful_mac_result = stateful_mac_factory_->Create();
    if (!stateful_mac_result.ok()) {
      return stateful_mac_result.status();
    }
    auto stateful_mac = std::move(stateful_mac_result.value());
    auto status = stateful_mac->Update(input);
    if (!status.ok()) {
      return status;
    }
    absl::StatusOr<SecretData> output = stateful_mac->FinalizeAsSecretData();
    if (!output.ok()) {
      return output.status();
    }
    // Clear the label on output -- this can now be given out.
    internal::DfsanClearLabel(output->data(), output->size());
    if (output->size() < output_length) {
      return absl::Status(
          absl::StatusCode::kInvalidArgument,
          absl::StrCat("PRF only supports outputs up to ", output->size(),
                       " bytes, but ", output_length, " bytes were requested"));
    }
    return std::string(
        util::SecretDataAsStringView(*output).substr(0, output_length));
  }

 private:
  std::unique_ptr<internal::StatefulMacFactory> stateful_mac_factory_;
};

}  // namespace

std::unique_ptr<Prf> CreatePrfFromStreamingPrf(
    std::unique_ptr<StreamingPrf> streaming_prf) {
  return absl::make_unique<PrfFromStreamingPrf>(std::move(streaming_prf));
}

std::unique_ptr<Prf> CreatePrfFromStatefulMacFactory(
    std::unique_ptr<internal::StatefulMacFactory> stateful_mac_factory) {
  return absl::make_unique<PrfFromStatefulMacFactory>(
      std::move(stateful_mac_factory));
}

}  // namespace subtle
}  // namespace tink
}  // namespace crypto
