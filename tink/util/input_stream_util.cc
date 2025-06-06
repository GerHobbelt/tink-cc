// Copyright 2019 Google LLC
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

#include "tink/util/input_stream_util.h"

#include <algorithm>
#include <string>
#include <utility>

#include "absl/algorithm/container.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "tink/input_stream.h"
#include "tink/internal/secret_buffer.h"
#include "tink/util/secret_data.h"
#include "tink/util/statusor.h"

namespace crypto {
namespace tink {

using ::crypto::tink::internal::SecretBuffer;

namespace {

template <typename Result>
absl::StatusOr<Result> ReadBytesFromStreamImpl(int num_bytes,
                                               InputStream* input_stream) {
  const void* buffer;
  Result result;
  if (num_bytes > 0) {
    result.resize(num_bytes);
  }
  int num_bytes_read = 0;

  while (num_bytes_read < num_bytes) {
    auto next_result = input_stream->Next(&buffer);
    if (!next_result.ok()) return next_result.status();

    int num_bytes_in_chunk = next_result.value();
    int num_bytes_to_copy =
        std::min(num_bytes - num_bytes_read, num_bytes_in_chunk);
    absl::c_copy(absl::MakeSpan(reinterpret_cast<const char*>(buffer),
                                num_bytes_to_copy),
                 result.data() + num_bytes_read);
    input_stream->BackUp(num_bytes_in_chunk - num_bytes_to_copy);
    num_bytes_read += num_bytes_to_copy;
  }
  return result;
}
}  // namespace

absl::StatusOr<std::string> ReadBytesFromStream(int num_bytes,
                                                InputStream* input_stream) {
  return ReadBytesFromStreamImpl<std::string>(num_bytes, input_stream);
}

absl::StatusOr<SecretData> ReadSecretBytesFromStream(
    int num_bytes, InputStream* input_stream) {
  absl::StatusOr<SecretBuffer> result =
      ReadBytesFromStreamImpl<SecretBuffer>(num_bytes, input_stream);
  if (!result.ok()) { return result.status(); }
  return util::internal::AsSecretData(*std::move(result));
}

}  // namespace tink
}  // namespace crypto
