#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 offset;

uniform float scale;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

void main() {
	vec3 pos = position + vec3(offset.x, 0.0, offset.y);
	pos.y *= scale;
	gl_Position = P * V * M * vec4(pos, 1.0);
}
