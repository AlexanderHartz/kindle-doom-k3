#!/bin/sh

DIR=$(cd "$(dirname "$0")" && pwd) || exit 1
cd "$DIR" || exit 1

export HOME=/mnt/us
LOG="$DIR/prboom.log"
PRBOOM_GAMMA=${PRBOOM_GAMMA:-1.0}
PRBOOM_INVERT=${PRBOOM_INVERT:-1}
export PRBOOM_GAMMA PRBOOM_INVERT

if [ -f "$DIR/asound.conf" ]; then
    ALSA_CONFIG_PATH="$DIR/asound.conf"
    export ALSA_CONFIG_PATH
elif [ -z "$ALSA_CONFIG_PATH" ] && [ -f /usr/share/alsa/alsa.conf ]; then
    ALSA_CONFIG_PATH=/usr/share/alsa/alsa.conf
    export ALSA_CONFIG_PATH
fi

restore() {
    echo "run.sh: restoring framework" >>"$LOG" 2>&1
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
{
    echo "=== PrBoom diagnostics ==="
    date
    echo "DIR=$DIR"
    echo "IWAD=$IWAD"
    echo "PWAD=$PWAD"
    echo "PRBOOM_GAMMA=$PRBOOM_GAMMA PRBOOM_INVERT=$PRBOOM_INVERT"
    echo "ALSA_CONFIG_PATH=$ALSA_CONFIG_PATH"
    ls -l "$DIR/asound.conf" 2>&1
    cat "$DIR/asound.conf" 2>&1
    echo "--- /dev ---"
    ls -l /dev/dsp /dev/snd 2>&1
    echo "--- /dev/snd ---"
    ls -l /dev/snd 2>&1
    echo "--- /proc/asound/cards ---"
    cat /proc/asound/cards 2>&1
    echo "--- /proc/asound/pcm ---"
    cat /proc/asound/pcm 2>&1
    echo "--- /usr/share/alsa ---"
    ls -l /usr/share/alsa/alsa.conf /usr/share/alsa 2>&1
    echo "--- audio commands ---"
    type aplay 2>&1
    type amixer 2>&1
    if type aplay >/dev/null 2>&1; then aplay -l 2>&1; fi
    if type amixer >/dev/null 2>&1; then
        amixer -c 0 scontrols 2>&1
        amixer -c 0 sset 'Master' 100% unmute 2>&1
        amixer -c 0 sset 'PCM' 100% unmute 2>&1
        amixer -c 0 sset 'Speaker' 100% unmute 2>&1
        amixer -c 0 sset 'Headphone' 100% unmute 2>&1
    fi
    echo "=== prboom output ==="
} >>"$LOG" 2>&1

/etc/init.d/framework stop >/dev/null 2>&1 || \
    stop framework >/dev/null 2>&1
sleep 1

lipc-set-prop -- com.lab126.powerd preventScreenSaver 1 >/dev/null 2>&1 || \
    lipc-set-prop com.lab126.powerd preventScreenSaver 1 >/dev/null 2>&1

./prboom -iwad "$IWAD" -file "$PWAD" -nomusic -warp 1 1 >>"$LOG" 2>&1
rc=$?
echo "prboom exit=$rc" >>"$LOG" 2>&1

exit 0
