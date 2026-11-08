#!/bin/sh
#
# Build the static Kindle 3 binary and the KUAL extension ZIP.
#
# Run scripts/build-alsa.sh first, or point ALSA_PREFIX at an existing
# static alsa-lib installation.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
CC=${CC:-armv6-linux-musleabi-gcc}
STRIP=${STRIP:-armv6-linux-musleabi-strip}
ALSA_PREFIX=${ALSA_PREFIX:-$ROOT/build/alsa}
JOBS=${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)}

if [ ! -f "$ALSA_PREFIX/lib/libasound.a" ]; then
    echo "error: $ALSA_PREFIX/lib/libasound.a not found" >&2
    echo "Run scripts/build-alsa.sh first or set ALSA_PREFIX." >&2
    exit 1
fi

ALSA_CFLAGS="-DHAVE_ALSA -I$ALSA_PREFIX/include -include stdint.h \
    -Du_int8_t=uint8_t -Du_int16_t=uint16_t \
    -Du_int32_t=uint32_t -Du_int64_t=uint64_t"
ALSA_LIBS="$ALSA_PREFIX/lib/libasound.a -ldl -lpthread -lm"

cd "$ROOT"
make clean
make -j"$JOBS" \
    CC="$CC" \
    STRIP="$STRIP" \
    ALSA_CFLAGS="$ALSA_CFLAGS" \
    ALSA_LIBS="$ALSA_LIBS"
make extension \
    CC="$CC" \
    STRIP="$STRIP" \
    ALSA_CFLAGS="$ALSA_CFLAGS" \
    ALSA_LIBS="$ALSA_LIBS"

echo
echo "Built: $ROOT/prboom"
echo "Package: $ROOT/PrBoom_KUAL.zip"
