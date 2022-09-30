#!/bin/bash
# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################

set -euo pipefail

readonly WORKSPACE_FOLDER="examples"

# If we are running on Kokoro cd into the repository.
if [[ -n "${KOKORO_ARTIFACTS_DIR:-}" ]]; then
  TINK_BASE_DIR="$(echo "${KOKORO_ARTIFACTS_DIR}"/git*)"
  cd "${TINK_BASE_DIR}/tink_cc"
  chmod +x "${KOKORO_GFILE_DIR}/use_bazel.sh"
  "${KOKORO_GFILE_DIR}/use_bazel.sh" "$(cat ${WORKSPACE_FOLDER}/.bazelversion)"
fi

: "${TINK_BASE_DIR:=$(cd .. && pwd)}"

# Sourcing required to update callers environment.
source ./kokoro/testutils/install_python3.sh

cp "${WORKSPACE_FOLDER}/WORKSPACE" "${WORKSPACE_FOLDER}/WORKSPACE.bak"
./kokoro/testutils/replace_http_archive_with_local_repository.py \
  -f "${WORKSPACE_FOLDER}/WORKSPACE" \
  -t "${TINK_BASE_DIR}"
./kokoro/testutils/run_bazel_tests.sh "${WORKSPACE_FOLDER}"
mv "${WORKSPACE_FOLDER}/WORKSPACE.bak" "${WORKSPACE_FOLDER}/WORKSPACE"
