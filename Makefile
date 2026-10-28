# ============================================================================
#  LINKED LIST ARENA - Makefile (Windows/MinGW, Linux, macOS)
#
#  Windows:   mingw32-make RAYLIB_PATH=C:/raylib/raylib-5.5_win64_mingw-w64
#  Linux:     make            (needs raylib installed, e.g. libraylib-dev)
#  macOS:     make            (brew install raylib)
# ============================================================================
CXX      ?= g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
SRC      := $(wildcard src/*.cpp)
TARGET   := LinkedListArena

ifeq ($(OS),Windows_NT)
    RAYLIB_PATH ?= C:/raylib/raylib-5.5_win64_mingw-w64
    CXXFLAGS += -I$(RAYLIB_PATH)/include
    LDFLAGS  := -L$(RAYLIB_PATH)/lib -lraylib -lopengl32 -lgdi32 -lwinmm
    TARGET   := $(TARGET).exe
else
    UNAME := $(shell uname -s)
    ifeq ($(UNAME),Darwin)
        LDFLAGS := -lraylib -framework OpenGL -framework Cocoa -framework IOKit \
                   -framework CoreVideo -framework CoreAudio
    else
        LDFLAGS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	-rm -f $(TARGET) LinkedListArena.exe

.PHONY: all run clean
