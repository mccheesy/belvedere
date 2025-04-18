# Makefile for Belvedere project
# This is a wrapper around CMake to simplify the build process

# Detect OS
UNAME_S := $(shell uname -s)

# Detect architecture
UNAME_M := $(shell uname -m)

# Default target
all: build

# Setup development environment and install dependencies
setup:
	@echo "Setting up development environment..."
ifeq ($(UNAME_S),Darwin)
	@echo "Detected macOS..."
ifeq ($(UNAME_M),arm64)
	@echo "Detected Apple Silicon (arm64)..."
	@if ! command -v brew >/dev/null 2>&1; then \
		echo "Installing Homebrew..."; \
		/bin/bash -c "$$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"; \
		echo 'eval "$$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zshrc; \
		eval "$$(/opt/homebrew/bin/brew shellenv)"; \
	fi
	@echo "Installing dependencies for Apple Silicon..."
	@brew install --build-from-source cmake cunit libuv hidapi libusb pkg-config autoconf automake libtool
else
	@echo "Detected Intel Mac..."
	@if ! command -v brew >/dev/null 2>&1; then \
		echo "Installing Homebrew..."; \
		/bin/bash -c "$$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"; \
	fi
	@echo "Installing dependencies..."
	@brew install cmake cunit libuv hidapi libusb pkg-config autoconf automake libtool
endif
else
	@echo "This setup target currently only supports macOS."
	@echo "For other platforms, please install the following dependencies manually:"
	@echo "- cmake (3.10 or higher)"
	@echo "- cunit"
	@echo "- libuv"
	@echo "- hidapi"
	@echo "- libusb"
	@echo "- pkg-config"
	@echo "- autoconf"
	@echo "- automake"
	@echo "- libtool"
endif
	@echo "Setting up git hooks..."
	@chmod +x scripts/setup-hooks.sh
	@./scripts/setup-hooks.sh
	@echo "Creating build directory..."
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
	@echo "Setup completed successfully!"

# Create build directory if it doesn't exist
build_dir:
	@mkdir -p build

# Configure the project with CMake
configure: build_dir
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build the project
build: configure
	@cd build && make

# Clean build files
clean:
	@rm -rf build

# Rebuild the project (clean and build)
rebuild: clean build

# Install the project
install: build
	@cd build && sudo make install

# Run tests
test: build
	@cd build && make test

# Run all tests with output
check: build
	@cd build && make check

# Generate code coverage report
coverage: build
	@cd build && make coverage

# Initialize development environment
init:
	@echo "Initializing development environment..."
	@chmod +x scripts/setup-hooks.sh
	@./scripts/setup-hooks.sh
	@echo "Development environment initialized successfully!"

# Help target
help:
	@echo "Available targets:"
	@echo "  setup      - Install dependencies and set up development environment"
	@echo "  all        - Build the project (default)"
	@echo "  build      - Configure and build the project"
	@echo "  clean      - Remove build directory"
	@echo "  rebuild    - Clean and rebuild the project"
	@echo "  install    - Build and install the project"
	@echo "  test       - Run tests"
	@echo "  check      - Run all tests with output"
	@echo "  coverage   - Generate code coverage report"
	@echo "  init       - Initialize development environment (git hooks, etc.)"
	@echo "  help       - Show this help message"

.PHONY: all setup build_dir configure build clean rebuild install test check coverage init help