#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
/* #include "imgui_impl_opengl3_loader.h" */
#if defined(BOB)
#include <GL/glew.h>
#else
#include <GLES3/gl3.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// C++ Standard Library
#include <cmath>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    bool full_screen = false;
    GLFWwindow* window;
    if (full_screen)
    {
        // Create a full screen window by specifing the monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        window = glfwCreateWindow(
                720, 720,                          // width, height
                "Dear ImGui GLFW+OpenGL3 example",  // title
                monitor,                            // monitor
                NULL                                // share
                );
    }
    else
    {
        window = glfwCreateWindow(
                720, 720,                          // width, height
                "Dear ImGui GLFW+OpenGL3 example",  // title
                NULL,                               // monitor
                NULL                                // share
                );
    }
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

#if defined(BOB)
    glewInit();
#endif
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    /* ImGui::StyleColorsClassic(); */
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;

    // Setup batch line buffers
    GLfloat g_vertex_buffer_data[4] = {
        0.0f,  1.0f,
        0.0f,  0.0f
    };

    GLuint vertexbuffer_id;
    glGenBuffers(1, &vertexbuffer_id);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_id);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );

    float t = 0.0f;
    float w = 1.0f;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        t += w * ImGui::GetIO().DeltaTime;

        g_vertex_buffer_data[0] = std::cos(t);
        g_vertex_buffer_data[1] = std::sin(t);

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // Create a window called "Hello, world!" and append into it.
        ImGui::Begin("Hello, world!");

        ImGui::SliderFloat("frequency", &w, 1.f, 100.f);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw lines to screen
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_id);
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), g_vertex_buffer_data, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, 2); // 2 indices for the 2 end points of 1 line
        glDisableVertexAttribArray(0);

        // Draw imgui stuff to screen
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    // Cleanup VBO
    glDeleteBuffers(1, &vertexbuffer_id);

    return 0;
}
