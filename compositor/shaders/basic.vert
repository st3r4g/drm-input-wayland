#version 300 es

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
uniform mat4 matrix;

out vec3 Color;

void main()
{
	gl_Position = matrix * vec4(position, 1.0);
	Color = color;
}
