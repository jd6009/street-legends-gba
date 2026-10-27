TARGET := street-legends
BUILD := build
SOURCES := src
DEVKITPRO ?= /opt/devkitpro

CFILES := $(wildcard $(SOURCES)/*.c)
OFILES := $(patsubst $(SOURCES)/%.c,$(BUILD)/%.o,$(CFILES))
ELF := $(BUILD)/$(TARGET).elf
ROM := $(TARGET).gba

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
GBAFIX := gbafix

CFLAGS := -mthumb -mthumb-interwork -O2 -Wall -Wextra \
          -ffunction-sections -fdata-sections \
          -I$(DEVKITPRO)/libgba/include

LDFLAGS := -mthumb -mthumb-interwork -specs=gba.specs \
           -Wl,--gc-sections \
           -L$(DEVKITPRO)/libgba/lib

.PHONY: all clean

all: $(ROM)

$(ELF): $(OFILES)
	mkdir -p $(BUILD)
	$(CC) $(LDFLAGS) $(OFILES) -lgba -o $@

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tSTREETLEGEND
	truncate -s 32768 $@

$(BUILD)/%.o: $(SOURCES)/%.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(ROM)
