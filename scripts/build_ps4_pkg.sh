#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VENDOR="$ROOT/vendor/ps4torrent"

if [[ -z "${PS4_PAYLOAD_SDK:-}" ]]; then
  echo "ERROR: PS4_PAYLOAD_SDK is not set." >&2
  echo "Install ps4-payload-sdk and export PS4_PAYLOAD_SDK=/path/to/ps4-payload-sdk." >&2
  exit 2
fi
if [[ -z "${OO_PS4_TOOLCHAIN:-}" ]]; then
  echo "ERROR: OO_PS4_TOOLCHAIN is not set." >&2
  echo "Install OpenOrbis PS4 Toolchain and export OO_PS4_TOOLCHAIN=/path/to/PS4Toolchain." >&2
  exit 2
fi
if [[ ! -f "$PS4_PAYLOAD_SDK/toolchain/orbis.mk" ]]; then
  echo "ERROR: $PS4_PAYLOAD_SDK/toolchain/orbis.mk was not found." >&2
  exit 2
fi
if [[ ! -f "$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core" && ! -f "$OO_PS4_TOOLCHAIN/bin/windows/PkgTool.Core" && ! -f "$OO_PS4_TOOLCHAIN/bin/macos/PkgTool.Core" ]]; then
  echo "ERROR: PkgTool.Core was not found in the OpenOrbis toolchain." >&2
  exit 2
fi

cd "$VENDOR/pkg"
make full
PKG="$(find . -maxdepth 1 -type f -name '*.pkg' -print -quit)"
if [[ -z "$PKG" || ! -s "$PKG" ]]; then
  echo "ERROR: build finished without producing a non-empty PKG." >&2
  exit 1
fi
sha256sum "$PKG"
echo "PKG created: $VENDOR/pkg/$PKG"
