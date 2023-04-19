# RiveQtQuickPlugin

RiveQtQuickPlugin is a Qt/QtQuick renderer that enables the integration of Rive animations in QML. For more information on Rive, visit [https://www.rive.app](https://www.rive.app).

This plugin allows you to display Rive animations, interact with them, and bind data to and from the animations in your Qt Quick applications.

https://user-images.githubusercontent.com/1797537/233192955-7360403b-b51b-422a-8770-20d504c130c0.mp4


## Features

- Display Rive animations in QML
- Interactive animations with support for user input and triggers
- Data bindings for reading and writing data to and from Rive animations
- Support for managing multiple artboards, animations, and state machines

## Dependencies

- Qt 5 or later
- Rive C++ Runtime (available at [https://github.com/rive-app/rive-cpp](https://github.com/rive-app/rive-cpp))

## Building

1. Clone this repository:

```
git clone https://github.com/jebos/RiveQtQuickPlugin.git
```

You can download and build the Rive C++ Runtime, following the instructions in the Rive C++ Runtime repository.
You can also just use the CMake I put into 3rdParty, it will download, build and link rive-cpp and its dependencies

2. Just build using cmake and make.

## Usage

Here's a short example of how to use the RiveQtQuickItem in your QML code:

```
import RiveQtQuickPlugin 1.0

...

RiveQtQuickItem {
    id: riveItem

    width: 800
    height: 800

    fileSource: "YOUR_RIVE_FILE"

    onStateMachineInterfaceChanged: {
        // This object provides a list of all possible properties of the file.
        // These properties can be used in bindings and updated through bindings!
        console.log(Object.keys(riveItem.stateMachineInterface))
    }
}

```

## Example Project

Check out the "example" folder in this repository for a complete example project demonstrating how to use the `RiveQtQuickPlugin`.

## Compatibility

The plugin has been tested on Windows using the MSVC 2019 compiler and Qt 5. To ensure broader compatibility, test the plugin on different platforms, compilers, and Qt versions, and address any compatibility issues that may arise.

## Contributing

Contributions are welcome! If you encounter any issues or would like to request a feature, feel free to open an issue or submit a pull request.

## License

This project is released under the MIT License.
