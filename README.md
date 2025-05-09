# Wienderer

A small tool for drawing basic shapes using the Vulkan API.

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [License](#license)
- [Acknowledgments](#acknowledgments)

## Features
- **Basic Shape Rendering**: Render basic shapes using shaders and buffers.
- **"Cross"-Platform Support**: Support for Windows and Linux(Maybe) platforms.

## Requirements

- **Vulkan SDK**: Ensure you have the Vulkan SDK installed. Because without it, you're just wasting your time.
- **C++11 Compiler**: A C++11 compatible compiler is required.
- **Operating System**: Windows or Linux. Mac users don't deserve nice things.

## Installation

1. **Install Vulkan SDK**: Download and install the Vulkan SDK from the [official website](https://www.lunarg.com/vulkan-sdk/). Because installing SDKs is the best part of development.
2. **Clone the Repository**:
    ```sh
    git clone https://github.com/misterdown/wirender.git
    cd wirender
    ```
3. **Build**
    ```sh
    mkdir build
    cd build
    cmake ..
    cmake --build ./
    ```


## Contributing

Contributions are welcome! If you find any issues or have suggestions for improvements, please open an issue or submit a pull request. Because we ALL LOVE fixing bugs.

## License

This project is licensed under the [MIT License](LICENSE) file for details.

## Acknowledgments

- Thanks to the Vulkan community for their support and resources.

Happy rendering, future me.
