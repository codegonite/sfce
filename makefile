DEPS_FLAGS		:= -M -MMD -MP
LIB_FLAGS		:=
INCLUDE_FLAGS	:=
CC_FLAGS		:= -I"C:/_dev/include/c" -Wall -Wextra -Wunused-function
CXX_FLAGS		:= $(CC_FLAGS) -std=c++20

CC				:= gcc
CXX				:= g++

TARGET			:= bin/$(notdir $(CURDIR)).exe
SOURCE_FILES	:= $(shell find source -type f)
INCLUDE_FILES	:= $(shell find include -type f)

CPP_FILES		:= $(filter %.cpp, $(SOURCE_FILES))
C_FILES			:= $(filter %.c,   $(SOURCE_FILES))
HPP_FILES		:= $(filter %.hpp, $(SOURCE_FILES) $(INCLUDE_FILES))
H_FILES			:= $(filter %.h,   $(SOURCE_FILES) $(INCLUDE_FILES))

C_INCLUDES      := $(patsubst %,-I"%",$(shell find include -type d))

OBJECT_FILES	:= $(patsubst source/%,build/%.o, $(CPP_FILES) $(C_FILES))

ifeq ($(strip $(CPP_FILES)),)
	LD := $(CC)
else
	LD := $(CXX)
endif

.PHONY: run clean

$(TARGET): $(OBJECT_FILES) | build
	$(LD) $(OBJECT_FILES) -o $(TARGET) $(LIB_FLAGS)

clean:
	rm -r build/*

run:
	$(TARGET)

build:
	mkdir build

build/%.cpp.o: source/%.cpp
	$(CXX) $(CXX_FLAGS) $(DEPS_FLAGS) -c $< -MT $@ -MF build/$*.cpp.d -o $@
	$(CXX) $(CXX_FLAGS) -c $< -o $@

build/%.c.o: source/%.c
	$(CC) $(CC_FLAGS) $(C_INCLUDES) $(DEPS_FLAGS) -c $< -MT $@ -MF build/$*.c.d -o $@
	$(CC) $(CC_FLAGS) $(C_INCLUDES) -c $< -o $@
