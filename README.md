# Vulkan Renderer

A small tool for drawing basic shapes using the Vulkan API.

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [License](#License)
- [Acknowledgments](#Acknowledgments)

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
    git clone https://github.com/yourusername/wirender.git
    cd wirender
    ```

## Usage

1. **Include the Header File**: Include the `render_manager.hpp` header file in your project. Because who needs documentation?
2. **Create a Window**: Create a `window_info` object containing the necessary information for creating a window on your platform (Windows or Linux).
3. **Initialize the Render Manager**: Create a `render_manager` object using the `window_info` object.
4. **Create Shaders**: Use the `shader_builder` object to create and manage shaders.
5. **Create Buffers**: Use the `buffer_builder` object to create vertex/index buffers.

## Contributing

Contributions are welcome! If you find any issues or have suggestions for improvements, please open an issue or submit a pull request. Because we all love fixing bugs and improving code.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Thanks to the Vulkan community for their support and resources.

Happy rendering, future me.
