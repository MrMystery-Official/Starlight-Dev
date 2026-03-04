#version 330 core

out vec4 FragColor;
uniform vec3 tintColor;
uniform float opacity;

void main()
{
	FragColor = vec4(tintColor, opacity);
}
