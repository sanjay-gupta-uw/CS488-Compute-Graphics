#version 330 core

flat in int SpriteIndex;
in vec2 TexCoords;

out vec4 FragColor;

uniform samplerBuffer SpriteUVS;
uniform sampler2D SpriteAtlas;

void main()
{
    vec4 spriteInfo = texelFetch(SpriteUVS, SpriteIndex);
    vec2 uv = TexCoords * spriteInfo.zw + spriteInfo.xy;
    FragColor = texture(SpriteAtlas, uv);
}
