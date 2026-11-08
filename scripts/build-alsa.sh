#!/bin/sh
#
# Build a static alsa-lib 1.0.25 for the Kindle 3 (ARMv6, musl, soft-float).
#
# alsa-lib 1.0.25 parses 64-bit integers with sscanf("%Li"). musl does not
# support that length modifier and leaves the destination uninitialised, so
# numeric ALSA options such as "card 0" are read as garbage. The patch in
# patches/ replaces the three affected printf/scanf formats with "%lld" and
# disables the library's own struct timeval/timespec definitions.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
VERSION=${ALSA_VERSION:-1.0.25}
PREFIX=${ALSA_PREFIX:-$ROOT/build/alsa}
CC=${CC:-armv6-linux-musleabi-gcc}
JOBS=${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)}
URL="https://www.alsa-project.org/files/pub/lib/alsa-lib-$VERSION.tar.bz2"
SHA256=5a1a805cf04106316d549ec650116ce6711a162e107ba8b3c551866680e286e6

mkdir -p "$ROOT/build"
cd "$ROOT/build"

if [ ! -f "alsa-lib-$VERSION.tar.bz2" ]; then
    echo "Downloading alsa-lib $VERSION..."
    curl -L --fail --output "alsa-lib-$VERSION.tar.bz2" "$URL"
fi

echo "$SHA256  alsa-lib-$VERSION.tar.bz2" | sha256sum -c -

rm -rf "alsa-lib-$VERSION"
tar xjf "alsa-lib-$VERSION.tar.bz2"
cd "alsa-lib-$VERSION"

patch -p1 < "$ROOT/patches/alsa-lib-1.0.25-musl.patch"

# The old alsa-lib headers use the BSD u_int*_t types; musl only provides
# the standard uint*_t names.
CFLAGS="-Os -march=armv6 -mfloat-abi=soft -D_GNU_SOURCE \
    -include stdint.h \
    -Du_int8_t=uint8_t -Du_int16_t=uint16_t \
    -Du_int32_t=uint32_t -Du_int64_t=uint64_t"

./configure \
    --host=arm-linux \
    --disable-shared \
    --enable-static \
    --disable-python \
    --without-versioned \
    --prefix="$PREFIX" \
    CC="$CC" \
    CFLAGS="$CFLAGS"

make -j"$JOBS"
make install

echo
echo "alsa-lib installed in $PREFIX"
