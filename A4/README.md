Compilation: 
- I was able to compile my program in the virtual box with defaults (Mounted folder into VM and used the premake4/make combination within the CS488 and then A3 directories) 
- I did 'make clean' before zipping 
- Run code with from Asset dir: `.././a4 <script>.lua`

Manual: 
- Datastructure Changes: 
    - SceneNode: I mimicked my code for A3: added an additional matrix that doesn't include scaling (which i then used to pass down to child nodes); added parent node pointer 
    - Mesh: Augmented the Mesh class to include the transformation that can get the local space of the mesh centered at origin
    - Primitive: Added paramaters to support differentiating between primitives and access their positions/sizing
- Notes: 
    - For my extra feature, I implemented mirror reflections with a maximum recursion depth of 10 (see nonhier-bb.png, also note that sample.png == screenshot.png -- same image)
    - Assumptions: bounding volumes are applied to meshes, but from piazza, geonodes appear at leafs so no overall bounding volume for parent -> child  
    - Due to time constraints:
        - the colouring of the reflection in shadows is still appearing 
        - files involving hierarchal transformations are completely off 
        - placing the ellipse on the box would result in space between them: see hier.png
        - lighting in hier-scenes also exhibited weird behaviour

