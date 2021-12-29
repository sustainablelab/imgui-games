#pragma once

// Standard Library
#include <stdio.h>

// OpenGL
#if defined(PLATFORM_WINDOWS)
#include <GL/glew.h>
#else
#include <GLES3/gl3.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers


unsigned create_shader_source(unsigned type, const char* source)
{
    unsigned shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int  success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("GL ERROR : shader compilation failed with:\n---\n\n%s\n---\n", infoLog);
        abort();
    }

    return shader;
}

unsigned link_shader_program(unsigned vert_shader, unsigned frag_shader, const unsigned* const geom_shader)
{
    unsigned program = glCreateProgram();
    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);
    if (geom_shader != nullptr)
    {
        glAttachShader(program, *geom_shader);
    }
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if(!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("GL ERROR : shader program linkage failed with:\n---\n\n%s\n---\n", infoLog);
        abort();
    }

    return program;
}
