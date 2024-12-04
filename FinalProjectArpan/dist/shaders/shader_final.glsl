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

in vec2 vTexCoords;      // Texture coordinates from vertex shader

out vec4 FragColor;     // Output color of the fragment

uniform sampler2D texture1;  // Texture sampler

void main()
{
    // Simply use the texture color based on the texture coordinates
    FragColor = texture(texture1, vTexCoords);
}