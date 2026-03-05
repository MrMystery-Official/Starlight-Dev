#version 330 core

layout (location = 0) out vec4 frag;

in vec2 texCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texAlb0;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec4 previewTint;
uniform float previewAlpha;

void main()
{
    vec4 objectColor = texture(texAlb0, texCoord);
    if (objectColor.a < 0.05)
    {
        discard;
    }

    vec3 tintedColor = mix(objectColor.rgb, previewTint.rgb, previewTint.a);

    float ambientStrength = 0.6;
    vec3 ambient = ambientStrength * tintedColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * lightColor) * 0.75;

    vec3 result = (ambient + diffuse) * tintedColor;
    frag = vec4(result, objectColor.a * previewAlpha);
}
