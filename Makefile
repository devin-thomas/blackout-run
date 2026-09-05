DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM

CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
OBJCOPY := $(DEVKITARM)/bin/arm-none-eabi-objcopy
GBAFIX := $(DEVKITPRO)/tools/bin/gbafix

TARGET := blackout-run
BUILD := build
CFLAGS := -mthumb -mthumb-interwork -O2 -Wall -Wextra -Werror -std=gnu11 -ffunction-sections -fdata-sections -Iinclude -Iassets/grit -I$(DEVKITPRO)/libgba/include
LDFLAGS := -mthumb -mthumb-interwork -specs=gba.specs -Wl,--gc-sections -Wl,--undefined=g_br_debug_offsets -Wl,-Map,$(BUILD)/$(TARGET).map -L$(DEVKITPRO)/libgba/lib
LIBS := -lgba

GAME_SOURCES := source/main.c source/sim.c source/video.c source/audio.c
ASSET_SOURCES := assets/grit/city_tiles.c assets/grit/font.c assets/grit/wordmark.c \
	assets/grit/courier.c assets/grit/pedestrians_4dir.c assets/grit/cars.c \
	assets/grit/cyclists_4dir.c assets/grit/crowd_clusters.c \
	assets/grit/package_marker.c assets/grit/bell_ring.c \
	assets/grit/route_dots.c assets/grit/delivery_fx.c \
	assets/grit/palette_variants.c
SOURCES := $(GAME_SOURCES) $(ASSET_SOURCES)
OBJECTS := $(patsubst %.c,$(BUILD)/%.o,$(SOURCES))

.PHONY: all clean verify test size site

all: $(TARGET).gba

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary "$<" "$@"
	$(GBAFIX) "$@" "-tBLACKOUT RUN" -cBLKR -m00

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -o "$@" $(OBJECTS) $(LIBS)

$(BUILD)/%.o: %.c
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) -MMD -MP -c "$<" -o "$@"

test:
	mkdir -p $(BUILD)
	cc -std=c11 -Wall -Wextra -Werror -pedantic -DBLACKOUT_RUN_HOST_TEST -Iinclude source/sim.c tests/test_sim.c -o $(BUILD)/test-sim
	$(BUILD)/test-sim
	python3 -m unittest discover -s tests -p 'test_*.py' -v

verify: $(TARGET).gba
	python3 tools/verify_rom.py $(TARGET).gba $(TARGET).elf

size: $(TARGET).elf
	$(DEVKITARM)/bin/arm-none-eabi-size $(TARGET).elf

site: verify
	mkdir -p site/downloads
	mkdir -p site/assets/screens
	cp $(TARGET).gba site/downloads/blackout-run.gba
	chmod 644 site/downloads/blackout-run.gba
	cp validation/native/*.png site/assets/screens/

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).gba

-include $(OBJECTS:.o=.d)
