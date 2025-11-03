#!/bin/bash

# hashbrowns Build Script
# Automated build configuration for different platforms and build types

set -e  # Exit on any error

# Configuration
PROJECT_NAME="hashbrowns"
BUILD_DIR="build"
CMAKE_MIN_VERSION="3.16"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    # Check CMake
    if ! command_exists cmake; then
        print_error "CMake is not installed. Please install CMake $CMAKE_MIN_VERSION or higher."
        exit 1
    fi
    
    local cmake_version=$(cmake --version | grep -o '[0-9]\+\.[0-9]\+' | head -1)
    print_status "Found CMake version: $cmake_version"
    
    # Check compiler
    if command_exists g++; then
        local gcc_version=$(g++ --version | grep -o '[0-9]\+\.[0-9]\+' | head -1)
        print_status "Found GCC version: $gcc_version"
    elif command_exists clang++; then
        local clang_version=$(clang++ --version | grep -o '[0-9]\+\.[0-9]\+' | head -1)
        print_status "Found Clang version: $clang_version"
    else
        print_error "No suitable C++ compiler found. Please install GCC 8+ or Clang 6+."
        exit 1
    fi
    
    print_success "Prerequisites check passed!"
}

# Function to clean build directory
clean_build() {
    if [ -d "$BUILD_DIR" ]; then
        print_status "Cleaning existing build directory..."
        rm -rf "$BUILD_DIR"
    fi
}

# Function to configure build
configure_build() {
    local build_type=${1:-Release}
    local additional_flags="$2"
    
    print_status "Configuring $build_type build..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake .. \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        $additional_flags
    
    cd ..
    print_success "Configuration complete!"
}

# Function to build project
build_project() {
    local jobs=${1:-$(nproc 2>/dev/null || echo 4)}
    
    print_status "Building project with $jobs parallel jobs..."
    
    cd "$BUILD_DIR"
    make -j"$jobs"
    cd ..
    
    print_success "Build complete!"
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    cd "$BUILD_DIR"
    if make test; then
        print_success "All tests passed!"
    else
        print_error "Some tests failed!"
        return 1
    fi
    cd ..
}

# Function to install project
install_project() {
    local prefix="$1"
    
    print_status "Installing project..."
    
    cd "$BUILD_DIR"
    if [ -n "$prefix" ]; then
        cmake --install . --prefix "$prefix"
    else
        make install
    fi
    cd ..
    
    print_success "Installation complete!"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -c, --clean         Clean build directory before building"
    echo "  -t, --type TYPE     Build type: Debug, Release, RelWithDebInfo (default: Release)"
    echo "  -j, --jobs JOBS     Number of parallel build jobs (default: auto-detect)"
    echo "  -i, --install       Install after building"
    echo "  --prefix PATH       Installation prefix"
    echo "  --test              Run tests after building"
    echo "  --static-analysis   Enable static analysis with clang-tidy"
    echo "  --sanitizers        Enable sanitizers (Debug builds only)"
    echo "  --verbose           Verbose output"
    echo ""
    echo "Examples:"
    echo "  $0                           # Basic release build"
    echo "  $0 -c -t Debug --test        # Clean debug build with tests"
    echo "  $0 --static-analysis         # Build with static analysis"
    echo "  $0 -t Release -i --prefix=/usr/local  # Release build and install"
}

# Main script logic
main() {
    local build_type="Release"
    local clean=false
    local run_tests_flag=false
    local install_flag=false
    local install_prefix=""
    local jobs=""
    local additional_cmake_flags=""
    local verbose=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -c|--clean)
                clean=true
                shift
                ;;
            -t|--type)
                build_type="$2"
                shift 2
                ;;
            -j|--jobs)
                jobs="$2"
                shift 2
                ;;
            -i|--install)
                install_flag=true
                shift
                ;;
            --prefix)
                install_prefix="$2"
                shift 2
                ;;
            --test)
                run_tests_flag=true
                shift
                ;;
            --static-analysis)
                additional_cmake_flags="$additional_cmake_flags -DCMAKE_CXX_CLANG_TIDY=clang-tidy"
                shift
                ;;
            --sanitizers)
                if [ "$build_type" = "Debug" ]; then
                    additional_cmake_flags="$additional_cmake_flags -DENABLE_SANITIZERS=ON"
                else
                    print_warning "Sanitizers are only supported in Debug builds"
                fi
                shift
                ;;
            --verbose)
                verbose=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Enable verbose output if requested
    if [ "$verbose" = true ]; then
        set -x
    fi
    
    print_status "Starting $PROJECT_NAME build process..."
    
    # Execute build steps
    check_prerequisites
    
    if [ "$clean" = true ]; then
        clean_build
    fi
    
    configure_build "$build_type" "$additional_cmake_flags"
    
    if [ -n "$jobs" ]; then
        build_project "$jobs"
    else
        build_project
    fi
    
    if [ "$run_tests_flag" = true ]; then
        run_tests
    fi
    
    if [ "$install_flag" = true ]; then
        install_project "$install_prefix"
    fi
    
    print_success "$PROJECT_NAME build process completed successfully!"
    
    # Show build artifacts
    print_status "Build artifacts:"
    if [ -d "$BUILD_DIR" ]; then
        find "$BUILD_DIR" -name "*.a" -o -name "hashbrowns" | sed 's/^/  /'
    fi
}

# Run main function with all arguments
main "$@"