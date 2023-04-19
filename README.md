# RiveQtQuickPlugin

RiveQtQuickPlugin is a Qt/QtQuick renderer that enables the integration of Rive animations in QML. For more information on Rive, visit [https://www.rive.app](https://www.rive.app).

The plugin allows you to display Rive animations, interact with them, and bind data to and from the animations.

https://user-images.githubusercontent.com/1797537/232922391-5a1f398a-9908-446a-bd2c-b2d7788c4f48.mp4

## Usage

Here's a short example of how to use the RiveQtQuickItem in your QML code:

```qml
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
...
```

By default, the interactive property is set to true, which means that mouse events are propagated to Rive. You can activate triggers with the following code:

```
riveItem.stateMachineInterface.activateTrigger("TRIGGER_NAME")
```

You can also create bindings to read and write data to and from Rive animations:
The name of the property is whatever is configured in the animation
Looking at the Object.keys function allows you to check for the names, in case you dont know them.

Note that stateMachineInterface will be null as long as no StateMachine was selected.
Note that rive animations come with and without stateMachines, also note that rive animations may not have a default state machine so you may have to select it initialy.

```
Text {
    text: riveItem.stateMachineInterface.InputName
}
```

For more details, check out the example project. 

Keep in mind that this is an initial release, so there will be some rough edges and the interface of the RiveQtQuickItem is subject to change. 
Currently, the plugin has only been tested on Windows using the MSVC 2019 compiler and Qt 5.

# RiveQtQuickItem Overview

The RiveQtQuickItem is item designed to display Rive animations and art in Qt Quick applications using the Rive C++ library. 

It inherits from the QQuickItem class.It exposes several key properties that allow you to customize the appearance and behavior of the Rive animation:

- `fileSource`: The path to the Rive file containing the animation or art to display.
- `currentArtboardIndex`: The index of the current artboard.
- `currentStateMachineIndex`: The index of the current state machine.
- `interactive`: A boolean value indicating whether the Rive animation should be interactive.

The RiveQtQuickItem also provides read-only properties, such as artboards, animations, and stateMachines, which return lists of available artboards, animations, and state machines, respectively. 
Additionally, it emits signals like `fileSourceChanged`, `currentArtboardIndexChanged`, and `currentStateMachineIndexChanged` that allow you to monitor changes in the animation's state.
