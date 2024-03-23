Compilation: 
- I was able to compile my program in the virtual box with defaults (Mounted folder into VM and used the premake4/make combination within the CS488 and then A3 directories) 
- I did 'make clean' before zipping 

Manual: 
- Notes: I created my model based on Luffy from onepiece. Specifically, i modelled the asset in blender and then applied subdivision modifiers/triangulate. From there i separated the mesh into different fragments (arm, leg fragments, etc.) which can be found in the assets folder, and set up the objects origins to be where the hinge/joints would be. These were then put together in the Lua code (some variation/not perfectly fit for some regions due to time constraints). 
- GUI NOTES: Clicking on the menu bar will result in a drop down with the options
- Datastructure Changes: 
    - SceneNode: I added an additional matrix that doesn't include scaling (which i then used to pass down to child nodes); added parent node pointer; 
    - Phong.fs: added picking bool to emulate picking example
    - JointNode: Added function to calculate/return rotation matrix with previous transformations from ancestor nodes; added initial angle param to update the struct one each time   

- Credit: Used the demo code for trackball rotation and picking 

- Overview of Hierarchical model: 
   - Root node is connected to the neckMesh, joint connecting the shoulders/torso to the neck, and the joint connecting the neck to the head.
   - Connected to the neck-head joint is the head mesh
   - Connected to the neck-torso joint is the torso, shoulder joints, and hip joints
   - Then the structure for the arms and legs follow a similar pattern: upper part to the hip/shoulder joint, middle part (forearm/shin) child to the elbow/knee joint, where these joints are children of the preceding joint, and then the hand/feet exteremities as children to the wrist/ankle joint, which is tied to the elbow/knee joint
   - Overall, there are 14 joints, and 15 DOF's (Head-Neck joint has 2 DOF, torso-neck is the third). 
        - This ensures that a parent of a geo node is not another geo node and internal nodes are joint nodes
