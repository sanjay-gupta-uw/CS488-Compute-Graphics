Compilation: 
- I was able to compile my program in the virtual box with defaults (Mounted folder into VM and used the premake4/make combination within the CS488 and then A1 directories) 
- I did 'make clean' before zipping 

Manual: 
- Note: Used lookat and normalize to generate view matrix (-- piazza post)
- Camera is positioned and angles are non-zero at the start and at reset (pitch, yaw, roll)
- Clipped to near plane then performed perspective projection/divide -> introduced artificial zero for the cases where clip space points (after clipping against normalized cube) where at the edge of the boundary of the viewport (in NDCS)  
- Assumed screenshot.png is a screenshot of the A2 code directory 