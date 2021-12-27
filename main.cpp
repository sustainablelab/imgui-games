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

inline void vec2_scale_compound_add(Vec2* const lhs, const Vec2* const rhs, const float scale)
{
    lhs->x += rhs->x * scale;
    lhs->y += rhs->y * scale;
}

inline void vec2_compound_add(Vec2* const lhs, const Vec2* const rhs)
{
    lhs->x += rhs->x;
    lhs->y += rhs->y;
}

inline void vec2_negate(Vec2* const dst, const Vec2* const src)
{
    dst->x = -src->x;
    dst->y = -src->y;  
}

inline void vec2_set_zero(Vec2* const dst)
{
    std::memset(dst, 0, sizeof(Vec2));
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
        vec2_scale_compound_add(velocities + i, acceleratons + i, dt);

        // x += v * dt
        vec2_scale_compound_add(positions + i, velocities + i, dt);
    }
}

inline float vec2_cross_product(const Vec2* const lhs, const Vec2* const rhs)
{
    return (lhs->x * rhs->y) - (lhs->y * rhs->x);
}

inline bool vec2_line_segment_line_segment_intercept(
    Vec2* intercept,
    const Vec2* const p,
    const Vec2* const p_head,
    const Vec2* const q,
    const Vec2* const q_head,
    const float tolerance)
{
    const Vec2 r{p_head->x - p->x, p_head->y - p->y};
    const Vec2 s{q_head->x - q->x, q_head->y - q->y};
    const float r_cross_s = vec2_cross_product(&r, &s);

    // Parallel case
    if (std::abs(r_cross_s) < tolerance)
    {
        return false;
    }

    const Vec2 q_m_p{q->x - p->x, q->y - p->y};
    const float q_m_p_cross_r = vec2_cross_product(&q_m_p, &r);
    const float u = q_m_p_cross_r / r_cross_s;
    const float t = vec2_cross_product(&q_m_p, &s) / r_cross_s;

    // Intersection on segment
    if (0 <= u && u <= 1 && 0 <= t && t <= 1)
    {
        intercept->x = p->x + t * r.x;
        intercept->y = p->y + t * r.y;
        return true;
    }

    // Intersection off segment
    return false;
}

inline bool vec2_within_aabb(const Vec2* const top, const Vec2* const bot, const Vec2* const point, const float tolerance)
{
    const float min_x = std::fmin(top->x, bot->x);
    const float min_y = std::fmin(top->y, bot->y);
    const float max_x = std::fmax(top->x, bot->x);
    const float max_y = std::fmax(top->y, bot->y);
    return ((min_x - tolerance) < point->x && point->x < (max_x + tolerance)) &&
           ((min_y - tolerance) < point->y && point->y < (max_y + tolerance));
}

inline bool vec2_near_line(const Line* const line, const Vec2* const point, const float tolerance)
{
    const Vec2 lhs{line->head.x - line->tail.x, line->head.y - line->tail.y};
    const Vec2 rhs{point->x - line->tail.x, point->y - line->tail.y};
    return std::abs(vec2_cross_product(&lhs, &rhs)) < tolerance;
}

inline bool vec2_near_line_segment(const Line* const line, const Vec2* const point, const float tolerance)
{
    return vec2_near_line(line, point, tolerance) && vec2_within_aabb(&line->tail, &line->head, point, tolerance);
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
    env->dampening = 0.8f;
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
    Vec2* velocities_previous;
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

    ps->velocities_previous = (Vec2*)std::malloc(sizeof(Vec2) * particle_count);
    vec2_set_zero_n(ps->velocities_previous, particle_count);

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
    vec2_copy_n(ps->velocities_previous, ps->velocities, ps->n_active);

    // Negative force to apply on contact
    Vec2 negative_gravity;
    vec2_negate(&negative_gravity, &env->gravity);

    // Divide by particle mass
    for (int i = 0; i < ps->n_active; ++i)
    {
        (ps->forces + i)->x /= 1.f;
        (ps->forces + i)->y /= 1.f;
    }

    // Collide points and environment lines
    for (int i = 0; i < ps->n_active; ++i)
    {
        for (int l = 0; l < env->n_active; ++l)
        {
            if (vec2_near_line_segment(env->boundaries + l, ps->positions + i, dt))
            {
                vec2_reflect(ps->velocities + i, ps->velocities + i, env->normals + l);
                vec2_scale(ps->velocities + i, env->dampening);

                const float d = vec2_dot(env->normals + i, &env->gravity);
                Vec2 projected_gravity = negative_gravity;
                vec2_scale_compound_add(&projected_gravity, &env->gravity, d);
                vec2_set(ps->forces + i, &projected_gravity);
                break;
            }
        }
    }

    // Update point states
    integrate_states_fixed_step(ps->positions, ps->velocities, ps->forces, ps->n_active, dt);

    // Apply hard screen limits on position
    for (int i = 0; i < ps->n_active; ++i)
    {
        vec2_clamp(ps->positions + i, -BOUNDARY_LIMIT, +BOUNDARY_LIMIT);
    }
}

void particle_state_destroy(ParticleState* const ps)
{
    std::free(ps->positions_previous);
    std::free(ps->positions);
    std::free(ps->velocities_previous);
    std::free(ps->velocities);
    std::free(ps->forces);   
}

struct Planets
{
    Vec2* positions;
    float* masses;
    int n_active;
    int n_max;
};

void planets_initialize(Planets* const planets, const int planets_count)
{
    planets->positions = (Vec2*)std::malloc(sizeof(Vec2) * planets_count);
    vec2_set_zero_n(planets->positions, planets_count);

    planets->masses = (float*)std::malloc(sizeof(float) * planets_count);
    std::memset(planets->masses, 0, sizeof(float) * planets_count);

    planets->n_active = 0;
    planets->n_max = planets_count;
}

void planets_spawn_at(Planets* const planets, const Vec2 position, const float mass)
{
    if (planets->n_active >= planets->n_max)
    {
        return;
    }

    // Initialize point state
    vec2_set(planets->positions + planets->n_active, &position);

    // Initialize mass
    *(planets->masses + planets->n_active) = mass;

    // Increment number of active particles
    ++planets->n_active;
}

void planets_apply_to_particles(const Planets* const planets, ParticleState* const ps)
{
    // Calc pull of each planet on each particle; add results to forces
    for (int i = 0; i < ps->n_active; ++i)
    {
        for (int p = 0; p < planets->n_active; ++p)
        {
            Vec2 force;
            // Force is particle_position - planet_position
            /* force.x = (ps.positions + i)->x - planet.x; */
            /* force.y = (ps.positions + i)->y - planet.y; */
            // Force is planet_position - particle_position
            force.x = (planets->positions + p)->x - (ps->positions + i)->x;
            force.y = (planets->positions + p)->y - (ps->positions + i)->y;

            // Normalize the force
            const float r = vec2_length_squared(&force);
            if (r > 1e-4f)
            {
                const float m = *(planets->masses + p) / r;
                force.x *= m;
                force.y *= m;
                (ps->forces + i)->x += force.x;
                (ps->forces + i)->y += force.y;
            }
        }
    }
}

void planets_destroy(Planets* const planets)
{
    std::free(planets->positions);
    std::free(planets->masses);
}

union Buttons
{
    struct BitField
    {
        uint64_t right_mouse_button : 1;
        uint64_t left_mouse_button : 1;
        uint64_t left_ctrl : 1;
    };

    BitField fields;
    uint64_t mask;
};

struct UserInputState
{
    Buttons previous;
    Buttons current;
    Buttons pressed;
    Buttons released;
};

void user_input_state_initialized(UserInputState* const state, GLFWwindow* const window)
{
    std::memset(&state->previous, 0, sizeof(Buttons));
    std::memset(&state->current, 0, sizeof(Buttons));
    std::memset(&state->pressed, 0, sizeof(Buttons));
    std::memset(&state->released, 0, sizeof(Buttons));
}

void user_input_state_update(UserInputState* const state, GLFWwindow* const window)
{
    state->current.fields.right_mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    state->current.fields.left_mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    state->current.fields.left_ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
    state->pressed.mask = (state->current.mask ^ state->previous.mask) & state->current.mask;
    state->released.mask = (state->current.mask ^ state->previous.mask) & state->previous.mask;
    std::memcpy(&state->previous, &state->current, sizeof(Buttons));  
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

    int small = 600;
    int display_w = small, display_h = small;

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

    static const int N_PLANETS_MAX = 10;
    static const int N_POINTS_MAX = 200000;

    // Initialize particles
    ParticleState ps;
    particle_state_initialize(&ps, N_POINTS_MAX);

    // Initial planets
    Planets planets;
    planets_initialize(&planets, N_PLANETS_MAX);

    // Initialize render data
    RenderPipelineData render_pipeline_data;
    render_pipeline_initialize(&render_pipeline_data);

    // Update current used input states
    UserInputState input_state;
    user_input_state_initialized(&input_state, window);

    float point_size = 3.f;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        const float dt = ImGui::GetIO().DeltaTime;

        particle_state_reset(&ps, &env);

        glfwPollEvents();

        // Update game user input stuff first
        user_input_state_update(&input_state, window);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Spawn particle on click of mouse
        if (input_state.current.fields.left_ctrl && input_state.current.fields.left_mouse_button)
        {
            // Spawn a particle where the mouse clicks
            Vec2 p;
            get_cursor_position_normalized(&p, window, display_w, display_h);
            particle_state_spawn_at(&ps, p);
        }
        else if (input_state.pressed.fields.left_mouse_button)
        {
            // Spawn a planet where the mouse clicks
            Vec2 p;
            get_cursor_position_normalized(&p, window, display_w, display_h);
            planets_spawn_at(&planets, p, 1.f /*mass*/);
        }

        // Apply planet gravity to particles
        planets_apply_to_particles(&planets, &ps);

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
    planets_destroy(&planets);
    particle_state_destroy(&ps);
    environment_destroy(&env);

    return 0;
}
