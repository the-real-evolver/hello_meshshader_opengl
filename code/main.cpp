//------------------------------------------------------------------------------
//  main.cpp
//  (C) 2023 Christian Bleicher
//------------------------------------------------------------------------------
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <cstdio>
#include <assert.h>

//------------------------------------------------------------------------------
const char* const MeshShader =
    "#version 450\n"
    "#extension GL_NV_mesh_shader : require\n"
    "// set the number of threads per workgroup\n"
    "layout(local_size_x = 1) in;\n"
    "// maximum allocation size for each meshlet\n"
    "layout(max_vertices = 3, max_primitives = 1) out;\n"
    "layout(triangles) out;\n"
    "out PerVertexData\n"
    "{\n"
    "  vec4 color;\n"
    "} v_out[];\n"
    "const vec3 vertices[3] = {vec3(-1,-1,0), vec3(0,1,0), vec3(1,-1,0)};\n"
    "const vec3 colors[3] = {vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(0.0,0.0,1.0)};\n"
    "void main()\n"
    "{\n"
    "    gl_MeshVerticesNV[0].gl_Position = vec4(vertices[0], 1.0);\n"
    "    gl_MeshVerticesNV[1].gl_Position = vec4(vertices[1], 1.0);\n"
    "    gl_MeshVerticesNV[2].gl_Position = vec4(vertices[2], 1.0);\n"
    "    v_out[0].color = vec4(colors[0], 1.0);\n"
    "    v_out[1].color = vec4(colors[1], 1.0);\n"
    "    v_out[2].color = vec4(colors[2], 1.0);\n"
    "    gl_PrimitiveIndicesNV[0] = 0;\n"
    "    gl_PrimitiveIndicesNV[1] = 1;\n"
    "    gl_PrimitiveIndicesNV[2] = 2;\n"
    "    gl_PrimitiveCountNV = 1;\n"
    "}\n";

const char* const FragmentShader =
    "#version 450\n"
    "layout(location = 0) out vec4 FragColor;\n"
    "in PerVertexData\n"
    "{\n"
    "  vec4 color;\n"
    "} fragIn;\n"
    "void main()\n"
    "{\n"
    "    FragColor = fragIn.color;\n"
    "}\n";

//------------------------------------------------------------------------------
/**
*/
void
framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

//------------------------------------------------------------------------------
/**
*/
void
process_input(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
}

//------------------------------------------------------------------------------
/**
*/
GLuint
create_shader_from_string(GLenum shader_type, const char* source)
{
    GLuint shader = glCreateShader(shader_type);
    if (shader != 0U)
    {
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (0 == compiled)
        {
            GLint info_len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
            if (info_len > 0)
            {
                char* buf = (char*)malloc(info_len);
                if (buf != NULL)
                {
                    glGetShaderInfoLog(shader, info_len, NULL, buf);
                    printf("Could not compile shader %d: %s\n", shader_type, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0U;
            }
        }
    }

    return shader;
}

//------------------------------------------------------------------------------
/**
*/
GLuint
create_program(GLuint mesh_shader, GLuint fragment_shader)
{
    if (0U == mesh_shader || 0U == fragment_shader)
    {
        return 0U;
    }

    GLuint program = glCreateProgram();
    if (program != 0U)
    {
        glAttachShader(program, mesh_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);
        GLint link_status = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &link_status);
        if (link_status != GL_TRUE)
        {
            GLint buf_length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &buf_length);
            if (buf_length > 0)
            {
                char* buf = (char*)malloc(buf_length);
                if (buf != NULL)
                {
                    glGetProgramInfoLog(program, buf_length, NULL, buf);
                    printf("Could not link program: %s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0U;
        }
    }

    return program;
}

//------------------------------------------------------------------------------
/**
*/
int
main()
{
    // init glfw and create window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Hello Meshshader", NULL, NULL);
    if (window == NULL)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }
    glViewport(0, 0, 800, 600);

    // create meshshader
    GLuint mesh_shader = create_shader_from_string(GL_MESH_SHADER_NV, MeshShader);
    assert(0U != mesh_shader, "create_shader_from_string: Could not create mesh shader.\n");
    GLuint fragment_shader = create_shader_from_string(GL_FRAGMENT_SHADER, FragmentShader);
    assert(0U != fragment_shader, "create_shader_from_string: Could not create fragment shader.\n");
    GLuint gpu_program = create_program(mesh_shader, fragment_shader);
    assert(0U != gpu_program, "create_program: Could not create program.\n");

    // renderloop
    while (!glfwWindowShouldClose(window))
    {
        process_input(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(gpu_program);
        glDrawMeshTasksNV(0U, 1U);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}