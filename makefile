SOURCE := sfce.c
TARGET := bin\sfce

.PHONY: run clean

$(TARGET): $(SOURCE) makefile | build
	gcc $(SOURCE) -std=c99 -O3 -g -Wall -Wextra -Wunused-function -o $(TARGET) -static -static-libgcc

clean:
	rm -r build/*

run: $(TARGET)
	$(TARGET)

build:
	mkdir build
