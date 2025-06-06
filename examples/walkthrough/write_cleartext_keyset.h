// Copyright 2022 Google LLC
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
#ifndef TINK_EXAMPLES_WALKTHROUGH_WRITE_CLEARTEXT_KEYSET_H_
#define TINK_EXAMPLES_WALKTHROUGH_WRITE_CLEARTEXT_KEYSET_H_

#include <memory>
#include <ostream>

#include "absl/status/status.h"
#include "tink/keyset_handle.h"

namespace tink_walkthrough {

// Writes a `keyset` to `output_stream` as a plaintext JSON format.
//
// Warning: Storing keys in cleartext is not recommended. We recommend using a
// Key Management Service to protect your keys. See
// https://developers.google.com/tink/key-management-overview.
absl::Status WriteKeyset(const crypto::tink::KeysetHandle& keyset,
                         std::unique_ptr<std::ostream> output_stream);

}  // namespace tink_walkthrough

#endif  // TINK_EXAMPLES_WALKTHROUGH_WRITE_CLEARTEXT_KEYSET_H_
