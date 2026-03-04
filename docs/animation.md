# Animation Specification

The animation of Rubik's Cube is the core feature of the project. The system 
can be described in a Finite State Machine model with states of `idle` and 
`moving`. 

- When performing keyboard controlled moving, pressing a key will trigger a 
transition from `idle` to `moving` state. During the `moving` animation, any 
moving operation will be ignored. Once the animation is finished, the system 
will return to `idle` state.

- When performing file controlled moving, the system will read the control 
sequence from the file and perform the animation. Once a move is done, the 
system will check if there are more moves in the queue to decide whether loop
to `moving` state or return to `idle` state. During `moving` state, any moving
operation will be ignored. Once the animation is finished, the system will 
return to `idle` state.

- Note: the moving system and camera system are independent of each other. That 
is to say, the camera control an be performed at any time, even during the 
moving animation. 
