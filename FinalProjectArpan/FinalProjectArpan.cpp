#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <FreeImage.h>
#include <map>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;


// namespace to define values for ImGui and other parameters
namespace values {
    float angle = 0.0f;
    float scale = 1.0f;
    float windowAspectRatio = 1.0f;
}

glm::vec3 position(0.0f, 2.5f, 10.0f);  // Camera position
glm::vec3 front(0.0f, 0.0f, -1.0f);     // Camera front vector
glm::vec3 up(0.0f, 1.0f, 0.0f);        // Camera up vector
glm::vec3 right;                       // Camera right vector (calculated dynamically)

float yaw = -90.0f;                    // Horizontal rotation
float pitch = 0.0f;                    // Vertical rotation
float movementSpeed = 2.5f;            // Movement speed (scale with deltaTime)
float mouseSensitivity = 0.1f;         // Mouse sensitivity
float lastX = 800.0f / 2.0f;           // Last mouse position
float lastY = 600.0f / 2.0f;           // Last mouse position
bool firstMouse = true;                // Flag to prevent jumping to center on first move
float deltaTime = 0.0f;                // Delta time between frames
float lastFrame = 0.0f;                // Last frame time

// Movement state flags
bool moveForward = false;
bool moveBackward = false;
bool moveLeft = false;
bool moveRight = false;

// Function to process input with keyboard callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_W)
            moveForward = true;
        if (key == GLFW_KEY_S)
            moveBackward = true;
        if (key == GLFW_KEY_A)
            moveLeft = true;
        if (key == GLFW_KEY_D)
            moveRight = true;
    }
    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_W)
            moveForward = false;
        if (key == GLFW_KEY_S)
            moveBackward = false;
        if (key == GLFW_KEY_A)
            moveLeft = false;
        if (key == GLFW_KEY_D)
            moveRight = false;
    }
}

// Function to process camera movement based on input flags
void processCameraMovement(float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    // Only update the x and z position to keep y constant
    if (moveForward) {
        position.x += front.x * velocity;  // Move along x based on front direction
        position.z += front.z * velocity;  // Move along z based on front direction
    }
    if (moveBackward) {
        position.x -= front.x * velocity;  // Move along x based on front direction
        position.z -= front.z * velocity;  // Move along z based on front direction
    }
    if (moveLeft) {
        position.x -= right.x * velocity;  // Move left along x based on right direction
        position.z -= right.z * velocity;  // Move left along z based on right direction
    }
    if (moveRight) {
        position.x += right.x * velocity;  // Move right along x based on right direction
        position.z += right.z * velocity;  // Move right along z based on right direction
    }
}

// Mouse movement callback function
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;  // Reversed because y-coordinates go from bottom to top in OpenGL
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Clamp the pitch to prevent flipping
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Update the front vector (yaw affects left-right movement, pitch affects up-down)
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));   // Horizontal direction
    newFront.y = sin(glm::radians(pitch));  // Vertical direction (pitch)
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));   // Horizontal direction

    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, up));  // Right vector is perpendicular to front and up
}

void calculateDeltaTime() {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}


// ImGui Function
void draw_gui(GLFWwindow* window) {
    // Begin ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Draw Gui
    ImGui::Begin("Arpan Prajapati");
    ImGui::Text("CGT 520 Final Project");
    ImGui::Text("");
    ImGui::Text("Features");
    ImGui::Text("");
    ImGui::Text("Winter Theme");

    ImGui::SliderFloat("Rotation angle", &values::angle, -glm::pi<float>(), +glm::pi<float>());
    ImGui::SliderFloat("Scale", &values::scale, -10.0f, +10.0f);
    if (ImGui::Button("Quit")) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    // End ImGui Frame
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Importing 3D Obj File Parameters
struct Vertex {
    float Position[3];
    float Normal[3];
    float TexCoords[2];
};

struct Mesh {
    unsigned int VAO, VBO, EBO;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int textureID;  // To store texture ID for the mesh
};

GLuint LoadTexture(const std::string& path) {
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(path.c_str());
    FIBITMAP* image = FreeImage_Load(format, path.c_str());
    if (!image) {
        std::cerr << "ERROR::Failed to load texture: " << path << std::endl;
        return 0;
    }

    FIBITMAP* image32bit = FreeImage_ConvertTo32Bits(image);
    unsigned char* data = FreeImage_GetBits(image32bit);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FreeImage_GetWidth(image32bit), FreeImage_GetHeight(image32bit), 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    FreeImage_Unload(image32bit);
    FreeImage_Unload(image);

    return texture;
}

// Helper function to process individual meshes
Mesh processMesh(aiMesh* mesh, const aiScene* scene, std::function<GLuint(const std::string&)> loadTexture) {
    Mesh myMesh;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position[0] = mesh->mVertices[i].x;
        vertex.Position[1] = mesh->mVertices[i].y;
        vertex.Position[2] = mesh->mVertices[i].z;

        vertex.Normal[0] = mesh->mNormals[i].x;
        vertex.Normal[1] = mesh->mNormals[i].y;
        vertex.Normal[2] = mesh->mNormals[i].z;

        if (mesh->mTextureCoords[0]) { // Check if the mesh has texture coordinates
            vertex.TexCoords[0] = mesh->mTextureCoords[0][i].x;
            vertex.TexCoords[1] = 1.0f - mesh->mTextureCoords[0][i].y;
            
            
        }
        else {
            vertex.TexCoords[0] = 0.0f;
            vertex.TexCoords[1] = 0.0f;
            std::cout << mesh<< "texcord absent" << std::endl;
        }

        myMesh.vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            myMesh.indices.push_back(face.mIndices[j]);
        }
    }

    // Load material and associated textures
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    aiString texturePath;
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
        std::string fullPath = (fs::current_path() / "assets" / std::string(texturePath.C_Str())).string();
        std::cout << "Texture Path: " << fullPath << std::endl;
        myMesh.textureID = LoadTexture(fullPath);
        if (myMesh.textureID == 0) {
            std::cerr << "WARNING::Texture loading failed for: " << fullPath << std::endl;
        }
    }
    else {
        std::cerr << "WARNING::Mesh has no diffuse texture!" << std::endl;
    }

    // Generate OpenGL buffers for the mesh
    glGenVertexArrays(1, &myMesh.VAO);
    glGenBuffers(1, &myMesh.VBO);
    glGenBuffers(1, &myMesh.EBO);

    glBindVertexArray(myMesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, myMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, myMesh.vertices.size() * sizeof(Vertex), myMesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, myMesh.indices.size() * sizeof(unsigned int), myMesh.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return myMesh;
}

// Main LoadModel function
std::vector<Mesh> LoadModel(const std::string& path) {
    Assimp::Importer importer;

    // Import the model file
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP: " << importer.GetErrorString() << std::endl;
        throw std::runtime_error("Failed to load model.");
    }

    std::vector<Mesh> meshes;           // Container for all meshes
    std::map<std::string, GLuint> textures_loaded; // Track loaded textures

    // Helper lambda to load a texture from the file system
    auto loadTexture = [&](const std::string& filePath) -> GLuint {
        if (textures_loaded.find(filePath) != textures_loaded.end()) {
            return textures_loaded[filePath];
        }

        GLuint textureID = LoadTexture(filePath);
        if (textureID != 0) {
            textures_loaded[filePath] = textureID;
        }
        return textureID;
        };

    // Recursive function to process all nodes in the scene
    std::function<void(aiNode*, const aiScene*)> processNode;
    processNode = [&](aiNode* node, const aiScene* scene) {
        // Process each mesh in the node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, loadTexture));
        }

        // Process each child node recursively
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
        };

    // Start processing from the root node
    processNode(scene->mRootNode, scene);

    return meshes;
}


// Linking Shader Files
struct ShaderProgramSource {
    std::string VertexSource;
    std::string FragmentSource;
};

static ShaderProgramSource ParseShader(const std::string& filepath) {
    std::ifstream stream(filepath);
    std::stringstream ss[2];
    std::string line;
    enum class ShaderType { NONE = -1, VERTEX = 0, FRAGMENT = 1 } type = ShaderType::NONE;

    while (getline(stream, line)) {
        if (line.find("#shader") != std::string::npos) {
            if (line.find("vertex") != std::string::npos) {
                type = ShaderType::VERTEX;
            }
            else if (line.find("fragment") != std::string::npos) {
                type = ShaderType::FRAGMENT;
            }
        }
        else {
            ss[(int)type] << line << '\n';
        }
    }

    return { ss[0].str(), ss[1].str() };
}

static unsigned int CompileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cerr << "Failed to compile shader\n" << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}

static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {

    glViewport(0, 0, width, height);
    if (height == 0)
    {
        height = 1;
    }
    values::windowAspectRatio = (float)width / (float)height;
    std::cout << "frame size changed!" << std::endl;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "ERROR::GLFW::INIT_FAILED" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1024, 1024, "Arpan Prajapati CGT 520", NULL, NULL);
    if (!window) {
        std::cerr << "ERROR::GLFW::WINDOW_CREATION_FAILED" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        std::cerr << "ERROR::GLEW::INIT_FAILED" << std::endl;
        return -1;
    }

    glfwSetWindowSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);  
    glfwSetKeyCallback(window, key_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    glEnable(GL_DEPTH_TEST);

    std::vector<Mesh> meshes = LoadModel("assets/snowman.obj");

    // Prepare shaders
    ShaderProgramSource source = ParseShader("shaders/shader_final.glsl");
    unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        calculateDeltaTime();  // Calculate deltaTime for smooth movement

        processCameraMovement(deltaTime);  // Move the camera based on input flags
        glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // Pass uniforms to the shader program
        glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
        glm::vec3 cameraPos(0.0f, 2.5f, 10.0f);

        glm::mat4 T = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 R = glm::rotate(values::angle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 S = glm::scale(glm::vec3(values::scale * 1.0f));

        draw_gui(window);

        glm::mat4 model = T * R * S;
        glm::mat4 view = glm::lookAt(position, position + front, up);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), values::windowAspectRatio, 0.1f, 100.0f);

        // Render all meshes
        glUseProgram(shader);

        glUniformMatrix4fv(glGetUniformLocation(shader, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(shader, "uLightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shader, "uViewPos"), 1, glm::value_ptr(cameraPos));
        glUniform3f(glGetUniformLocation(shader, "uLightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shader, "uObjectColor"), 1.0f, 0.5f, 0.31f);

        for (auto & mesh : meshes) {

            //std::cout << "Rendering mesh with texture ID: " << mesh.textureID << std::endl;
            // Bind the VAO for the mesh
            glBindVertexArray(mesh.VAO);

            // Activate and bind the texture for this mesh
            glActiveTexture(GL_TEXTURE0); // Activate texture unit 0
            glBindTexture(GL_TEXTURE_2D, mesh.textureID);

            // Pass the texture unit to the shader sampler
            glUniform1i(glGetUniformLocation(shader, "texture1"), 0);

            // Draw the mesh
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);

            // Unbind the VAO (optional for clarity)
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteProgram(shader);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
