#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source_dir=vendor/vkshade-original-fx
build_dir=build/vkshade-original-fx
revision=1b65208c01ff0a4d3592d4617911d111c5fe4694
if [[ ! -d "$source_dir/.git" ]]; then
  git clone https://github.com/slobodaapl/vkShade.git "$source_dir"
  git -C "$source_dir" checkout "$revision"
fi
[[ $(git -C "$source_dir" rev-parse HEAD) == "$revision" ]] || { echo 'Unexpected vkShade revision; refusing to patch.' >&2; exit 1; }
patch_path="$PWD/rewired-visuals/runtime/vkshade-compat.patch"
if git -C "$source_dir" apply --check "$patch_path" 2>/dev/null; then
  git -C "$source_dir" apply "$patch_path"
elif ! git -C "$source_dir" apply --reverse --check "$patch_path" 2>/dev/null; then
  echo 'vkShade has incompatible local changes; inspect before building.' >&2; exit 1
fi
meson setup "$build_dir" "$source_dir" --buildtype=release -Dproton_bridge=disabled
ninja -C "$build_dir" -j4 src/libvkshade.so src/device-features-test
"$build_dir/src/device-features-test"
