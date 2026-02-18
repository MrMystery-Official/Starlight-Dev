#version 420 core

layout (location = 0) in vec4 _p0;
layout (location = 1) in vec3 _n0;
layout (location = 2) in vec4 _t0;
layout (location = 3) in vec2 _u0;
layout (location = 4) in mat4 _instanceMatrix;

// Outputs the texture coordinates to the fragment shader
out vec2 texCoord;
out vec3 Normal;
out vec3 FragPos;

// Imports the camera matrix from the main function
uniform mat4 camMatrix;

void main()
{
	// Outputs the positions/coordinates of all vertices
	gl_Position = camMatrix * _instanceMatrix * vec4(_p0.xyz, 1.0);
	// Assigns the texture coordinates from the Vertex Data to "texCoord"
	texCoord = _u0;
	FragPos = vec3(_instanceMatrix * vec4(_p0.xyz, 1.0));
	Normal = mat3(transpose(inverse(_instanceMatrix))) * _n0;  
}