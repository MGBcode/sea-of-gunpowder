CC = gcc
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS ?=
LDLIBS ?= -lraylib -lm
SOURCES = src/main.c src/estruturas.c src/ordenacao.c src/combate.c src/ia_inimigos.c src/dados_jogo.c src/animacao.c src/interface.c
OBJECTS = $(SOURCES:src/%.c=build/%.o)

ifeq ($(OS),Windows_NT)
	SHELL := cmd.exe
	.SHELLFLAGS := /C
	CFLAGS += -isystem C:/w64devkit/include -isystem C:/w64devkit/x86_64-w64-mingw32/include
	LDFLAGS += -LC:/w64devkit/lib -LC:/w64devkit/x86_64-w64-mingw32/lib
	TARGET = build/jogo.exe
	LDLIBS += -lopengl32 -lgdi32 -lwinmm -lshell32
	MKDIR_BUILD = if not exist build mkdir build
	RM = if exist build rmdir /S /Q build
else
	TARGET = build/jogo
	MKDIR_BUILD = mkdir -p build
	RM = rm -rf build
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		LDLIBS += -ldl -lpthread -lX11
	endif
	ifeq ($(UNAME_S),Darwin)
		LDLIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	endif
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

build/%.o: src/%.c
	$(MKDIR_BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM)
