CC := /opt/amiga/bin/m68k-amigaos-gcc
CFLAGS ?= -Os -Wall -Wextra -fomit-frame-pointer -mcrt=nix13 -fno-builtin
LDFLAGS ?= -mcrt=nix13
CPPFLAGS += -Iinclude -DAMIGA_OS13
BUILD := build
TARGET := $(BUILD)/Tankerkoenig
CORE := $(BUILD)/tkcore
TARGET_OBJS := $(BUILD)/launcher.o
CORE_OBJS := $(BUILD)/main.o $(BUILD)/app.o
.PHONY: all clean
all: $(TARGET) $(CORE)
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
$(TARGET): $(TARGET_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(TARGET_OBJS)
$(CORE): $(CORE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CORE_OBJS)
clean:
	rm -rf $(BUILD)
