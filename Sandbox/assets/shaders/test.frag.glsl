#version 440 core

layout (location = 0) in vec3 pass_Posn;
layout (location = 0) out vec4 o_Color;

void main()
{
	o_Color = vec4(0.5,0.5,0.5, 1.0);
};