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

// Utility
#include "math.inl"

// C++ Standard Library
#include <cmath>
#include <cstring>
#include <cstdlib>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


// for each point
//    for each line
//       if intersects(point, line)
//          DEFLECT(point.velocity, line.normal)
//


static const float BOUNDARY_PADDING = 0.05f;
static const float BOUNDARY_LIMIT = 1.f - BOUNDARY_PADDING;


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
    int n_boundaries;
    int n_max;

    float boundary_thickness;
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
    env->boundary_thickness = 1e-3f;
    env->n_boundaries = 0;
    env->n_max = boundary_count;
}

void environment_add_boundary(Environment* const env, const Vec2 tail, const Vec2 head)
{
    // Don't add anything if we are already at/over the max allocated boundary count
    if (env->n_boundaries >= env->n_max)
    {
        return;
    }

    // Add boundary points
    if (tail.x < head.x)
    {
        (env->boundaries + env->n_boundaries)->tail = tail;
        (env->boundaries + env->n_boundaries)->head = head;
    }
    else
    {
        (env->boundaries + env->n_boundaries)->tail = head;
        (env->boundaries + env->n_boundaries)->head = tail;
    }

    // Compute normal for boundary
    *(env->normals + env->n_boundaries) = line_to_normal(env->boundaries + env->n_boundaries);

    // Count new boundary
    ++env->n_boundaries;
}

bool environment_is_boundary_between(const Environment* const env, const Vec2* const start, const Vec2* const end)
{
    // TODO(enhancment) only check intercept on nearby boundary lines
    for (int l = 0; l < env->n_boundaries; ++l)
    {
        if (vec2_segment_segment_intercept_check(
            &(env->boundaries + l)->tail,
            &(env->boundaries + l)->head,
            start,
            end
        ))
        {
            return true;
        }
    }
    return false;
}

void environment_destroy(Environment* const env)
{
    std::free(env->boundaries);
    std::free(env->normals);
}

struct Particles
{
    Vec2* positions_previous;
    Vec2* positions;
    Vec2* velocities_previous;
    Vec2* velocities;
    Vec2* forces;
    int n_active;
    int n_max;
    float max_velocity;
};

void particles_initialize(Particles* const ps, const int particle_count)
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
<<<<<<< HEAD
    ps->max_velocity = 1;
=======

    ps->max_velocity = 1.f;
>>>>>>> 1e59df2... Adds experimental "no-force" boundaries
}

void particles_spawn_at(Particles* const ps, const Vec2 position)
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

void particles_clear(Particles* const ps)
{
    ps->n_active = 0;
}

void particles_update(Particles* const ps, const Environment* const env, const float dt)
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

    // Update point states BEFORE collision resolution to figure out
    // where points will be next as if they hadn't collided
    integrate_states_fixed_step(ps->positions, ps->velocities, ps->forces, ps->n_active, dt);

    // Collide points and environment lines
    for (int i = 0; i < ps->n_active; ++i)
    {
        // TODO(enhancement) limit search to nearby boundaries
        for (int l = 0; l < env->n_boundaries; ++l)
        {
            // Particle shot through boundary
            Vec2 intercept_result;
            if (vec2_segment_segment_intercept(
                &intercept_result,
                (ps->positions + i),
                (ps->positions_previous + i),
                &(env->boundaries + l)->tail,
                &(env->boundaries + l)->head
            ))
            {
                // Set new location to intercept point
                vec2_set(ps->positions + i, &intercept_result);

                // Offset to a bit above the intercept point if comming from above
                if (vec2_above_line_with_normal(env->boundaries + l, env->normals + l, ps->positions_previous + i))
                {
                    vec2_scale_compound_add(ps->positions + i, env->normals + l, env->boundary_thickness);
                }
                // Offset to a bit below the intercept point if comming from below
                else
                {
                    vec2_scale_compound_add(ps->positions + i, env->normals + l, -env->boundary_thickness);
                }

                // Reflect and dampen velocity vector
                vec2_reflect(ps->velocities + i, ps->velocities + i, env->normals + l);
                vec2_scale(ps->velocities + i, env->dampening);
                break;
            }
            // Particle right above boundary
            else if (vec2_near_segment_with_normal(env->boundaries + l, env->normals + l, ps->positions + i, env->boundary_thickness))
            {
                // Set new location as last location
                vec2_set(ps->positions + i, ps->positions_previous + i);

                // Reflect and dampen velocity vector
                vec2_reflect(ps->velocities + i, ps->velocities + i, env->normals + l);
                vec2_scale(ps->velocities + i, env->dampening);
                break;
            }
        }
    }

<<<<<<< HEAD
    // Apply hard limits on velocities
    for (int i = 0; i < ps->n_active; ++i)
    {
        vec2_clamp(ps->velocities + i, -(ps->max_velocity), ps->max_velocity);
=======
    // Apply hard screen limits on velocity
    for (int i = 0; i < ps->n_active; ++i)
    {
        vec2_clamp(ps->velocities + i, -ps->max_velocity, ps->max_velocity);
>>>>>>> 1e59df2... Adds experimental "no-force" boundaries
    }

    // Apply hard screen limits on position
    for (int i = 0; i < ps->n_active; ++i)
    {
        vec2_clamp(ps->positions + i, -BOUNDARY_LIMIT, +BOUNDARY_LIMIT);
    }

    vec2_set_n(ps->forces, &env->gravity, ps->n_active);
}

void particles_destroy(Particles* const ps)
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

void planets_apply_to_particles(const Planets* const planets, const Environment* const env, Particles* const ps)
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

void planets_clear(Planets* const planets)
{
    planets->n_active = 0;
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
        uint64_t left_shift : 1;
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
    state->current.fields.left_shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    state->pressed.mask = (state->current.mask ^ state->previous.mask) & state->current.mask;
    state->released.mask = (state->current.mask ^ state->previous.mask) & state->previous.mask;
    std::memcpy(&state->previous, &state->current, sizeof(Buttons));  
}

struct RenderPipelineData
{
    GLuint particles_vba_id;
    GLuint planets_vba_id;
    GLuint environment_vba_id;
};

void render_pipeline_initialize(RenderPipelineData* const r_data)
{
    glPointSize(3.f);

    // Setup vertex buffer for points
    glGenBuffers(1, &r_data->particles_vba_id);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->particles_vba_id);

    // Setup vertex buffer for planets
    glGenBuffers(1, &r_data->planets_vba_id);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->planets_vba_id);

    // Setup vertex buffer for lines
    glGenBuffers(1, &r_data->environment_vba_id);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->environment_vba_id);
}

void render_pipeline_destroy(RenderPipelineData* const r_data)
{
    glDeleteBuffers(1, &r_data->particles_vba_id);
    glDeleteBuffers(1, &r_data->planets_vba_id);
    glDeleteBuffers(1, &r_data->environment_vba_id);
}

void render_pipeline_draw_points(const GLuint vbid, const Vec2* const points, const int n_points)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbid);
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

void render_pipeline_draw_lines(const GLuint vbid, const Line* const lines, const int n_lines)
{
    // TODO(optimization) environment data need only be uploaded once, so glBufferData is redundant on
    //                    each update. Make a separate upload function and call on loop entry, or keep things
    //                    this way if the environment is to be dynamic
    glBindBuffer(GL_ARRAY_BUFFER, vbid);
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

void render_pipeline_draw_planets(RenderPipelineData* const r_data, const Planets* const planets)
{
    render_pipeline_draw_points(r_data->planets_vba_id, planets->positions, planets->n_active);
}

void render_pipeline_draw_particles(RenderPipelineData* const r_data, const Particles* const particles)
{
    render_pipeline_draw_points(r_data->particles_vba_id, particles->positions, particles->n_active);
}

void render_pipeline_draw_environment(RenderPipelineData* const r_data, const Environment* const env)
{
    render_pipeline_draw_lines(r_data->environment_vba_id, env->boundaries, env->n_boundaries);
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
        "snad",  // title
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

    static const int N_PLANETS_MAX = 1000;
    static const int N_POINTS_MAX = 200000;

    // Initialize particles
    Particles particles;
    particles_initialize(&particles, N_POINTS_MAX);

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

        glfwPollEvents();

        // Update game user input stuff first
        user_input_state_update(&input_state, window);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Spawn single particle on click
        if (input_state.current.fields.left_ctrl && input_state.pressed.fields.left_mouse_button)
        {
            Vec2 p;
            get_cursor_position_normalized(&p, window, display_w, display_h);
            particles_spawn_at(&particles, p);
        }
        // Spew particles from mouse
        else if (input_state.current.fields.left_shift && input_state.current.fields.left_mouse_button)
        {
            Vec2 p;
            get_cursor_position_normalized(&p, window, display_w, display_h);
            particles_spawn_at(&particles, p);
        }
        // Spawn single planet on click
        else if (input_state.pressed.fields.left_mouse_button)
        {
            Vec2 p;
            get_cursor_position_normalized(&p, window, display_w, display_h);
            planets_spawn_at(&planets, p, 0.1f /*mass*/);
        }

        // Apply planet gravity to particles
        planets_apply_to_particles(&planets, &env, &particles);

        // Do particle update
        particles_update(&particles, &env, dt);

        // Create a window called "Hello, world!" and append into it.
        ImGui::Begin("Debug stuff");
        ImGui::Text("Particles  : (%d)", particles.n_active);
        ImGui::Text("Boundaries : (%d)", env.n_boundaries);
        ImGui::InputFloat2("gravity", (float*)(&env.gravity));
        ImGui::SliderFloat("dampening", &env.dampening, 0.1f, 1.f);
        ImGui::SliderFloat("max_velocity", &particles.max_velocity, 0.5f, 5.f);
        if (ImGui::SliderFloat("point size", &point_size, 1.f, 20.f))
        {
            glPointSize(point_size);
        }
        if (ImGui::SmallButton("Clear particles"))
        {
            particles_clear(&particles);
        }
        if (ImGui::SmallButton("Clear planets"))
        {
            planets_clear(&planets);
        }
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        // TODO:
        // Render the game to a texture (IMGUI displays this image in a window)

        // Rendering
        ImGui::Render();
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw lines to screen
        render_pipeline_draw_environment(&render_pipeline_data, &env);
        render_pipeline_draw_particles(&render_pipeline_data, &particles);
        render_pipeline_draw_planets(&render_pipeline_data, &planets);

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
    particles_destroy(&particles);
    environment_destroy(&env);

    return 0;
}
