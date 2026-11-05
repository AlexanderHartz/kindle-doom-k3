#!/bin/sh

DIR=$(cd "$(dirname "$0")" && pwd) || exit 1
cd "$DIR" || exit 1

export HOME=/mnt/us
LOG=/tmp/prboom.log

restore() {
    lipc-set-prop -- com.lab126.powerd preventScreenSaver 0 >/dev/null 2>&1 || \
        lipc-set-prop com.lab126.powerd preventScreenSaver 0 >/dev/null 2>&1
    /etc/init.d/framework start >/dev/null 2>&1 || \
        start framework >/dev/null 2>&1
}

trap restore EXIT INT TERM HUP

IWAD=""
for f in /mnt/us/doom.wad /mnt/us/DOOM.WAD \
         /mnt/us/doom/doom.wad /mnt/us/doom/DOOM.WAD \
         /mnt/us/freedoom1.wad /mnt/us/freedoom.wad; do
    if [ -f "$f" ]; then
        IWAD="$f"
        break
    fi
done

if [ -z "$IWAD" ]; then
    eips 0 0 "PrBoom: doom.wad not found" >/dev/null 2>&1
    sleep 3
    exit 1
fi

PWAD="$DIR/prboom.wad"
[ -f "$PWAD" ] || PWAD=/mnt/us/prboom.wad

chmod +x "$DIR/prboom" 2>/dev/null
if [ ! -f "$DIR/prboom" ]; then
    eips 0 0 "PrBoom: binary not found" >/dev/null 2>&1
    sleep 3
    exit 1
fi

: >"$LOG"

/etc/init.d/framework stop >/dev/null 2>&1 || \
    stop framework >/dev/null 2>&1
sleep 1

lipc-set-prop -- com.lab126.powerd preventScreenSaver 1 >/dev/null 2>&1 || \
    lipc-set-prop com.lab126.powerd preventScreenSaver 1 >/dev/null 2>&1

./prboom -iwad "$IWAD" -file "$PWAD" -nosound -nomusic -nosfx -warp 1 1 \
    >>"$LOG" 2>&1

exit 0
