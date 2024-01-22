#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 offset;

uniform float scale;
uniform vec2 avatar_offset;
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

uniform bool is_avatar;

void main() {
	if (is_avatar) {
		vec3 pos = position;
		pos.x *= 0.5;
		pos.y *= 0.5;
		pos.z *= 0.5;
		pos += vec3(avatar_offset.x, 0, avatar_offset.y);
		gl_Position = P * V *M * vec4(pos, 1.0);
	}
	else {
		vec3 pos = position;
		pos.y *= scale;
		pos += vec3(offset.x, 0.0, offset.y);
		gl_Position = P * V * M * vec4(pos, 1.0);
	}
}
