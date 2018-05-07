#version 300 es

precision mediump float;

in vec2 Tex;
out vec4 fragColor;

uniform sampler2D ourTexture;

void main()
{
	vec4 texel = texture(ourTexture, Tex);
	if (texel.a < 1.0)
		discard;
	fragColor = texel;
}
