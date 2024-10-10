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

double eyeX = 5.0, eyeY = 3.0, eyeZ = 5.0;
double objX = 0.0, objY = 0.0, objZ = 0.0;
double upX = 0.0, upY = 1.0, upZ = 0.0;

glm::mat4 view = glm::lookAt(
    glm::vec3(eyeX, eyeY, eyeZ),  // Camera position
    glm::vec3(objX, objY, objZ),  // Target position (where the camera looks)
    glm::vec3(upX, upY, upZ)   // Up vector (defines camera's upward direction)
);

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);  // Close the window when ESC is pressed
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        eyeY += 0.5;
        view = glm::lookAt(
            glm::vec3(eyeX, eyeY, eyeZ),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );
    }
    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS) {
        eyeY -= 0.5;
        view = glm::lookAt(
            glm::vec3(eyeX, eyeY, eyeZ),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );
    }
    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        eyeZ -= 0.5;
        view = glm::lookAt(
            glm::vec3(eyeX, eyeY, eyeZ),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        eyeZ += 0.5;
        view = glm::lookAt(
            glm::vec3(eyeX, eyeY, eyeZ),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );
    }
    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        eyeX -= 0.5;
        view = glm::lookAt(
            glm::vec3(eyeX, eyeY, eyeZ),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );
    }
    if (key == GLFW_KEY_D && action == GLFW_PRESS) {
        eyeX += 0.5;
        view = glm::lookAt(
            glm::vec3(eyeX, eyeY, eyeZ),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );
    }
}

int main() {
    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "3D Scene", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, keyCallback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK) {
        std::cout << "Error initializing GLEW!" << std::endl;
        return -1;
    }

    // Enable depth test for correct 3D rendering
    glEnable(GL_DEPTH_TEST);

    // Cube vertices and indices
    float cubePositions[] = {
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

    unsigned int cubeIndices[] = {
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

    unsigned int cubeVao, cubeVbo, cubeIbo;
    GLCall(glGenVertexArrays(1, &cubeVao));
    GLCall(glBindVertexArray(cubeVao));

    GLCall(glGenBuffers(1, &cubeVbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, cubeVbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(cubePositions), cubePositions, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));

    GLCall(glGenBuffers(1, &cubeIbo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIbo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW));

    // Pyramid vertices and indices
    float pyramidPositions[] = {
        // Base
        -0.5f, 0.0f, -0.5f,
         0.5f, 0.0f, -0.5f,
         0.5f, 0.0f,  0.5f,
        -0.5f, 0.0f,  0.5f,
        // Apex
         0.0f, 1.0f,  0.0f
    };

    unsigned int pyramidIndices[] = {
        // Base
        0, 1, 2, 2, 3, 0,
        // Sides
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4
    };

    unsigned int pyramidVao, pyramidVbo, pyramidIbo;
    GLCall(glGenVertexArrays(1, &pyramidVao));
    GLCall(glBindVertexArray(pyramidVao));

    GLCall(glGenBuffers(1, &pyramidVbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, pyramidVbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidPositions), pyramidPositions, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));

    GLCall(glGenBuffers(1, &pyramidIbo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pyramidIbo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pyramidIndices), pyramidIndices, GL_STATIC_DRAW));

    // Axes vertices
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

    // Load shaders
    ShaderProgramSource srcCube = ParseShader("res/shaders/Cube.shader");
    unsigned int cubeShader = CreateShader(srcCube.VertexSource, srcCube.FragmentSource);

    ShaderProgramSource srcAxes = ParseShader("res/shaders/Axes.shader");
    unsigned int axesShader = CreateShader(srcAxes.VertexSource, srcAxes.FragmentSource);

    ShaderProgramSource srcPyramid = ParseShader("res/shaders/Cube.shader");
    unsigned int pyramidShader = CreateShader(srcPyramid.VertexSource, srcPyramid.FragmentSource);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);

    glm::mat4 cubeModel = glm::mat4(1.0f);
    glm::mat4 cubeMvp = proj * view * cubeModel;

    glm::mat4 pyramidModel = glm::translate(glm::mat4(1.0f), glm::vec3(2.5f, 0.0f, 1.0f));
    // glm::mat4 pyramidModel = glm::mat4(1.0f);
    glm::mat4 pyramidModelTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(2.5f, 0.0f, 1.0f));
    glm::mat4 pyramidMvp = proj * view * pyramidModel;

    glm::mat4 axesMvp = proj * view;

    int mvpLocationCube = glGetUniformLocation(cubeShader, "u_MVP");
    int mvpLocationPyramid = glGetUniformLocation(pyramidShader, "u_MVP");

    while (!glfwWindowShouldClose(window)) {
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        // Draw the cube
        // cubeModel = glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
        cubeMvp = proj * view * cubeModel;
        GLCall(glUseProgram(cubeShader));
        GLCall(glBindVertexArray(cubeVao));
        GLCall(glUniformMatrix4fv(mvpLocationCube, 1, GL_FALSE, glm::value_ptr(cubeMvp)));
        GLCall(glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr));


        // Draw the pyramid
        // pyramidModel = glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f)) * pyramidModelTranslation;
        // pyramidModel = pyramidModelTranslation * glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
        pyramidMvp = proj * view * pyramidModel;
        GLCall(glUseProgram(pyramidShader));
        GLCall(glBindVertexArray(pyramidVao));
        GLCall(glUniformMatrix4fv(mvpLocationPyramid, 1, GL_FALSE, glm::value_ptr(pyramidMvp)));
        GLCall(glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, nullptr));

        // Draw the axes
        axesMvp = proj * view;
        GLCall(glUseProgram(axesShader));
        GLCall(glBindVertexArray(axesVao));
        int axesMvpLocation = glGetUniformLocation(axesShader, "u_MVP");
        GLCall(glUniformMatrix4fv(axesMvpLocation, 1, GL_FALSE, glm::value_ptr(axesMvp)));

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
    glDeleteProgram(pyramidShader);
    glDeleteProgram(axesShader);
    glfwTerminate();
    return 0;
}


