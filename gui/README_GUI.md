cat > gui/README_GUI.md << 'EOF'
# myshell GUI Frontend

## Overview

This is a lightweight **Python Tkinter GUI wrapper** for the `myshell` C shell executable.

### Key Features

- ✅ **Terminal-like interface** with scrollable output
- ✅ **Command input field** with Enter key support
- ✅ **Run, Clear, Exit buttons** for easy control
- ✅ **Real-time output capture** from shell subprocess
- ✅ **Non-blocking I/O** with threading
- ✅ **Zero modification** to C backend
- ✅ **Works on Ubuntu 22.04+ with Python 3.8+**

## Requirements

### Ubuntu System Requirements

```bash
# Python 3.8+ with Tkinter
sudo apt update
sudo apt install python3 python3-tk python3-dev
