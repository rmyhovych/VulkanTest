#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vInPosition;
layout(location = 1) in vec3 vInColor;
layout(location = 2) in vec2 vTexCoord;

layout(location = 0) out vec3 fragmentColor;
layout(location = 1) out vec2 fragTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main()
{
	fragTexCoord = vTexCoord;
	fragmentColor = vInColor;
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(vInPosition, 1.0);
}