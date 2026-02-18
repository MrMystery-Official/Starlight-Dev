#version 420 core

layout (location = 0) in vec3 _p0;
layout (location = 1) in vec3 _n0;
layout (location = 2) in vec4 _t0;
layout (location = 3) in vec2 _u0;
layout (location = 4) in mat4 _instanceMatrix;

// Imports the camera matrix from the main function
uniform mat4 camMatrix;

void main()
{
	gl_Position = camMatrix * _instanceMatrix * vec4(_p0, 1.0);
}