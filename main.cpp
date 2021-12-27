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
#include <cstring>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


struct Vec2
{
    float x;
    float y;
};


struct Line
{
    Vec2 tail;
    Vec2 head;
};



// for each point
//    for each line
//       if intersects(point, line)
//          DEFLECT(point.velocity, line.normal)
//



void vec2_scale_add(Vec2* lhs, Vec2* rhs, float scale)
{
    lhs->x += rhs->x * scale;
    lhs->y += rhs->y * scale;
}

void vec2_set_zero(Vec2* data, int n)
{
    std::memset(data, 0, sizeof(Vec2) * n);
}

void integrate(Vec2* positions, Vec2* velocities, Vec2* forces, int n, float dt)
{
    #define INV_MASS 1.f
    for (int i = 0; i < n; ++i)
    {
        // f = m * a
        // a = f / m = f * inv_mass
        // v += a * dt
        vec2_scale_add(velocities + i, forces + i, INV_MASS * dt);
        // x += v * dt
        vec2_scale_add(positions + i, velocities + i, dt);
    }
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

    static const int N_LINES = 1;
    static const Line environment[N_LINES] = {
        Line{
            Vec2{-1.f, -1.f},
            Vec2{+1.f, -1.f}
        }
    };

    // Setup batch line buffers
    static const int N_POINTS = 2;

    // Number of points to draw this frame
    const int N_POINTS_DRAWN = 2;

    Vec2 positions[N_POINTS] = {
        Vec2{0.0f,  0.0f},
        Vec2{0.0f,  0.2f}
    };

    Vec2 velocities[N_POINTS] = {
        Vec2{0.0f, 0.0f},
        Vec2{0.0f, 0.0f}
    };

    Vec2 forces[N_POINTS] = {
        Vec2{0.0f, 0.0f},
        Vec2{0.0f, 0.0f}
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

    velocities[1].y = 0.2f;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        vec2_set_zero(forces, N_POINTS_DRAWN);

        const float dt = ImGui::GetIO().DeltaTime;

        forces[1].x = 0; 
        forces[1].y = -0.1;

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        integrate(positions, velocities, forces, N_POINTS_DRAWN, dt);

        // Create a window called "Hello, world!" and append into it.
        ImGui::Begin("Hello, world!");

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
        glBufferData(GL_ARRAY_BUFFER, N_POINTS_DRAWN * sizeof(Vec2), positions, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, N_POINTS_DRAWN);
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
