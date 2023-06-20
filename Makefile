CC = g++
CXX_FLAGS = -std=c++20 -Wall -Wextra -pedantic

TARGET = kilo
BUILD = build
SRC = src

SOURCES = $(shell find $(SRC) -name *.cpp)
OBJS = $(SOURCES:%=$(BUILD)/%.o)
DEPS = $(OBJS:.o=.d)

$(TARGET): $(OBJS)
	@echo "Building..."
	$(CC) $(OBJS) $(CXX_FLAGS) -o $@

$(BUILD)/%.cpp.o: %.cpp
	@echo "Compiling C++ files"
	$(MKDIR_P) $(dir $@)
	$(CC) $(CXX_FLAGS) -c $< -o $@

.PHONY: clean

clean:
	@echo "Cleaning..."
	$(RM) -r $(BUILD)
	$(RM) $(TARGET)

-include $(DEPS)

MKDIR_P ?= mkdir -p
