#version 420 core

// Outputs colors in RGBA
layout (location = 0) out vec4 frag;

// Inputs the texture coordinates from the Vertex Shader
in vec2 texCoord;
in vec3 Normal;
in vec3 FragPos;

// Gets the Texture Unit from the main function
uniform sampler2D texAlb0;

uniform vec3 lightColor;
uniform vec3 lightPos;

void main()
{
	vec4 objectColor = texture(texAlb0, texCoord);

	if(objectColor.a < 0.5)
		discard;

	//ambient
	float ambientStrength = 0.6;
    vec3 ambient = ambientStrength * objectColor.rgb;

	//diffuse
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * lightColor) * 0.75;

	vec3 result = (ambient + diffuse) * objectColor.xyz;

	frag = vec4(result, objectColor.a);
	//frag = vec4(1.0, 0.0, 0.0, 1.0);
}