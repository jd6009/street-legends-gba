TARGET := street-legends
BUILD := build
SOURCES := src
DEVKITPRO ?= /opt/devkitpro

CFILES := $(wildcard $(SOURCES)/*.c)
OFILES := $(patsubst $(SOURCES)/%.c,$(BUILD)/%.o,$(CFILES))

CC := arm-none-eabi-gcc
CFLAGS := -mthumb -mthumb-interwork -O2 -Wall -Wextra \
          -ffunction-sections -fdata-sections \
          -I$(DEVKITPRO)/libgba/include
LDFLAGS := -mthumb -mthumb-interwork -specs=gba.specs \
           -Wl,--gc-sections \
           -L$(DEVKITPRO)/libgba/lib -lgba

.PHONY: all clean

all: $(TARGET).gba

$(TARGET).gba: $(OFILES)
	$(CC) $(LDFLAGS) $(OFILES) -o $@

$(BUILD)/%.o: $(SOURCES)/%.c
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(TARGET).gba
