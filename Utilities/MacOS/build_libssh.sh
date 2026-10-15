#!/bin/bash
#
# Build OpenSSL and libssh from source for the macOS ITK-SNAP package.
#
# Usage: build_libssh.sh <install-prefix>
#
# Homebrew's libssh is not suitable for the package: its libraries are built
# for the macOS version of the build machine rather than the package's
# minimum macOS version, and on Intel Macs Homebrew no longer provides
# bottles, so installing it builds CMake, OpenSSL and libssh from source
# (about 40 minutes). This script builds just OpenSSL and libssh for
# MACOSX_DEPLOYMENT_TARGET and installs them into <install-prefix>, which
# is then passed to the ITK-SNAP build in CMAKE_PREFIX_PATH.
#
# The tarballs are checked against the SHA256 sums below. When updating a
# version, take the OpenSSL sum from the .sha256 file published next to the
# tarball, and check the libssh tarball's .asc signature (libssh release key
# 88A228D89B07C2C77D0C780903D5DF8CFDD3E8E7) before computing its sum.
set -euo pipefail

OPENSSL_VERSION=3.5.9
OPENSSL_SHA256=603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a
LIBSSH_VERSION=0.12.2
LIBSSH_SHA256=49560f677d96e3706a904ac2de1116e25f3680937d51e5c92198fcba4a1c1e9f

if [ $# -ne 1 ]; then
  echo "Usage: $0 <install-prefix>" >&2
  exit 1
fi

: "${MACOSX_DEPLOYMENT_TARGET:?MACOSX_DEPLOYMENT_TARGET must be set}"
export MACOSX_DEPLOYMENT_TARGET

mkdir -p "$1"
PREFIX=$(cd "$1" && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
NCPU=$(sysctl -n hw.ncpu)

download() {
  local url=$1 sha=$2 file
  file="$WORK/$(basename "$url")"
  curl -fsSL --retry 3 -o "$file" "$url"
  echo "$sha  $file" | shasum -a 256 -c -
  tar -xf "$file" -C "$WORK"
}

# OpenSSL (shared libraries only, no docs or tests)
download "https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz" \
  "$OPENSSL_SHA256"
case "$(uname -m)" in
  arm64) OPENSSL_TARGET=darwin64-arm64-cc ;;
  x86_64) OPENSSL_TARGET=darwin64-x86_64-cc ;;
  *) echo "Unsupported architecture $(uname -m)" >&2; exit 1 ;;
esac
(
  cd "$WORK/openssl-${OPENSSL_VERSION}"
  ./Configure "$OPENSSL_TARGET" shared no-tests \
    --prefix="$PREFIX" --libdir=lib \
    "-mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}"
  make -j"$NCPU"
  make install_sw
)

# libssh, against that OpenSSL. GSSAPI (Kerberos) comes from macOS itself.
# The libraries get an absolute install name, as Homebrew's do, so that
# macdeployqt can find and bundle them.
download "https://www.libssh.org/files/${LIBSSH_VERSION%.*}/libssh-${LIBSSH_VERSION}.tar.xz" \
  "$LIBSSH_SHA256"
cmake -S "$WORK/libssh-${LIBSSH_VERSION}" -B "$WORK/libssh-build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DCMAKE_INSTALL_NAME_DIR="$PREFIX/lib" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET" \
  -DCMAKE_PREFIX_PATH="$PREFIX" \
  -DOPENSSL_ROOT_DIR="$PREFIX" \
  -DWITH_EXAMPLES=OFF \
  -DWITH_SYMBOL_VERSIONING=OFF
cmake --build "$WORK/libssh-build" --parallel "$NCPU"
cmake --install "$WORK/libssh-build"
