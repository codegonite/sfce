SOURCE := sfce.c
TARGET := bin/$(notdir $(CURDIR)).exe

.PHONY: run clean

$(TARGET): $(SOURCE) | build
	gcc $(SOURCE) -Wall -Wextra -Wunused-function -o $(TARGET)

clean:
	rm -r build/*

run: $(TARGET)
	$(TARGET)

build:
	mkdir build
