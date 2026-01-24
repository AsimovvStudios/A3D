CC ?= clang
DBG ?= -DDEBUG
API ?= --opengl
CFLAGS = -std=c11 -Wall -Wextra $(DBG) $(shell pkg-config --cflags sdl3) -Iglad -Iinclude
LDFLAGS = $(shell pkg-config --libs sdl3) -lvulkan -lGL -lm -lcglm

SRC = src/*.c src/vulkan/*.c src/opengl/*.c glad/glad/*.c tests/*.c
BIN = build/a3d_test

GLSLANG = glslangValidator
VSH_SRC = shaders_vk/triangle.vert
FSH_SRC = shaders_vk/triangle.frag
VSH_SPV = shaders_vk/triangle.vert.spv
FSH_SPV = shaders_vk/triangle.frag.spv

all: $(BIN)

$(BIN): $(SRC) $(VSH_SPV) $(FSH_SPV)
	BUILD_MODE=$(BUILD_MODE)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS)

$(VSH_SPV): $(VSH_SRC)
	$(GLSLANG) -V $< -o $@

$(FSH_SPV): $(FSH_SRC)
	$(GLSLANG) -V $< -o $@

run: $(BIN)
	./$(BIN) $(API)

clean:
	rm -rf build
	rm -f shaders/*.spv shaders_vk/*.spv

compile_flags:
	@echo "generating compile_flags.txt"
	@rm -f compile_flags.txt
	@for flag in $(CFLAGS); do echo $$flag >> compile_flags.txt; done

.PHONY: all run clean compile_flags
