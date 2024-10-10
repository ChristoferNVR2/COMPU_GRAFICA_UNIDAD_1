#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <csignal>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef __linux__
    #define debugBreak() raise(SIGTRAP)
#elif _WIN32
    #define debugBreak() __debugbreak()
#endif

#define ASSERT(x) if (!(x)) debugBreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__));

static void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line) {
    while (GLenum error = glGetError()) {
        std::cout << "[OpenGL Error] (" << error << "): " << function << " " << file << ":" << line << std::endl;
        return false;
    }
    return true;
}

struct ShaderProgramSource {
    std::string VertexSource;
    std::string FragmentSource;
};

static ShaderProgramSource ParseShader(const std::string& filepath) {
    std::ifstream stream(filepath);
    enum class ShaderType {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;
    while (getline(stream, line)) {
        if (line.find("#shader") != std::string::npos) {
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::VERTEX;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::FRAGMENT;
        } else {
            ss[static_cast<int>(type)] << line << '\n';
        }
    }
    return {
        ss[static_cast<int>(ShaderType::VERTEX)].str(),
        ss[static_cast<int>(ShaderType::FRAGMENT)].str()
    };
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
        char* message = static_cast<char*>(alloca(length * sizeof(char)));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
    GLCall(unsigned int program = glCreateProgram());
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    GLCall(glAttachShader(program, vs));
    GLCall(glAttachShader(program, fs));
    GLCall(glLinkProgram(program));
    GLCall(glValidateProgram(program));
    GLCall(glDeleteShader(vs));
    GLCall(glDeleteShader(fs));

    return program;
}

int main() {
    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(640, 480, "3D Cube", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        std::cout << "Error initializing GLEW!" << std::endl;
        return -1;
    }

    // Enable depth test for correct 3D rendering
    glEnable(GL_DEPTH_TEST);

    float positions[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        // Back face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };

    unsigned int indices[] = {
        // Front
        0, 1, 2, 2, 3, 0,
        // Back
        4, 5, 6, 6, 7, 4,
        // Left
        0, 3, 7, 7, 4, 0,
        // Right
        1, 2, 6, 6, 5, 1,
        // Top
        3, 2, 6, 6, 7, 3,
        // Bottom
        0, 1, 5, 5, 4, 0
    };

    unsigned int vao, vbo, ibo;
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    GLCall(glGenBuffers(1, &vbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));

    GLCall(glGenBuffers(1, &ibo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

    ShaderProgramSource srcCube = ParseShader("res/shaders/Cube.shader");
    unsigned int cubeShader = CreateShader(srcCube.VertexSource, srcCube.FragmentSource);
    GLCall(glUseProgram(cubeShader));

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 640.0f / 480.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(5, 3, 5),  // Camera position
        glm::vec3(0, 0, 0),     // Target position (where the camera looks)
        glm::vec3(0, 1, 0)      // Up vector (defines camera's upward direction)
    );
    // glm::mat4 model = glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = proj * view * model;
    glm::mat4 mvpAxes = proj * view;

    int mvpLocation = glGetUniformLocation(cubeShader, "u_MVP");

    // Add these vertices for the axes
    float axes[] = {
        // X axis (red)
        0.0f, 0.0f, 0.0f,
        3.0f, 0.0f, 0.0f,
        // Y axis (green)
        0.0f, 0.0f, 0.0f,
        0.0f, 3.0f, 0.0f,
        // Z axis (blue)
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 3.0f
    };

    unsigned int axesVao, axesVbo;
    GLCall(glGenVertexArrays(1, &axesVao));
    GLCall(glBindVertexArray(axesVao));

    GLCall(glGenBuffers(1, &axesVbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, axesVbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(axes), axes, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));

    ShaderProgramSource srcAxes = ParseShader("res/shaders/Axes.shader");
    unsigned int axesShader = CreateShader(srcAxes.VertexSource, srcAxes.FragmentSource);


    while (!glfwWindowShouldClose(window)) {
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        // model = glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
        mvp = proj * view * model;

        // Draw the cube
        GLCall(glUseProgram(cubeShader));
        GLCall(glBindVertexArray(vao));
        GLCall(glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp)));

        GLCall(glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr));

        // Draw the axes
        GLCall(glUseProgram(axesShader));
        GLCall(glBindVertexArray(axesVao));
        int axesMvpLocation = glGetUniformLocation(axesShader, "u_MVP");
        GLCall(glUniformMatrix4fv(axesMvpLocation, 1, GL_FALSE, glm::value_ptr(mvpAxes)));

        // Draw X axis in red
        int colorLocation = glGetUniformLocation(axesShader, "u_Color");
        GLCall(glUniform4f(colorLocation, 1.0f, 0.0f, 0.0f, 1.0f));
        GLCall(glDrawArrays(GL_LINES, 0, 2));

        // Draw Y axis in green
        GLCall(glUniform4f(colorLocation, 0.0f, 1.0f, 0.0f, 1.0f));
        GLCall(glDrawArrays(GL_LINES, 2, 2));

        // Draw Z axis in blue
        GLCall(glUniform4f(colorLocation, 0.0f, 0.0f, 1.0f, 1.0f));
        GLCall(glDrawArrays(GL_LINES, 4, 2));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(cubeShader);
    glDeleteProgram(axesShader);
    glfwTerminate();
    return 0;
}


