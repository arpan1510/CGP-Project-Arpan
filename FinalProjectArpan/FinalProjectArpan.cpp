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


// namespace to define values for ImGui and other parameters
namespace values {
    float angle = 0.0f;
    float scale = 1.0f;
    float windowAspectRatio = 1.0f;
}

//ImGui Function
void draw_gui(GLFWwindow* window)
{
    //Begin ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //Draw Gui
    ImGui::Begin("Arpan Prajapati");
    ImGui::Text("CGT 520 Final Project");
    ImGui::Text("");
    ImGui::Text("Features");
    ImGui::Text("");
    ImGui::Text("Winter Theme");
    
    ImGui::SliderFloat("Rotation angle", &values::angle, -glm::pi<float>(), +glm::pi<float>());
    ImGui::SliderFloat("Scale", &values::scale, -10.0f, +10.0f);
    if (ImGui::Button("Quit"))
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    //End ImGui Frame
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
};

std::vector<Mesh> LoadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        throw std::runtime_error("Failed to load model.");
    }

    std::vector<Mesh> meshes;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        Mesh myMesh;

        // Process vertices
        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            Vertex vertex;
            vertex.Position[0] = mesh->mVertices[j].x;
            vertex.Position[1] = mesh->mVertices[j].y;
            vertex.Position[2] = mesh->mVertices[j].z;

            vertex.Normal[0] = mesh->mNormals[j].x;
            vertex.Normal[1] = mesh->mNormals[j].y;
            vertex.Normal[2] = mesh->mNormals[j].z;

            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords[0] = mesh->mTextureCoords[0][j].x;
                vertex.TexCoords[1] = mesh->mTextureCoords[0][j].y;
            }
            else {
                vertex.TexCoords[0] = 0.0f;
                vertex.TexCoords[1] = 0.0f;
            }
            myMesh.vertices.push_back(vertex);
        }

        // Process indices
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                myMesh.indices.push_back(face.mIndices[k]);
            }
        }

        // Generate OpenGL buffers
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

        meshes.push_back(myMesh);
        std::cout << "Mesh vertices: " << myMesh.vertices.size() << std::endl;

    }

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
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader\n";
        std::cout << message << std::endl;
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

// main method
int main() {
    // Initialize GLFW
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1024, 1024, "Arpan Final Project CGT 520", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Error initializing GLEW\n";
        return -1;
    }

    glfwSetWindowSizeCallback(window, framebuffer_size_callback);

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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // Pass uniforms to the shader program
        glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
        glm::vec3 cameraPos(0.0f, 0.0f, 10.0f);

        glm::mat4 T = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 R = glm::rotate(values::angle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 S = glm::scale(glm::vec3(values::scale * 1.0f));

        glm::mat4 model = T * R * S;
        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), values::windowAspectRatio, 0.1f, 100.0f);

        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(shader, "uLightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shader, "uViewPos"), 1, glm::value_ptr(cameraPos));
        glUniform3f(glGetUniformLocation(shader, "uLightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shader, "uObjectColor"), 1.0f, 0.5f, 0.31f);

        for (const auto& mesh : meshes) {
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
        }

        draw_gui(window);

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
