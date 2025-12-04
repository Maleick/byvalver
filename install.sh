#!/bin/bash
#
# byvalver Installation Script
# This script installs byvalver to your system

set -e

# Default variables
PREFIX="/usr/local"
BINARY_NAME="byvalver"
RELEASE_URL="https://api.github.com/repos/mrnob0dy666/byvalver/releases/latest"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to detect OS
detect_os() {
    case "$(uname -s)" in
        Linux*)
            echo "linux"
            ;;
        Darwin*)
            echo "darwin"
            ;;
        *)
            print_error "Unsupported OS: $(uname -s)"
            exit 1
            ;;
    esac
}

# Function to detect architecture
detect_arch() {
    case "$(uname -m)" in
        x86_64|amd64)
            echo "amd64"
            ;;
        aarch64|arm64)
            echo "arm64"
            ;;
        *)
            print_error "Unsupported architecture: $(uname -m)"
            exit 1
            ;;
    esac
}

# Function to download file
download_file() {
    local url="$1"
    local output="$2"
    
    if command_exists curl; then
        curl -sSL "$url" -o "$output"
    elif command_exists wget; then
        wget -q "$url" -O "$output"
    else
        print_error "Neither curl nor wget is available. Please install one of them."
        exit 1
    fi
}

# Function to determine the latest release
get_latest_release() {
    local os="$1"
    local arch="$2"
    
    local asset_name="byvalver-${os}-${arch}"
    echo "$asset_name"
}

# Function to install byvalver
install_byvalver() {
    local binary_path="$1"
    local install_dir="${PREFIX}/bin"
    
    # Create install directory if it doesn't exist
    sudo mkdir -p "$install_dir"
    
    # Install the binary
    sudo cp "$binary_path" "$install_dir/"
    sudo chmod +x "$install_dir/$BINARY_NAME"
    
    # Check if the install directory is in PATH
    if [[ ":$PATH:" != *":$install_dir:"* ]]; then
        print_warning "The installation directory ($install_dir) is not in your PATH."
        print_warning "You may need to add it to your shell configuration file (~/.bashrc, ~/.zshrc, etc.)."
        echo "export PATH=\"\$PATH:$install_dir\""
    fi
    
    print_status "Successfully installed byvalver to $install_dir/"
    print_status "You can now run: byvalver --help"
}

# Function to compile byvalver from source
compile_from_source() {
    if [ ! -f "Makefile" ] || [ ! -d "src" ]; then
        print_error "Makefile or source directory not found in current directory."
        print_error "Please run this script from the byvalver source directory."
        exit 1
    fi
    
    print_status "Building byvalver from source..."
    
    # Build the project
    make clean
    make release
    
    local binary_path="./bin/byvalver"
    if [ ! -f "$binary_path" ]; then
        print_error "Build failed. Binary not found at $binary_path"
        exit 1
    fi
    
    install_byvalver "$binary_path"
}

# Main installation function
main() {
    print_status "Starting byvalver installation..."
    
    # Check for root privileges if installing to system directories
    if [[ "$PREFIX" == "/usr/local"* ]]; then
        if [[ $EUID -eq 0 ]]; then
            print_warning "Running as root. It's generally not recommended to run scripts as root."
        else
            if ! command_exists sudo; then
                print_error "sudo is required to install to $PREFIX. Please install sudo or run as root."
                exit 1
            fi
        fi
    fi
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                echo "Usage: $0 [OPTIONS]"
                echo "Install byvalver to your system"
                echo ""
                echo "Options:"
                echo "  -h, --help     Show this help message"
                echo "  -p, --prefix   Installation prefix (default: /usr/local)"
                echo "  --from-source  Build and install from source instead of downloading pre-built binary"
                exit 0
                ;;
            -p|--prefix)
                PREFIX="$2"
                shift 2
                ;;
            --from-source)
                compile_from_source
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    # Check if building from source is requested
    if [ -f "Makefile" ] && [ -d "src" ]; then
        compile_from_source
        exit 0
    fi
    
    # Otherwise, try to download pre-built binary
    print_warning "Pre-built binary installation not yet implemented."
    print_warning "Building from source instead..."
    compile_from_source
}

# Run the main function with all arguments
main "$@"