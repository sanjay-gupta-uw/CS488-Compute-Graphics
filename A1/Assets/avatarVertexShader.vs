#version 330

layout(location = 0) in vec3 position;

uniform vec2 offset;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main()
{
	vec3 pos = position;
	pos += vec3(offset.x, 0, offset.y);
	gl_Position = P * V * M * vec4(pos, 1.0);
}
