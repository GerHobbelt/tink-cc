// Copyright 2024 Google LLC
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

#ifndef TINK_JWT_JWT_MAC_KEY_GEN_CONFIG_V0_H_
#define TINK_JWT_JWT_MAC_KEY_GEN_CONFIG_V0_H_

#include "tink/key_gen_configuration.h"

namespace crypto {
namespace tink {

// KeyGenConfiguration used to generate JWT MAC keys with recommended key
// managers.
const KeyGenConfiguration& KeyGenConfigJwtMacV0();

}  // namespace tink
}  // namespace crypto

#endif  // TINK_JWT_JWT_MAC_KEY_GEN_CONFIG_V0_H_
