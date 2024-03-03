#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;

// Inputs the texture coordinates from the Vertex Shader
in vec2 texCoord;

// Gets the Texture Unit for the first texture
uniform sampler2D Texture;

void main()
{
    // Original texture color
    vec4 originalColor = texture(Texture, texCoord);

    // Transparent blue overlay color
    vec3 blueOverlay = vec3(0.475, 0.305, 0.035); // Adjust the alpha (last component) for transparency

    // Add blue overlay directly to the RGB components of the original color
    vec3 combinedRGB = originalColor.rgb + blueOverlay.rgb;
    
    // Combine the modified RGB components with the original alpha
    FragColor = vec4(combinedRGB, originalColor.a);
}
