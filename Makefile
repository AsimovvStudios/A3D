.POSIX:
SHELL = /bin/sh

MESON = meson
BUILD = build/gl
BACKEND = gl

.PHONY: all setup compile gl vk run clean distclean compile_flags

all: compile

setup:
	@if [ ! -d "$(BUILD)" ]; then \
		$(MESON) setup "$(BUILD)" -Drender_backend="$(BACKEND)"; \
	else \
		$(MESON) configure "$(BUILD)" -Drender_backend="$(BACKEND)"; \
	fi

compile: setup
	$(MESON) compile -C "$(BUILD)"

gl:
	$(MAKE) compile BACKEND=gl BUILD=build/gl

vk:
	$(MAKE) compile BACKEND=vk BUILD=build/vk

run: compile
	"./$(BUILD)/a3d_test"

clean:
	@if [ -d "$(BUILD)" ]; then \
		$(MESON) compile -C "$(BUILD)" --clean; \
	fi

distclean:
	rm -rf build

compile_flags: setup
	@echo "generating compile_flags.txt"
	@: > compile_flags.txt
	@printf '%s\n' -std=c11 -Wall -Wextra -Iinclude -Iexternal -DDEBUG >> compile_flags.txt
	@if [ "$(BACKEND)" = "vk" ]; then \
		printf '%s\n' -DBACKEND_VK >> compile_flags.txt; \
	else \
		printf '%s\n' -DBACKEND_GL >> compile_flags.txt; \
	fi
	@pkg-config --cflags sdl3 cglm | tr ' ' '\n' >> compile_flags.txt
