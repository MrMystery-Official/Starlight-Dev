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

	//ambient
	float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * objectColor.rgb;

	//diffuse
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

	// specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(lightPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

	vec3 result = (ambient + diffuse + specular) * objectColor.xyz;

	frag = vec4(result, objectColor.a);
	//frag = vec4(1.0, 0.0, 0.0, 1.0);
}