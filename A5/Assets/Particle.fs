#version 330 core

in vec4 frag_color;
uniform vec3 backgroundColor;

out vec4 FragColor;

void main()
{
    float alpha = frag_color.w;
    FragColor = vec4(frag_color.xyz * alpha + backgroundColor.xyz * (1.0 - alpha), 1.0);
}
