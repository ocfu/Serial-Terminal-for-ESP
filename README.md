# Serial Terminal for ESP

**Serial-Terminal-for-ESP** is a command-line utility written in C++ for macOS and Linux. It facilitates serial communication with an ESP by reading data from a specified serial port and writing data to it. The program can also log specific patterns of received data to a file and handle user-defined baud rates.

## Features
- Opens a serial port for a terminal communication with an ESP.
- Reads and writes data via the serial port.
- Logs specific data patterns to a file.
- Supports customizable baud rates via command-line arguments.
- Redirects `stdin` to the serial port for sending data.
- Maintains build and runtime cleanliness with organized directories.
  
## It does not
- Upload or download sketches, filesystems or files over the serial port.

## Prerequisites
- **ESP8266** (ESP32 to come)
- **macOS** or **Linux**
- **Xcode Command Line Tools** (macOS) or **gcc/g++** (Linux)
- **C++17** or later support

## Installation

1. Clone the repository:
    ```bash
    git clone https://github.com/ocfu/Serial-Terminal-for-ESP.git
    cd Serial-Terminal-for-ESP
    ```

2. Build the program:
    ```bash
    make
    ```
    This will create the `serial_esp` executable in the `.build` directory.

3. (Optional) Install for the user:
    ```bash
    make install
    ```
    This will copy the program to `~/bin/`.

## Usage

### Command-Line Syntax
```bash
serial_esp -b <baudrate> [-d] <serial_port>
```

### Options:
- `-b <baudrate>`: Sets the baud rate for the serial connection (e.g., `115200`).
- `-d`: Enables logging to a file. Logs matching patterns to `dump.log` or an indexed file if `dump.log` already exists.
- `<serial_port>`: Specifies the serial port to use (e.g., `/dev/ttyUSB0`).

### Examples:
1. Open serial port `/dev/ttyUSB0` at 115200 baud:
    ```bash
    serial_esp -b 115200 /dev/ttyUSB0
    ```

2. Log data to a file:
    ```bash
    serial_esp -b 115200 -d /dev/ttyUSB0
    ```

3. Send data via `stdin`:
    ```bash
    echo "Hello, Serial!" | serial_esp -b 115200 /dev/ttyUSB0
    ```

## Logging Details
The program looks for the following pattern in received data:
```plaintext
--------------- CUT HERE FOR EXCEPTION DECODER ---------------
```
Once detected, it logs all data to `dump.log` until the pattern appears again. The program ensures the file is not overwritten by copying the existing log file to a new indexed filename.

## Development

### Source Code
- All source files are located in the `src` directory.

### Build Artifacts
- Compiled files and executables are stored in the `.build` directory, which is automatically created during the build process.

### Clean Build
To remove all build artifacts:
```bash
make clean
```

## Linux-Specific Instructions
- Ensure `g++` is installed. On most distributions, you can install it with:
    ```bash
    sudo apt update && sudo apt install g++
    ```
- Install `make` if it's not already installed:
    ```bash
    sudo apt install make
    ```
- Serial port names on Linux typically follow the `/dev/tty*` convention (e.g., `/dev/ttyUSB0` or `/dev/ttyS0`).

## Contributing
Contributions are welcome! Please follow these steps:
1. Fork the repository.
2. Create a feature branch: `git checkout -b my-feature`
3. Commit your changes: `git commit -am 'Add new feature'`
4. Push to the branch: `git push origin my-feature`
5. Create a pull request.

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.

## Acknowledgments
Thanks to everyone contributing to open-source projects that inspired this utility!

