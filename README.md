<!--
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later
-->
# RiveQtQuickPlugin

RiveQtQuickPlugin is a Qt/QtQuick renderer that enables the integration of Rive animations in QtQuick / QML. 
For more information on Rive, visit [https://www.rive.app](https://www.rive.app).

This plugin allows you to display Rive animations, interact with them, and bind data to and from the animations in your Qt Quick applications.

https://user-images.githubusercontent.com/1797537/233192955-7360403b-b51b-422a-8770-20d504c130c0.mp4


## Features

- Display Rive animations in QML
- OpenGL Rendering
- Software Rendering
- Interactive animations with support for user input and triggers
- Data bindings for reading and writing data to and from Rive animations
- Support for managing multiple artboards, animations, and state machines

## Compatibility
The plugin has been tested on:

- Windows using the MSVC 2019 compiler and Qt 5.15.2

- Linux (Ubuntu 22.04 LTS) using Clang14 and Qt 5.15.2, Qt 6.5.0

  - Ensure that all clang dependencies are installed 

  - `apt-get install clang-format clang-tidy clang-tools clang clangd libc++-dev libc++1 libc++abi-dev libc++abi1 libclang-dev libclang1 liblldb-dev libllvm-ocaml-dev libomp-dev libomp5 lld lldb llvm-dev llvm-runtime llvm python3-clang`

  - Use the automatic installation script if you get issues: https://apt.llvm.org/ 
  - `chmod +x llvm.sh` and `sudo ./llvm.sh 14`

## Dependencies

- Qt 5.15.2 or Qt 6.5.0
- Rive C++ Runtime (available at [https://github.com/rive-app/rive-cpp](https://github.com/rive-app/rive-cpp))
  * **Hint:** You don't have to compile the it from the repo. Cmake will download rive-cpp and its dependencies and build them using cmake. The result is a static lib the plugin links against.

## Building

- Install missing dependencies (`libxkbcommon-dev` might be missing on Ubuntu 22.04)
- Building using cmake and make should work just fine

```example
cd /path/to/repo/

export CC=clang
export CXX=clang++

mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=~/.Qt/6.5.1/gcc_64 -S .. -B .
make
```

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

### Available Properties

| Name                     | Type             | R/W | Description |
|--------------------------|------------------|-----|---------------------------------------------------------------------------------------------------------------------------------------|
| fileSource               | string           | R/W | File path to the file that shall be loaded. An empty string unloads the current file.                                                 |
| loadingStatus            | int (enum)       | RO  | Current loading state of the rive item.                                                                                               |
| artboards                | list of struct   | R/W | Artboard information of the loaded rive file.                                                                                         |
| animations               | list of struct   | R/W | Animation information of the loaded rive file.                                                                                        |
| stateMachines            | list of struct   | R/W | State machine information of the loaded rive file.                                                                                    |
| currentArboardIndex      | int              | R/W | Specifies the currently displayed artboard. Can be used to overwrite the internal default artboard, which is often not set correctly. |
| currentStateMachineIndex | int              | R/W | Specifies the currently active state machine. Can be used to overwrite the internal default state machine.                            |
| interactive              | bool             | R/W | If true (default), mouse buttons are are accepted to interact with the items triggers.                                                |
| stateMachineInterface    | QML Property Map | RO  | Contains the triggers of the current state machine.                                                                                   |
| renderQuality            | int (enum)       | R/W | Specifies the render quality (e.g. to set the number of vertices per line).                                                           |
| fillMode                 | int (enum)       | R/W | Specified how the artboard is rendered into the qml item. Available options are: PreserveAspectFit, PreserveAspectCrop, Stretch.      |


## Logging

There are 4 logging categories, so it is easy to filter relevant output:

- **rqqpItem:** Logs related to the RiveQtQuickItem, like file loading, properties, etc.
- **rqqpRendering:** Logs related to rendering and the rendering classes
- **rqqpInspection:** Logs related to the content of the rive file like artboards, animations, statemachine info.
- **rqqpInteraction:** Logs related to user interaction with the item and rive content

## Example Project

Check out the "example" folder in this repository for a complete example project demonstrating how to use the `RiveQtQuickPlugin`.

### Linux

In order to run the example project, set the import path correctly

Qt 5.15.2:

`export QML2_IMPORT_PATH=YOUR/BUILD/DIR/binary/`

Qt 6.5.0:

`export QML_IMPORT_PATH=YOUR/BUILD/DIR/binary/`

**Hint:** In QtCreator you can set the variable(s) in the project settings under "Environment"

### Windows

Everything is deployed in your build directory into a directory called "binary"

## Contributing

Contributions are welcome! If you encounter any issues or would like to request a feature, feel free to open an issue or submit a pull request.

## License

This project is released under the LGPLv3.0-or-later License.
