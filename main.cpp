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
#include <cstdlib>

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


static const float BOUNDARY_PADDING = 0.05f;
static const float BOUNDARY_LIMIT = 1.f - BOUNDARY_PADDING;

inline int imin(const int lhs, const int rhs)
{
    return ((lhs <= rhs) * lhs) + ((rhs < lhs) * rhs);
}

inline float clampf(const float v, const float vmin, const float vmax)
{
    return std::fmax(vmin, std::fmin(v, vmax));
}

inline void vec2_scale_add(Vec2* const lhs, Vec2* const rhs, const float scale)
{
    lhs->x += rhs->x * scale;
    lhs->y += rhs->y * scale;
}

inline void vec2_set_zero_n(Vec2* const dst, const int n)
{
    std::memset(dst, 0, sizeof(Vec2) * n);
}

inline void vec2_set(Vec2* const dst, const Vec2* const set_value)
{
    std::memcpy(dst, set_value, sizeof(Vec2));
}

inline void vec2_set_n(Vec2* const dst, const Vec2* const set_value, const int n)
{
    for (int i = 0; i < n; ++i)
    {
        vec2_set(dst + i, set_value);
    }
}

inline void vec2_copy_n(Vec2* const dst, const Vec2* const src, const int n)
{
    std::memcpy(dst, src, sizeof(Vec2) * n);
}

inline void vec2_set_random_uniform_unit(Vec2* const dst)
{
    #define MAX_RAND_I 10000
    #define MAX_RAND_F 10000.f
    #define N_RAND 2.f * ((std::rand() % MAX_RAND_I) / MAX_RAND_F) - 1.f

    dst->x = N_RAND;
    dst->y = N_RAND;

    #undef MAX_RAND_I
    #undef MAX_RAND_F
    #undef N_RAND
}

inline void vec2_set_random_uniform_scaled(Vec2* const dst, const float scale)
{
    vec2_set_random_uniform_unit(dst);
    dst->x *= scale;
    dst->y *= scale;
}

inline float vec2_dot(const Vec2* const lhs, const Vec2* const rhs)
{
    return lhs->x * rhs->x + lhs->y * rhs->y;
}

inline float vec2_length_squared(Vec2* const src)
{
    return vec2_dot(src, src);
}

inline float vec2_dist_squared(Vec2* const lhs, Vec2* const rhs)
{
    const float dx = lhs->x - rhs->x;
    const float dy = lhs->y - rhs->y;
    return dx * dx + dy * dy;
}

inline float vec2_near(Vec2* const lhs, Vec2* const rhs, const float tol)
{
    const float dx = lhs->x - rhs->x;
    const float dy = lhs->y - rhs->y;
    return std::abs(dx) < tol and std::abs(dy) < tol;
}

inline void vec2_clamp(Vec2* const dst, const float vmin, const float vmax)
{
    dst->x = clampf(dst->x, vmin, vmax);
    dst->y = clampf(dst->y, vmin, vmax);
}

inline void vec2_scale(Vec2* const src, const float scale)
{
    src->x *= scale;
    src->y *= scale;
}

inline void vec2_reflect(Vec2* const r, const Vec2* const d, const Vec2* const n)
{
    // r = d - 2 * dot(d, n) * n
    const float dot_dn = vec2_dot(d, n);

    r->x = d->x - 2.f * dot_dn * n->x;
    r->y = d->y - 2.f * dot_dn * n->y;
}

inline Vec2 line_normal(const Line* const line)
{
    Vec2 normal;
    normal.x = -(line->head.y - line->tail.y);
    normal.y = line->head.x - line->tail.x;
    const float d = std::sqrt(vec2_length_squared(&normal));
    normal.x /= d;
    normal.y /= d;
    return normal;
}

void integrate_states_fixed_step(Vec2* positions, Vec2* velocities, Vec2* acceleratons, int n, float dt)
{
    for (int i = 0; i < n; ++i)
    {
        // f = m * a
        // a = f / m = f * inv_mass
        // v += a * dt
        vec2_scale_add(velocities + i, acceleratons + i, dt);

        // x += v * dt
        vec2_scale_add(positions + i, velocities + i, dt);
    }
}

// void collide_points(Vec2* const positions, Vec2* const velocities, Vec2* const forces, const int src_id, const int dst_id)
// {
//     if (vec2_near(*(positions + src_id), *(positions + dst_id), 1e-1))
//     {

//     }
// }

inline bool cross_product(const Vec2* const a, const Vec2* const b, const Vec2* const c)
{
    return (a->x - c->x) * (b->y - c->y) - (a->y - c->y) * (b->x - c->x);
}

inline bool point_within_aabb(const Vec2* const top, const Vec2* const bot, const Vec2* const point)
{
    return ((top->x <= point->x && point->x <= bot->x) ||
            (bot->x <= point->x && point->x <= top->x)) &&
           ((top->y <= point->y && point->y <= bot->y) ||
            (bot->y <= point->y && point->y <= top->y));
}

inline bool point_on_line(const Line* const line, const Vec2* const point)
{
    return std::abs(cross_product(&line->tail, &line->head, point)) < 1e-3f;
}

inline bool point_on_line_segment(const Line* const line, const Vec2* const point)
{
    return point_on_line(line, point) && point_within_aabb(&line->tail, &line->head, point);
}

struct RenderPipelineData
{
    GLuint points_vba_id;
    GLuint lines_vba_id;
};

void render_pipeline_initialize(RenderPipelineData* const r_data)
{
    glPointSize(3.f);

    // Setup vertex buffer for points
    glGenBuffers(1, &r_data->points_vba_id);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->points_vba_id);

    // Setup vertex buffer for lines
    glGenBuffers(1, &r_data->lines_vba_id);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->lines_vba_id);
}

void render_pipeline_destroy(RenderPipelineData* const r_data)
{
    glDeleteBuffers(1, &r_data->points_vba_id);
    glDeleteBuffers(1, &r_data->lines_vba_id);
}

void render_pipeline_draw_points(RenderPipelineData* const r_data, const Vec2* const points, const int n_points)
{
    glBindBuffer(GL_ARRAY_BUFFER, r_data->points_vba_id);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );
    glBufferData(GL_ARRAY_BUFFER, n_points * sizeof(Vec2), points, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_POINTS, 0, n_points);
}

void render_pipeline_draw_lines(RenderPipelineData* const r_data, const Line* const lines, const int n_lines)
{
    // TODO(optimization) environment data need only be uploaded once, so glBufferData is redundant on
    //                    each update. Make a separate upload function and call on loop entry, or keep things
    //                    this way if the environment is to be dynamic
    glBindBuffer(GL_ARRAY_BUFFER, r_data->lines_vba_id);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );
    glBufferData(GL_ARRAY_BUFFER, n_lines * sizeof(Line), lines, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, 2 * n_lines);
}


void get_cursor_position_normalized(Vec2* const cursor_position, GLFWwindow* const window, const int display_w, const int display_h)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    cursor_position->x = 2.f * ((float)xpos / (float)display_w) - 1.f;
    cursor_position->y = 1.f - 2.f * ((float)ypos / (float)display_h);
}

static const int N_ENVIRONMENT_LINES_MAX = 10;

struct Environment
{
    Line* boundaries;
    Vec2* normals;
    int n_active;
    int n_max;

    float dampening;
    Vec2 gravity;
};

void environment_initialize(Environment* const env, const int boundary_count)
{
    env->boundaries = (Line*)std::malloc(sizeof(Line) * boundary_count);
    env->normals = (Vec2*)std::malloc(sizeof(Vec2) * boundary_count);
    env->dampening = 0.95f;
    env->gravity.x = 0.f;
    env->gravity.y = -1.f;
    env->n_active = 0;
    env->n_max = boundary_count;
}

void environment_add_boundary(Environment* const env, const Vec2 tail, const Vec2 head)
{
    // Don't add anything if we are already at/over the max allocated boundary count
    if (env->n_active >= env->n_max)
    {
        return;
    }

    // Add boundary points
    (env->boundaries + env->n_active)->tail = tail;
    (env->boundaries + env->n_active)->head = head;

    // Compute normal for boundary
    *(env->normals + env->n_active) = line_normal(env->boundaries + env->n_active);

    // Count new boundary
    ++env->n_active;
}

void environment_destroy(Environment* const env)
{
    std::free(env->boundaries);
    std::free(env->normals);
}

struct ParticleState
{
    Vec2* positions_previous;
    Vec2* positions;
    Vec2* velocities;
    Vec2* forces;
    int n_active;
    int n_max;
};

void particle_state_initialize(ParticleState* const ps, const int particle_count)
{
    ps->positions_previous = (Vec2*)std::malloc(sizeof(Vec2) * particle_count);
    vec2_set_zero_n(ps->positions_previous, particle_count);

    ps->positions = (Vec2*)std::malloc(sizeof(Vec2) * particle_count);
    vec2_set_zero_n(ps->positions, particle_count);

    ps->velocities = (Vec2*)std::malloc(sizeof(Vec2) * particle_count);
    vec2_set_zero_n(ps->velocities, particle_count);

    ps->forces = (Vec2*)std::malloc(sizeof(Vec2) * particle_count);
    vec2_set_zero_n(ps->forces, particle_count);

    ps->n_active = 0;
    ps->n_max = particle_count;
}

void particle_state_spawn_random(ParticleState* const ps, int n_spawn, const float y_min, const float y_max)
{
    // Clamp number of points to spawn to max allocated
    n_spawn = imin(ps->n_max - ps->n_active, n_spawn);

    // Initialize point state
    while (n_spawn-- > 0)
    {
        // Get random starting positions and velocities
        Vec2 p_init;
        vec2_set_random_uniform_scaled(&p_init, BOUNDARY_LIMIT);
        p_init.y = std::fmin(y_max, std::fmax(y_min, p_init.y));
        vec2_set(ps->positions + ps->n_active, &p_init);

        // Increment number of active particles
        ++ps->n_active;
    }
}

void particle_state_spawn_at(ParticleState* const ps, const Vec2 position)
{
    if (ps->n_active >= ps->n_max)
    {
        return;
    }

    // Initialize point state
    vec2_set(ps->positions + ps->n_active, &position);

    // Increment number of active particles
    ++ps->n_active;
}

void particle_state_reset(ParticleState* const ps, const Environment* const env)
{
    vec2_set_n(ps->forces, &env->gravity, ps->n_active);
}

void particle_state_update(ParticleState* const ps, const Environment* const env, const float dt)
{
    // Cache previous positions
    vec2_copy_n(ps->positions_previous, ps->positions, ps->n_active);

    // Divide by particle mass
    for (int i = 0; i < ps->n_active; ++i)
    {
        (ps->forces + i)->x /= 1.f;
        (ps->forces + i)->y /= 1.f;
    }

    // Update point states
    integrate_states_fixed_step(ps->positions, ps->velocities, ps->forces, ps->n_active, dt);

    // Apply hard screen limits on position
    for (int i = 0; i < ps->n_active; ++i)
    {
        vec2_clamp(ps->positions + i, -BOUNDARY_LIMIT, +BOUNDARY_LIMIT);
    }

    // Collide points and environment lines
    for (int l = 0; l < env->n_active; ++l)
    {
        for (int i = 0; i < ps->n_active; ++i)
        {
            if (point_on_line_segment(env->boundaries + l, ps->positions + i))
            {
                vec2_reflect(ps->velocities + i, ps->velocities + i, env->normals + l);
                vec2_scale(ps->velocities + i, env->dampening);
            }
        }
    }
}

void particle_state_destroy(ParticleState* const ps)
{
    std::free(ps->positions_previous);
    std::free(ps->positions);
    std::free(ps->velocities);
    std::free(ps->forces);   
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

    int display_w = 720, display_h = 720;

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(
        display_w,
        display_h,                          // width, height
        "Dear ImGui GLFW+OpenGL3 example",  // title
        NULL,                               // monitor
        NULL                                // share
        );

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

    Environment env;
    environment_initialize(&env, N_ENVIRONMENT_LINES_MAX);
    // bottom wall
    environment_add_boundary(
        &env,
        Vec2{-BOUNDARY_LIMIT, -BOUNDARY_LIMIT},
        Vec2{+BOUNDARY_LIMIT, -BOUNDARY_LIMIT}
    );
    // right wall
    environment_add_boundary(
        &env,
        Vec2{+BOUNDARY_LIMIT, -BOUNDARY_LIMIT},
        Vec2{+BOUNDARY_LIMIT, +BOUNDARY_LIMIT}
    );
    // top wall
    environment_add_boundary(
        &env,
        Vec2{-BOUNDARY_LIMIT, +BOUNDARY_LIMIT},
        Vec2{+BOUNDARY_LIMIT, +BOUNDARY_LIMIT}
    );
    // left wall
    environment_add_boundary(
        &env,
        Vec2{-BOUNDARY_LIMIT, -BOUNDARY_LIMIT},
        Vec2{-BOUNDARY_LIMIT, +BOUNDARY_LIMIT}
    );

    // add some hard-coded level stuff
    environment_add_boundary(
        &env,
        Vec2{-0.5f * BOUNDARY_LIMIT, +0.5 * BOUNDARY_LIMIT},
        Vec2{+1.0f * BOUNDARY_LIMIT, +0.5 * BOUNDARY_LIMIT}
    );
    environment_add_boundary(
        &env,
        Vec2{-1.0f * BOUNDARY_LIMIT, -0.5 * BOUNDARY_LIMIT},
        Vec2{+0.5f * BOUNDARY_LIMIT, -0.5 * BOUNDARY_LIMIT}
    );

    // Setup batch line buffers
    static const int N_POINTS_MAX = 200000;
    static const int N_POINTS_MAX_SPAWN = 1000;

    // Initialize particles
    ParticleState ps;
    particle_state_initialize(&ps, N_POINTS_MAX);

    // Add some initial particles
    particle_state_spawn_random(&ps, 100, -BOUNDARY_LIMIT, -BOUNDARY_LIMIT + 0.1f);

    // Initialize render data
    RenderPipelineData render_pipeline_data;
    render_pipeline_initialize(&render_pipeline_data);

    float point_size = 3.f;
    int n_to_spawn = 1;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        particle_state_reset(&ps, &env);

        const float dt = ImGui::GetIO().DeltaTime;

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Spawn particle on click of mouse
        {
            const int left_button_pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            const int right_button_pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
            if (left_button_pressed == GLFW_PRESS || right_button_pressed == GLFW_PRESS)
            {
                Vec2 p;
                get_cursor_position_normalized(&p, window, display_w, display_h);
                particle_state_spawn_at(&ps, p);
            }
        }

        // Do particle update
        particle_state_update(&ps, &env, dt);

        // Create a window called "Hello, world!" and append into it.
        ImGui::Begin("Debug stuff");
        ImGui::Text("Points : (%d)", ps.n_active);
        ImGui::Text("Boundaries : (%d)", env.n_active);
        ImGui::InputFloat2("gravity", (float*)(&env.gravity));
        ImGui::SliderFloat("dampening", &env.dampening, 0.1f, 1.f);
        if (ImGui::SliderFloat("point size", &point_size, 1.f, 20.f))
        {
            glPointSize(point_size);
        }
        ImGui::SliderInt("n spawns", &n_to_spawn, 1, N_POINTS_MAX_SPAWN);
        if (ImGui::SmallButton("Spawn"))
        {
            particle_state_spawn_random(&ps, n_to_spawn, -BOUNDARY_LIMIT, -BOUNDARY_LIMIT + 0.1f);
        }
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        // Rendering
        ImGui::Render();
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw lines to screen
        render_pipeline_draw_points(&render_pipeline_data, ps.positions, ps.n_active);
        render_pipeline_draw_lines(&render_pipeline_data, env.boundaries, env.n_active);

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

    // Cleanup game state
    render_pipeline_destroy(&render_pipeline_data);
    particle_state_destroy(&ps);
    environment_destroy(&env);

    return 0;
}
