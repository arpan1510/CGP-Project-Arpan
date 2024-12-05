#shader vertex
#version 330 core

layout(location = 0) in vec3 aPosition;  // Vertex position
layout(location = 1) in vec3 aNormal;    // Vertex normal
layout(location = 2) in vec2 aTexCoords; // Vertex texture coordinates

out vec3 vNormal;         // Normal for fragment shader
out vec2 vTexCoords;      // Texture coordinates for fragment shader
out vec3 TexCoords;       // Skybox texture coordinates

uniform mat4 uModel;      // Model matrix
uniform mat4 uView;       // View matrix
uniform mat4 uProjection; // Projection matrix

uniform bool isSkybox;    // Toggle for skybox rendering

void main() {
    if (isSkybox) {
        TexCoords = aPosition;
        gl_Position = uProjection * uView * vec4(aPosition, 1.0);
    } else {
        vNormal = mat3(transpose(inverse(uModel))) * aNormal; // Normal in world space
        vTexCoords = aTexCoords;                             // Pass texture coordinates
        gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    }
}


#shader fragment
#version 330 core

in vec3 vNormal;          // Normal vector
in vec2 vTexCoords;       // Texture coordinates
in vec3 TexCoords;        // Skybox texture coordinates

out vec4 FragColor;       // Output fragment color

uniform vec3 uLightPos;   // Light position
uniform vec3 uViewPos;    // Camera position
uniform vec3 uLightColor; // Light color
uniform vec3 uObjectColor; // Object color

uniform sampler2D texture1; // Texture sampler for meshes
uniform samplerCube skybox; // Skybox cubemap sampler

uniform bool isSkybox;    // Toggle for skybox rendering

void main() {
    if (isSkybox) {
        // Skybox rendering: no blending, just use the cubemap
        //FragColor = vec4(1.0f);
        FragColor = texture(skybox, TexCoords);
    } else {
        // Mesh rendering: sample the mesh texture
        vec4 textureColor = texture(texture1, vTexCoords);
        
        // Blending the mesh color over the skybox (no lighting)
        FragColor = textureColor; // This will blend the mesh normally over the background
    }
}
