CC ?= armv6-linux-musleabi-gcc
STRIP ?= armv6-linux-musleabi-strip
INCLUDE=-Isrc/ -Isrc/KINDLE -I.

CFLAGS=$(INCLUDE) -D_GNU_SOURCE -DHAVE_CONFIG_H -DDISABLE_DOUBLEBUFFER \
	-Os -fno-pie -march=armv6 -mfloat-abi=soft -pthread
LDFLAGS=-static -no-pie -pthread
ALSA_CFLAGS ?=
ALSA_LIBS ?=
CFLAGS += $(ALSA_CFLAGS)

C_FILES=$(wildcard src/*.c) $(wildcard src/KINDLE/*.c)
OBJ_FILES=$(patsubst %.c,%.o,$(C_FILES))

all: prboom

src/%.o: src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

prboom: $(OBJ_FILES) Makefile
	$(CC) $(LDFLAGS) $(CFLAGS) -o prboom $(OBJ_FILES) $(ALSA_LIBS)
	$(STRIP) prboom

extension: prboom
	cp $< extensions/prboom/$<
	cp data/prboom.wad extensions/prboom/prboom.wad
	cp data/README.prboom-wad.md extensions/prboom/PRBOOM-WAD-LICENSE.md
	cp COPYING extensions/prboom/COPYING
	rm -f extensions/prboom/prboom.log
	rm -f PrBoom_KUAL.zip
	zip -r PrBoom_KUAL.zip extensions

clean:
	rm -rf $(OBJ_FILES)
	rm -f prboom PrBoom_KUAL.zip
	rm -f extensions/prboom/prboom extensions/prboom/prboom.wad
	rm -f extensions/prboom/PRBOOM-WAD-LICENSE.md extensions/prboom/COPYING
	rm -f extensions/prboom/prboom.log