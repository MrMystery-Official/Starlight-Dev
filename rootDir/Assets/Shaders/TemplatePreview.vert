#version 330 core

layout (location = 0) in vec4 _p0;
layout (location = 1) in vec3 _n0;
layout (location = 2) in vec4 _t0;
layout (location = 3) in vec2 _u0;
layout (location = 4) in mat4 _instanceMatrix;

out vec2 texCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 camMatrix;

void main()
{
    gl_Position = camMatrix * _instanceMatrix * vec4(_p0.xyz, 1.0);
    texCoord = _u0;
    FragPos = vec3(_instanceMatrix * vec4(_p0.xyz, 1.0));
    Normal = normalize(mat3(transpose(inverse(_instanceMatrix))) * _n0);
}
