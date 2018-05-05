#version 300 es

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 tex;
uniform mat4 matrix;

out vec2 Tex;

void main()
{
	gl_Position = matrix * vec4(position, 0.0, 1.0);
	Tex = tex;
}
