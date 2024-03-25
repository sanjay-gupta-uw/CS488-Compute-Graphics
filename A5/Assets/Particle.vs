#version 330 core

layout(location = 0) in vec3 square_vert;
layout(location = 1) in vec4 transform_1;
layout(location = 2) in vec4 transform_2;
layout(location = 3) in vec4 transform_3;
layout(location = 4) in vec4 transform_4;
layout(location = 5) in vec4 colour;

out vec4 particlecolor;

uniform mat4 VP;

void main()
{
 mat4 model_transform = mat4(transform_1, transform_2, transform_3, transform_4);
 gl_Position = VP * model_transform * vec4(square_vert, 1.0f);
 particlecolor = colour;
}
