Compilation: 
- I was able to compile my program in the virtual box with defaults (Mounted folder into VM and used the premake4/make combination within the CS488 and then A1 directories) 
- I did 'make clean' before zipping 

Manual: 
- Added multiple shader files to isolate rendering the grid/floor, maze, and avatar
- Implemented avatar by using an algorithm to generate a Icosphere (in sphere.hpp)
- Assumed the floor is its own unique colour opposed to changing with the grid (as such, assumed the grid colour doesnt change)