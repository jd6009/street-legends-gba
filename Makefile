TARGET := street-legends
BUILD := build
SOURCES := src
INCLUDES :=

CFILES := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
OFILES := $(CFILES:%.c=$(BUILD)/%.o)

CC := arm-none-eabi-gcc
LD := arm-none-eabi-gcc

ARCH := -mthumb -mthumb-interwork
CFLAGS := $(ARCH) -O2 -Wall -Wextra -ffunction-sections -fdata-sections
LDFLAGS := $(ARCH) -specs=gba.specs -Wl,--gc-sections

.PHONY: all clean

all: $(TARGET).gba

$(TARGET).gba: $(OFILES)
	$(LD) $(LDFLAGS) $(OFILES) -o $@

$(BUILD)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD) $(TARGET).gba
