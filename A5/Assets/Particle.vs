#version 330 core

layout(location = 0) in vec2 square_vert;
layout(location = 1) in vec4 transform_1;
layout(location = 2) in vec4 transform_2;
layout(location = 3) in vec4 transform_3;
layout(location = 4) in vec4 transform_4;
layout(location = 5) in int sprite_index;

uniform mat4 VP;

flat out int SpriteIndex;
out vec2 TexCoords;

void main()
{
 mat4 transform = mat4(transform_1, transform_2, transform_3, transform_4); 
 gl_Position = VP * transform * vec4(square_vert.x, square_vert.y, 0.0f, 1.0f);
 SpriteIndex = sprite_index;
 TexCoords = vec2(square_vert.x + 0.5, 1.0 - (square_vert.y + 0.5));
}
