# Compiler
CXX = clang++

# Compiler Flags
CXXFLAGS = -Wall -Wextra -O2 -std=c++17 -pthread

# Target (program name)
TARGET = serial_logger

# Source and Build Directories
SRC_DIR = src
BUILD_DIR = .build

# Source Files (all .cpp files in src directory)
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Object Files (in the build directory)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Default Target
all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

# Ensure the build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Rule to create the program
$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Rule to create object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Install rule (optional)
install: $(BUILD_DIR)/$(TARGET)
	cp $(BUILD_DIR)/$(TARGET) /usr/local/bin/

# Uninstall rule (optional)
uninstall:
	rm -f /usr/local/bin/$(TARGET)

