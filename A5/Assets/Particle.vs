#version 330 core

layout(location = 0) in vec2 square_vert;
layout(location = 1) in vec4 transform_1;
layout(location = 2) in vec4 transform_2;
layout(location = 3) in vec4 transform_3;
layout(location = 4) in vec4 transform_4;
layout(location = 5) in vec4 color;

uniform mat4 VP;

out vec4 frag_color;

void main()
{
 mat4 transform = mat4(transform_1, transform_2, transform_3, transform_4); 
 gl_Position = VP * transform * vec4(square_vert.x, square_vert.y, 0.0f, 1.0f);
 frag_color = color;
}
