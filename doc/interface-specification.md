# User Interface Specification

## Keyboard Input Specification

This project uses following Move Notation.

- F: Front face
- B: Back face
- U: Up face
- D: Down face
- L: Left face
- R: Right face

- pressing <key> of ('f', 'b', 'u', 'd', 'l', 'r'), the cube will be rotated around the corresponding face clockwise.
- pressing <Shift> + <key> of ('f', 'b', 'u', 'd', 'l', 'r'), the cube will be rotated around the corresponding face counterclockwise.
Note: any 180 degree rotation is not allowed to be performed, instead, perform two 90 degree rotations in the same direction.

Advanced Features:

- Undo: press <Ctrl> + <Z> to undo the last move. could also be triggered by the undo button in the UI.
- Redo: press <Ctrl> + <Y> to redo the last undone move. could also be triggered by the redo button in the UI

## Mouse Control Specification

- Zoom in & out: use the scroll wheel to zoom in & out.
- Rotate View(Orbit): left click & drag to rotate the cube. 
- Pan View: <Shift> + left click & drag.


## File I/O Specification

a file of control sequence in .json format can be read/saved from/to the program.

an example file is as follows:

```json
{
    "control_sequence": [
        "f",
        "F",
        "b",
        "B",
        "u",
        "U",
        "d",
        "D",
        "l",
        "r"
    ]
}
```

all lowercase letters are for 90 degree clockwise rotation, and all uppercase letters are for 90 degree counterclockwise rotation.



