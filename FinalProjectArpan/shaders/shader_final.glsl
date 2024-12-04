#shader vertex
#version 330 core

layout(location = 0) in vec3 aPosition; // Vertex position
layout(location = 1) in vec3 aNormal;   // Vertex normal
layout(location = 2) in vec2 aTexCoords; // Vertex texture coordinates

uniform mat4 uModel;      // Model matrix
uniform mat4 uView;       // View matrix
uniform mat4 uProjection; // Projection matrix

out vec3 vNormal;         // Pass the normal to the fragment shader
out vec2 vTexCoords;      // Pass the texture coordinates to the fragment shader
out vec3 vFragPosition;   // Pass the fragment position in world space

void main() {
    vFragPosition = vec3(uModel * vec4(aPosition, 1.0)); // Compute fragment position
    vNormal = mat3(transpose(inverse(uModel))) * aNormal; // Correctly transform normals
    vTexCoords = aTexCoords;

    gl_Position = uProjection * uView * vec4(vFragPosition, 1.0);
}


#shader fragment
#version 330 core

in vec3 vNormal;        // Interpolated normal from the vertex shader
in vec2 vTexCoords;     // Interpolated texture coordinates from the vertex shader
in vec3 vFragPosition;  // Fragment position in world space

out vec4 FragColor;

uniform sampler2D uTexture;  // Texture sampler
uniform vec3 uLightPos;      // Position of the light source
uniform vec3 uViewPos;       // Position of the camera/viewer
uniform vec3 uLightColor;    // Color of the light
uniform vec3 uObjectColor;   // Base color of the object (used if no texture)

void main() {
    // Ambient lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * uLightColor;

    // Diffuse lighting
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPosition);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(uViewPos - vFragPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * uLightColor;

    // Combine all components
    vec3 lighting = (ambient + diffuse + specular);

    // Sample texture color and apply lighting
    vec4 texColor = texture(uTexture, vTexCoords);
    vec3 finalColor = (texColor.rgb * lighting) * uObjectColor;

    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Use texture alpha for transparency
}