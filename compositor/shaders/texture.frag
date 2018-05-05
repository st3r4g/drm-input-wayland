#version 300 es

precision mediump float;

in vec2 Tex;
out vec4 fragColor;

uniform sampler2D ourTexture;

void main()
{
	fragColor = texture(ourTexture, Tex);
}
