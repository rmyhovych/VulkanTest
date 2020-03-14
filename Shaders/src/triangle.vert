#version 450

layout(location = 0) in vec3 vInPosition;
layout(location = 1) in vec3 vInColor;

layout(location = 0) out vec3 fragmentColor;

void main()
{
	fragmentColor = vInColor;
	gl_Position = vec4(vInPosition, 1.0);
}