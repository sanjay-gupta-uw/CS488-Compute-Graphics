#version 330

struct LightSource {
    vec3 position;
    vec3 rgbIntensity;
};

in VsOutFsIn {
	vec3 position_ES; // Eye-space position
	vec3 normal_ES;   // Eye-space normal
	LightSource light;
} fs_in;


out vec4 fragColour;

uniform vec3 color;

void main() {
	vec3 light_dir = normalize(fs_in.light.position - fs_in.position_ES);
    float intensity = dot(fs_in.normal_ES, light_dir);

	float steps = 1.0;
	if (intensity > 0.5) {
		steps = 0.75;
	} else if (intensity > 0.25) {
		steps = 0.5;
	} else if (intensity > 0.1) {
		steps = 0.25;
	} else {
		steps = 0.1;
	}

	vec3 toon = color * steps;
	fragColour = vec4(toon, 1.0);
}
