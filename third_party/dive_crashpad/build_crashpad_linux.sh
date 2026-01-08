#!/bin/bash

set -ex

readonly DEPOT_TOOLS_URL=https://chromium.googlesource.com/chromium/tools/depot_tools

if [[ ! -e $1 ]]; then
  echo "Commit hash file not found"
  exit 1
fi
readonly COMMIT_HASH_FILE="$1"
readonly COMMIT_HASH="$(cat <"${COMMIT_HASH_FILE}")"

if [[ ! -e depot_tools ]]; then
  git clone "${DEPOT_TOOLS_URL}" depot_tools
fi

export PATH="$(pwd)/depot_tools:${PATH}"
if [[ ! -e crashpad ]]; then
  fetch crashpad
fi

pushd crashpad
git fetch origin "${COMMIT_HASH}"
git checkout "${COMMIT_HASH}"
gclient sync
gn gen out/Default
ninja -C out/Default
popd

cp -f \
  crashpad/out/Default/crashpad_handler \
  crashpad/out/Default/obj/third_party/mini_chromium/mini_chromium/base/libbase.a \
  crashpad/out/Default/obj/client/libcommon.a \
  crashpad/out/Default/obj/client/libclient.a \
  crashpad/out/Default/obj/util/libutil.a \
  .
