#version 410 core
out vec4 oFragColor;
in vec2 FragMateTexCoords;
uniform sampler2DArray uMaterialAlb;
uniform usampler2D uLayers;
uniform sampler2D uTexCoords;

uniform int uRenderLayerA;
uniform int uRenderLayerB;

void main()
{
    vec4 layerData = texture(uLayers, FragMateTexCoords);
    float layer0 = layerData.r;
    float layer1 = layerData.g;

    if(int(layer0) == 120 || int(layer1) == 120)
        discard;

    // Get mate texture dimensions and convert to float
    ivec2 texSize = textureSize(uLayers, 0);
    vec2 texSizeF = vec2(texSize);

    // Compute the base coordinate in texel space from the normalized mate coordinate
    vec2 baseExactCoord = FragMateTexCoords * texSizeF;

    // Use gl_FragCoord and derivatives to capture the sub-pixel offset
    vec2 subpixelOffset = fract(gl_FragCoord.xy) - 0.5;
    vec2 d = fwidth(FragMateTexCoords * texSizeF);
    vec2 fragPos = baseExactCoord + subpixelOffset * d; // precise fragment position in texel space

    // Determine the base texel coordinate
    ivec2 baseCoord = ivec2(floor(fragPos));
    ivec2 maxCoord = texSize - ivec2(1, 1);


    if(baseCoord.x + 1 >= maxCoord.x || baseCoord.y + 1 >= maxCoord.y)
        discard;

    //if(baseCoord.x <= 1 || baseCoord.y <= 1)
        //discard;

    // Interpolate texture coordinates for texture blending
    ivec2 coord00 = clamp(baseCoord, ivec2(0), maxCoord);
    ivec2 coord10 = clamp(baseCoord + ivec2(1, 0), ivec2(0), maxCoord);
    ivec2 coord01 = clamp(baseCoord + ivec2(0, 1), ivec2(0), maxCoord);
    ivec2 coord11 = clamp(baseCoord + ivec2(1, 1), ivec2(0), maxCoord);
    
    vec4 tc00 = texelFetch(uTexCoords, coord00, 0);
    vec4 tc10 = texelFetch(uTexCoords, coord10, 0);
    vec4 tc01 = texelFetch(uTexCoords, coord01, 0);
    vec4 tc11 = texelFetch(uTexCoords, coord11, 0);
    
    vec4 tc0 = mix(tc00, tc10, fract(fragPos.x));
    vec4 tc1 = mix(tc01, tc11, fract(fragPos.x));
    vec4 texCoordTexture = mix(tc0, tc1, fract(fragPos.y));
    
    // Sample terrain textures
    vec4 texColor0 = texture(uMaterialAlb, vec3(texCoordTexture.xy, layer0));
    vec4 texColor1 = texture(uMaterialAlb, vec3(texCoordTexture.zw, layer1));

    // Gaussian blend calculation from your original shader
    float sigma = 0.5;
    float sumWeights = 0.0;
    float sumMaterialWeight = 0.0;

    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            ivec2 sampleCoord = clamp(baseCoord + ivec2(i, j), ivec2(0), maxCoord);
            float sampleVal = float(texelFetch(uLayers, sampleCoord, 0).b) / 255.0;
            vec2 texelCenter = vec2(sampleCoord) + 0.5;
            float dist = length(fragPos - texelCenter);
            float weightFactor = exp(- (dist * dist) / (2.0 * sigma * sigma));
            sumWeights += weightFactor;
            sumMaterialWeight += sampleVal * weightFactor;
        }
    }
    float materialWeight = sumMaterialWeight / sumWeights;
    
    if(uRenderLayerA == 1 && uRenderLayerB == 1) {
        oFragColor = mix(texColor0, texColor1, materialWeight);
    } else if(uRenderLayerA == 1) {
        oFragColor = mix(texColor0, vec4(0.0, 0.0, 0.0, 0.0), materialWeight);
    } else if(uRenderLayerB == 1) {
        oFragColor = mix(vec4(0.0, 0.0, 0.0, 0.0), texColor1, materialWeight);
    } else {
        oFragColor = vec4(1.0, 0.0, 1.0, 1.0);
    }
    
}