CC := /opt/amiga/bin/m68k-amigaos-gcc
CFLAGS ?= -Os -Wall -Wextra -fomit-frame-pointer -mcrt=nix13 -fno-builtin
LDFLAGS ?= -mcrt=nix13
CPPFLAGS += -Iinclude -DAMIGA_OS13
BUILD := build
TARGET := $(BUILD)/Tankerkoenig
CORE := $(BUILD)/tkcore
TARGET_OBJS := $(BUILD)/launcher.o
CORE_OBJS := $(BUILD)/main.o $(BUILD)/app.o $(BUILD)/config.o $(BUILD)/https.o $(BUILD)/json.o $(BUILD)/amitls13_client_stubs.o
CONFIG := $(BUILD)/Tankerkoenig.conf
HOST_CC ?= cc
HOST_TEST := $(BUILD)/json_test
.PHONY: all clean test-json
all: $(TARGET) $(CORE) $(CONFIG)
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
$(BUILD)/%.o: src/%.S | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
$(TARGET): $(TARGET_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(TARGET_OBJS)
$(CORE): $(CORE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(CORE_OBJS)
$(CONFIG): Tankerkoenig.conf | $(BUILD)
	cp Tankerkoenig.conf $@

$(HOST_TEST): src/json.c tests/json_test.c | $(BUILD)
	$(HOST_CC) -std=c89 -Wall -Wextra -pedantic -Iinclude -o $@ src/json.c tests/json_test.c

test-json: $(HOST_TEST)
	$(HOST_TEST) tests/fixtures

clean:
	rm -rf $(BUILD)
