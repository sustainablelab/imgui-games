// C++ Standard Library
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Utility
#include "math.inl"
#include "graphics.inl"

// OpenAL
#if defined(PLATFORM_SUPPORTS_AUDIO)
#include <AL/al.h>
#include <AL/alc.h>
#include <audio/wave.h>
#endif  // defined(PLATFORM_SUPPORTS_AUDIO)

// TODO
//
//  - Add animated sprites or shader-based graphical flair (light-orbs, sparkling) for particles and planets
//  - Add text rendering (e.g. for score, menus)
//  - Optimize collision / intercept checking
//  - Add some sort of ambient sound for planets
//  - Add some sort of spatial sound or wooshing (synethesized?) sound for particles
//  - Add sounds for boundary collisions
//  - Add level serialization
//  - Decide procedural level generation or not?
//


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


static const int N_ENVIRONMENT_LINES_MAX = 10;

struct EnvironmentBoundaryProperties
{
    float tail_hits;
    float head_hits;
};

struct Environment
{
    AABB goal;
    Line* boundaries;
    Vec2* normals;
    EnvironmentBoundaryProperties* boundary_properties;
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
    env->boundary_properties = (EnvironmentBoundaryProperties*)std::malloc(sizeof(EnvironmentBoundaryProperties) * boundary_count);
    env->dampening = 0.7f;
    env->gravity.x = 0.0f;
    env->gravity.y = -0.123f;
    env->boundary_thickness = 1e-3f;
    env->n_boundaries = 0;
    env->n_max = boundary_count;
}

void environment_update(Environment* const env, const float dt)
{
    // Decay hit accumulators over time
    for (int l = 0; l < env->n_boundaries; ++l)
    {
        (env->boundary_properties + l)->tail_hits = std::fmin(50.f, std::fmax(0.f, (env->boundary_properties + l)->tail_hits - 50.f * dt));
        (env->boundary_properties + l)->head_hits = std::fmin(50.f, std::fmax(0.f, (env->boundary_properties + l)->head_hits - 50.f * dt));
    }
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

    // Initialize properties
    std::memset(env->boundary_properties + env->n_boundaries, 0, sizeof(EnvironmentBoundaryProperties));

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
    ps->max_velocity = 2.5;
}

void particles_spawn_at(Particles* const ps, const Vec2 position)
{
    if (ps->n_active >= ps->n_max)
    {
        return;
    }

    // Initialize point state
    vec2_set(ps->positions + ps->n_active, &position);
    vec2_set(ps->positions_previous + ps->n_active, &position);
    vec2_set_zero(ps->velocities + ps->n_active);
    vec2_set_zero(ps->velocities_previous + ps->n_active);
    vec2_set_zero(ps->forces + ps->n_active);

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

    // // Divide by particle mass
    // for (int i = 0; i < ps->n_active; ++i)
    // {
    //     (ps->forces + i)->x *= 2.f;
    //     (ps->forces + i)->y *= 2.f;
    // }

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
                    vec2_scale_compound_add(ps->positions + i, env->normals + l, 3.f * env->boundary_thickness);
                }
                // Offset to a bit below the intercept point if comming from below
                else
                {
                    vec2_scale_compound_add(ps->positions + i, env->normals + l, -3.f * env->boundary_thickness);
                }

                // Reflect and dampen velocity vector
                vec2_reflect(ps->velocities + i, ps->velocities + i, env->normals + l);
                vec2_scale(ps->velocities + i, env->dampening);

                // TODO(enhancement) count particle intersection ("hits") nearest to endpoint; for now, counting hits for both
                // Count boundary hits when particle collide hard with boundaries
                const float approx_energy = 0.5f * vec2_length_manhattan(ps->velocities + i);
                (env->boundary_properties + l)->tail_hits += approx_energy;
                (env->boundary_properties + l)->head_hits += approx_energy;
                break;
            }
            // Particle right above boundary
            else if (vec2_near_segment_with_normal(env->boundaries + l, env->normals + l, ps->positions + i, env->boundary_thickness))
            {
                // Set new location as last location
                vec2_set(ps->positions + i, ps->positions_previous + i);

                // Offset to a bit above the intercept point if comming from above
                if (vec2_above_line_with_normal(env->boundaries + l, env->normals + l, ps->positions_previous + i))
                {
                    vec2_scale_compound_add(ps->positions + i, env->normals + l, 3.f * env->boundary_thickness);
                }
                // Offset to a bit below the intercept point if comming from below
                else
                {
                    vec2_scale_compound_add(ps->positions + i, env->normals + l, -3.f * env->boundary_thickness);
                }

                // Reflect and dampen velocity vector
                vec2_reflect(ps->velocities + i, ps->velocities + i, env->normals + l);
                vec2_scale(ps->velocities + i, env->dampening);

                // TODO(enhancement) count particle intersection ("hits") nearest to endpoint; for now, counting hits for both
                // Count boundary hits when particle collide hard with boundaries
                const float approx_energy = 0.5f * vec2_length_manhattan(ps->velocities + i);
                (env->boundary_properties + l)->tail_hits += approx_energy;
                (env->boundary_properties + l)->head_hits += approx_energy;
                break;
            }
        }
    }

    // Apply hard limits on velocities
    for (int i = 0; i < ps->n_active; ++i)
    {
        vec2_clamp(ps->velocities + i, -(ps->max_velocity), ps->max_velocity);
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

struct PlanetProperties
{
    float age;
    float mass;
};

struct Planets
{
    Vec2* positions;
    Vec2* directions;
    PlanetProperties* properties;

    int n_active;
    int n_max;
};

void planets_initialize(Planets* const planets, const int planets_count)
{
    planets->positions = (Vec2*)std::malloc(sizeof(Vec2) * planets_count);
    vec2_set_zero_n(planets->positions, planets_count);

    planets->directions = (Vec2*)std::malloc(sizeof(Vec2) * planets_count);
    vec2_set_zero_n(planets->directions, planets_count);

    planets->properties = (PlanetProperties*)std::malloc(sizeof(PlanetProperties) * planets_count);
    std::memset(planets->properties, 0, sizeof(PlanetProperties) * planets_count);

    planets->n_active = 0;
    planets->n_max = planets_count;
}

void planets_spawn_at(Planets* const planets, const Vec2 position, const Vec2 direction, const float mass)
{
    if (planets->n_active >= planets->n_max)
    {
        return;
    }

    // Initialize point state
    vec2_set(planets->positions + planets->n_active, &position);
    vec2_set(planets->directions + planets->n_active, &direction);

    // Initialize mass
    (planets->properties + planets->n_active)->mass = mass;

    // Initialize time alive
    (planets->properties + planets->n_active)->age = 0.f;

    // Increment number of active particles
    ++planets->n_active;
}

void planets_update(Planets* const planets, const float dt)
{
    for (int i = 0; i < planets->n_active; ++i)
    {
        planets->properties[i].age += dt;
    }
}

void planets_apply_to_particles(const Planets* const planets, const Environment* const env, Particles* const ps)
{
    // Calc pull of each planet on each particle; add results to forces
    for (int i = 0; i < ps->n_active; ++i)
    {
        for (int p = 0; p < planets->n_active; ++p)
        {
            Vec2 delta;

            // Force is planet_position - particle_position
            delta.x = (ps->positions + i)->x - (planets->positions + p)->x;
            delta.y = (ps->positions + i)->y - (planets->positions + p)->y;

            // The "direction" of a planet's field basically splits it into two-halves. On one side, its an attractor,
            // and the other a repeller. It seems like this sort of asymmetry is needed to make the game more playable
            // otherwise, you end up with particles cycling clusters of planets in chaos as opposed to getting "flung,"
            // unless you are extremely careful, which is not fun IMO.
            const bool is_symmetric = (vec2_length_squared(planets->directions + p) < 1e-4f);
            const float sign = (is_symmetric)*(-1.f) +
                               (!is_symmetric)*std::copysign(1.f, vec2_dot(&delta, planets->directions + p));

            // Squared distance between planet and particle
            const float r_sq = vec2_length_squared(&delta);

            // NOTE: this is no longer consistent with the Newtonian gravitational
            //       model, but make attractions more stable
            vec2_scale_compound_add(ps->forces + i, &delta, sign * ((planets->properties + p)->mass / (r_sq + 1e-5f)));
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
    std::free(planets->directions);
    std::free(planets->properties);
}

union Buttons
{
    struct BitField
    {
        uint64_t right_mouse_button : 1;
        uint64_t left_mouse_button : 1;
        uint64_t left_ctrl : 1;
        uint64_t left_shift : 1;
        uint64_t key_f : 1;
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
    state->current.fields.key_f = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    state->pressed.mask = (state->current.mask ^ state->previous.mask) & state->current.mask;
    state->released.mask = (state->current.mask ^ state->previous.mask) & state->previous.mask;
    std::memcpy(&state->previous, &state->current, sizeof(Buttons));  
}

struct RenderPipelineData
{
    GLuint particles_shader;
    GLuint particles_vao;
    GLuint particles_vbo;

    GLuint planets_shader;
    GLuint planets_vao;
    GLuint planets_vbo;

    GLuint environment_shader;
    GLuint environment_vao;
    GLuint environment_vbo;

    float aspect_ratio;
    float display_h;
    float display_w;

    GLFWwindow* window;
};

void render_pipeline_initialize(RenderPipelineData* const r_data,
                                GLFWwindow* const window,
                                const Planets* const planets,
                                const Particles* const particles,
                                const Environment* const environment)
{
    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create shader for particles
    {
        const GLuint vert_shader = create_shader_source(
            GL_VERTEX_SHADER,
            R"VertexShader(
                #version 330 core
                layout (location = 0) in vec2 aPos;
                layout (location = 1) in vec2 aVel;

                out vec4 VertColor;

                vec4 lerp(vec4 lhs, vec4 rhs, float a)
                {
                    return lhs * a + (1-a) * rhs;
                }

                void main()
                {
                    float mag = sqrt(aVel.x * aVel.x + aVel.y * aVel.y);
                    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
                    VertColor = lerp(vec4(mag, 0.3 * mag, 1.f-mag, 1), vec4(1, 1, 1, 0.3), 0.9);
                }
            )VertexShader"
        );
        const GLuint frag_shader = create_shader_source(
            GL_FRAGMENT_SHADER,
            R"FragmentShader(
                #version 330 core
                out vec4 FragColor;
                in vec4 GeomColor;
                void main()
                {
                    FragColor = GeomColor;
                }
            )FragmentShader"
        );
        const GLuint geom_shader = create_shader_source(
            GL_GEOMETRY_SHADER,
            R"FragmentShader(
                #version 330 core
                layout(points) in;
                layout(triangle_strip, max_vertices = 40) out;

                uniform float uAspectRatio;

                in vec4[] VertColor;
                out vec4 GeomColor;

                const float TWO_PI = 2.0 * 3.1415926;
                const float RADIUS = 0.01;

                vec4 apply_aspect_ratio(vec4 position, float ratio)
                {
                    return vec4(position[0] * ratio, position[1], position[2], position[3]);
                }

                void main()
                {
                    vec4 vColor = VertColor[0];

                    for (int i = 0; i <= 9; i++) {
                        float curr_ang = TWO_PI / 10.0 * (i+0);
                        vec4 curr_offset = vec4(cos(curr_ang) * RADIUS, -sin(curr_ang) * RADIUS, 0.0, 0.0);
                        gl_Position = apply_aspect_ratio(gl_in[0].gl_Position + curr_offset, uAspectRatio);
                        GeomColor = 0.5 * vColor;
                        EmitVertex();

                        gl_Position = apply_aspect_ratio(gl_in[0].gl_Position, uAspectRatio);
                        GeomColor = vColor;
                        EmitVertex();

                        float next_ang = TWO_PI / 10.0 * (i+1);
                        vec4 next_offset = vec4(cos(next_ang) * RADIUS, -sin(next_ang) * RADIUS, 0.0, 0.0);
                        gl_Position = apply_aspect_ratio(gl_in[0].gl_Position + next_offset, uAspectRatio);
                        GeomColor = 0.5 * vColor;
                        EmitVertex();
                    }

                    EndPrimitive();
                }
            )FragmentShader"
        );

        // Link shaders into program
        r_data->particles_shader = link_shader_program(vert_shader, frag_shader, &geom_shader);

        // Cleanup shader source
        glDeleteShader(vert_shader);
        glDeleteShader(frag_shader);
        glDeleteShader(geom_shader);
    }

    // Setup vertex buffer for points
    glGenVertexArrays(1, &r_data->particles_vao);
    glBindVertexArray(r_data->particles_vao);
    glGenBuffers(1, &r_data->particles_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->particles_vbo);
    glBufferData(GL_ARRAY_BUFFER, particles->n_max * sizeof(Vec2), 0, GL_DYNAMIC_DRAW);

    // Create shader for planets
    {
        const GLuint vert_shader = create_shader_source(
            GL_VERTEX_SHADER,
            R"VertexShader(
                #version 330 core
                layout (location = 0) in vec2 aPos;
                layout (location = 1) in vec2 aProps;

                out vData
                {
                   vec4 color;
                   float t;
                   float r;
                } VertProps;

                void main()
                {
                    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
                    VertProps.color = vec4(1.0, 0.5, 0.3, 0.8);
                    VertProps.t = aProps[0];
                    VertProps.r = aProps[1];
                }
            )VertexShader"
        );
        const GLuint frag_shader = create_shader_source(
            GL_FRAGMENT_SHADER,
            R"FragmentShader(
                #version 330 core
                out vec4 FragColor;
                in vec4 GeomColor;
                void main()
                {
                    FragColor = GeomColor;
                }
            )FragmentShader"
        );
        const GLuint geom_shader = create_shader_source(
            GL_GEOMETRY_SHADER,
            R"FragmentShader(
                #version 330 core
                layout(points) in;
                layout(triangle_strip, max_vertices = 40) out;

                uniform float uAspectRatio;

                in vData
                {
                   vec4 color;
                   float t;
                   float r;
                } VertProps[];

                out vec4 GeomColor;

                const float TWO_PI = 2.0 * 3.1415926;
                const float RADIUS_MIN = 0.15;
                const float RADIUS_DELTA = 0.025;

                vec4 apply_aspect_ratio(vec4 position, float ratio)
                {
                    return vec4(position[0] * ratio, position[1], position[2], position[3]);
                }

                void main()
                {
                    vec4 vColor = VertProps[0].color;
                    float t = VertProps[0].t;
                    float r = VertProps[0].r;
                    float radius = RADIUS_MIN + RADIUS_DELTA * sin(10 * r * t);

                    for (int i = 0; i <= 9; i++) {
                        float curr_ang = TWO_PI / 10.0 * (i+0);
                        vec4 curr_offset = vec4(cos(curr_ang) * radius, -sin(curr_ang) * radius, 0.0, 0.0);
                        gl_Position = apply_aspect_ratio(gl_in[0].gl_Position + curr_offset, uAspectRatio);
                        GeomColor = 0.2 * vColor;
                        EmitVertex();

                        gl_Position = apply_aspect_ratio(gl_in[0].gl_Position, uAspectRatio);
                        GeomColor = vColor;
                        EmitVertex();

                        float next_ang = TWO_PI / 10.0 * (i+1);
                        vec4 next_offset = vec4(cos(next_ang) * radius, -sin(next_ang) * radius, 0.0, 0.0);
                        gl_Position = apply_aspect_ratio(gl_in[0].gl_Position + next_offset, uAspectRatio);
                        GeomColor = 0.2 * vColor;
                        EmitVertex();
                    }

                    EndPrimitive();
                }
            )FragmentShader"
        );

        // Link shaders into program
        r_data->planets_shader = link_shader_program(vert_shader, frag_shader, &geom_shader);

        // Cleanup shader source
        glDeleteShader(vert_shader);
        glDeleteShader(frag_shader);
        glDeleteShader(geom_shader);
    }

    // Setup vertex buffer for planets
    glGenVertexArrays(1, &r_data->planets_vao);
    glBindVertexArray(r_data->planets_vao);
    glGenBuffers(1, &r_data->planets_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->planets_vbo);
    glBufferData(GL_ARRAY_BUFFER, planets->n_max * sizeof(Vec2), 0, GL_DYNAMIC_DRAW);

    // Create shader for environment
    {
        const GLuint vert_shader = create_shader_source(
            GL_VERTEX_SHADER,
            R"VertexShader(
                #version 330 core
                layout (location = 0) in vec2 aPos;
                layout (location = 1) in float aHitCount;

                out vec4 vColor;

                void main()
                {
                    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);

                    // Apply a glow to boundary lines when hit enough times
                    float heat = min(1, aHitCount / 25.f);
                    vColor = vec4(heat, 0.1 * heat + 0.1, 0.1 * (1-heat) + 0.1, 1);
                }
            )VertexShader"
        );
        const GLuint frag_shader = create_shader_source(
            GL_FRAGMENT_SHADER,
            R"FragmentShader(
                #version 330 core

                in vec4 gFragColor;
                out vec4 FragColor;
                void main()
                {
                    FragColor = gFragColor;
                }
            )FragmentShader"
        );

        const GLuint geom_shader = create_shader_source(
            GL_GEOMETRY_SHADER,
            R"GeometryShader(
                #version 330 core

                layout(lines) in;
                layout(triangle_strip, max_vertices = 8) out;

                uniform float uAspectRatio;

                in vec4 vColor[];
                out vec4 gFragColor;

                vec4 apply_aspect_ratio(vec4 position, float ratio)
                {
                    return vec4(position[0] * ratio, position[1], position[2], position[3]);
                }

                void main() {
                  vec2 g1 = vec2(gl_in[0].gl_Position);
                  vec2 g2 = vec2(gl_in[1].gl_Position);
                  vec2 v1 = normalize(g1 - g2) * 0.005;
                  vec2 v2 = normalize(g2 - g1) * 0.005;

                  gFragColor = vColor[0];
                  gl_Position = apply_aspect_ratio(vec4(-v2.y + g1.x, v2.x + g1.y, 0.0, 1.0), uAspectRatio);
                  EmitVertex();

                  gFragColor = vColor[0];
                  gl_Position = apply_aspect_ratio(vec4(v1.y + g2.x, -v1.x + g2.y, 0.0, 1.0), uAspectRatio);
                  EmitVertex();

                  gFragColor = vColor[0];
                  gl_Position = apply_aspect_ratio(vec4(v2.y + g1.x, -v2.x + g1.y, 0.0, 1.0), uAspectRatio);
                  EmitVertex();

                  gFragColor = vColor[0];
                  gl_Position = apply_aspect_ratio(vec4(-v1.y + g2.x, v1.x + g2.y, 0.0, 1.0), uAspectRatio);
                  EmitVertex();

                  EndPrimitive();
                }
            )GeometryShader"
        );

        // Link shaders into program
        r_data->environment_shader = link_shader_program(vert_shader, frag_shader, &geom_shader);

        // Cleanup shader source
        glDeleteShader(vert_shader);
        glDeleteShader(frag_shader);
        glDeleteShader(geom_shader);
    }

    // Setup vertex buffer for lines
    glGenVertexArrays(1, &r_data->environment_vao);
    glBindVertexArray(r_data->environment_vao);
    glGenBuffers(1, &r_data->environment_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->environment_vbo);
    glBufferData(GL_ARRAY_BUFFER, environment->n_max * (sizeof(Line) + sizeof(EnvironmentBoundaryProperties)), 0, GL_DYNAMIC_DRAW);

    // Unset VBO/VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    r_data->aspect_ratio = 1.f;
    r_data->display_h = 600;
    r_data->display_w = 600;
    r_data->window = window;
}

void render_pipeline_update(RenderPipelineData* const r_data, const int display_w, const int display_h)
{
    r_data->display_w = display_w;
    r_data->display_h = display_h;
    r_data->aspect_ratio  = ((float)display_h) / ((float)display_w);
}

void render_pipeline_destroy(RenderPipelineData* const r_data)
{
    glDeleteVertexArrays(1, &r_data->particles_vao);
    glDeleteVertexArrays(1, &r_data->planets_vao);
    glDeleteVertexArrays(1, &r_data->environment_vao);
    glDeleteBuffers(1, &r_data->particles_vbo);
    glDeleteBuffers(1, &r_data->planets_vbo);
    glDeleteBuffers(1, &r_data->environment_vbo);
    glDeleteProgram(r_data->particles_shader);
    glDeleteProgram(r_data->planets_shader);
    glDeleteProgram(r_data->environment_shader);
}

void render_pipeline_draw_points(const GLuint vao, const GLuint vbo, const Vec2* const points, const int n_points)
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );
    glBufferSubData(GL_ARRAY_BUFFER, 0, n_points * sizeof(Vec2), points);
    glDrawArrays(GL_POINTS, 0, n_points);
}

void render_pipeline_draw_points_with_direction(const GLuint vao, const GLuint vbo, const Vec2* const points, const Vec2* const directions, const int n_points)
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        sizeof(float) * 2,  // stride
        (void*)0            // array buffer offset
    );
    glBufferSubData(GL_ARRAY_BUFFER, 0, n_points * sizeof(Vec2), points);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                  // attribute 1. No particular reason for 1, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        sizeof(float) * 2,  // stride
        (void*)(n_points * sizeof(Vec2)) // array buffer offset
    );
    glBufferSubData(GL_ARRAY_BUFFER, n_points * sizeof(Vec2), n_points * sizeof(Vec2), directions);
    glDrawArrays(GL_POINTS, 0, n_points);
}

void render_pipeline_draw_lines(const GLuint vao, const GLuint vbo, const Line* const lines, const int n_lines)
{
    // TODO(optimization) environment data need only be uploaded once, so glBufferData is redundant on
    //                    each update. Make a separate upload function and call on loop entry, or keep things
    //                    this way if the environment is to be dynamic
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );
    glBufferSubData(GL_ARRAY_BUFFER, 0, n_lines * sizeof(Line), lines);
    glDrawArrays(GL_LINES, 0, 2 * n_lines);
}

void render_pipeline_draw_planets(RenderPipelineData* const r_data, const Planets* const planets)
{
    const int n_points = planets->n_active;
    glUseProgram(r_data->planets_shader);
    glUniform1f(glGetUniformLocation(r_data->planets_shader, "uAspectRatio"), r_data->aspect_ratio);
    glBindVertexArray(r_data->planets_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->planets_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        sizeof(float) * 2,  // stride
        (void*)0            // array buffer offset
    );
    glBufferSubData(GL_ARRAY_BUFFER, 0, n_points * sizeof(Vec2), planets->positions);

    // Pack age/mass properties into a "vec2" as [age0, mass0, age1, mass1, ...]
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                  // attribute 1. No particular reason for 1, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        sizeof(float) * 2,  // stride
        (void*)(n_points * sizeof(Vec2)) // array buffer offset
    );
    glBufferSubData(GL_ARRAY_BUFFER, n_points * sizeof(Vec2), n_points * sizeof(Vec2), planets->properties);
    glDrawArrays(GL_POINTS, 0, n_points);
}

void render_pipeline_draw_particles(RenderPipelineData* const r_data, const Particles* const particles)
{
    glUseProgram(r_data->particles_shader);
    glUniform1f(glGetUniformLocation(r_data->particles_shader, "uAspectRatio"), r_data->aspect_ratio);
    render_pipeline_draw_points_with_direction(r_data->particles_vao, r_data->particles_vbo, particles->positions, particles->velocities, particles->n_active);
}

void render_pipeline_draw_environment(RenderPipelineData* const r_data, const Environment* const environment)
{
    const int n_boundaries = environment->n_boundaries;
    glUseProgram(r_data->environment_shader);
    glUniform1f(glGetUniformLocation(r_data->environment_shader, "uAspectRatio"), r_data->aspect_ratio);
    glBindVertexArray(r_data->environment_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r_data->environment_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        sizeof(float) * 2,  // stride
        (void*)0            // array buffer offset
    );
    glBufferSubData(GL_ARRAY_BUFFER, 0, n_boundaries * sizeof(Line), environment->boundaries);

    // Pack age/mass properties into a "vec2" as [age0, mass0, age1, mass1, ...]
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                  // attribute 1. No particular reason for 1, but must match the layout in the shader.
        1,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        sizeof(float),      // stride
        (void*)(n_boundaries * sizeof(Line)) // array buffer offset
    );
    glBufferSubData(GL_ARRAY_BUFFER, n_boundaries * sizeof(Line), n_boundaries * sizeof(Vec2), environment->boundary_properties);
    glDrawArrays(GL_LINES, 0, 2 * n_boundaries);
}

Vec2 render_pipeline_get_screen_mouse_position(RenderPipelineData* const r_data)
{
    Vec2 cursor_position;
    double xpos_raw, ypos_raw;
    glfwGetCursorPos(r_data->window, &xpos_raw, &ypos_raw);
    cursor_position.x = (2.f * ((float)xpos_raw / r_data->display_w) - 1.f) / r_data->aspect_ratio;
    cursor_position.y = (1.f - 2.f * ((float)ypos_raw / r_data->display_h));
    return cursor_position;
}


#if defined(PLATFORM_SUPPORTS_AUDIO)
static inline ALenum to_al_format(short channels, short samples)
{
    bool stereo = (channels > 1);

    switch (samples) {
    case 16:
        if (stereo)
            return AL_FORMAT_STEREO16;
        else
            return AL_FORMAT_MONO16;
    case 8:
        if (stereo)
            return AL_FORMAT_STEREO8;
        else
            return AL_FORMAT_MONO8;
    default:
        return -1;
    }
}

static inline const char* al_error_to_str(const ALenum error)
{
    switch (error)
    {
        case AL_NO_ERROR: return "AL_NO_ERROR";
        case AL_INVALID_NAME: return "AL_INVALID_NAME";
        case AL_INVALID_ENUM: return "AL_INVALID_ENUM";
        case AL_INVALID_VALUE: return "AL_INVALID_VALUE";
        case AL_INVALID_OPERATION: return "AL_INVALID_OPERATION";
        case AL_OUT_OF_MEMORY: return "AL_OUT_OF_MEMORY";
    }
    return "I have no idea...";
}


#ifndef NDEBUG
    #define AL_TEST_ERROR_RET(statement, retval) {       \
        (statement);\
        const ALCenum al_error = alGetError();  \
        if (al_error != AL_NO_ERROR) {  \
            fprintf(stderr, "FAILED: " #statement " :: (%s) \n", al_error_to_str(al_error)); \
            return retval;                   \
        }\
        else\
        {\
            fprintf(stderr, "PASSED: " #statement "\n");\
        }}
#else
    #define AL_TEST_ERROR_RET(statement, _) statement
#endif // NDEBUG

#define AL_TEST_ERROR(statement) AL_TEST_ERROR_RET(statement, -1)

static ALuint read_wav_file_to_buffer(const char* filename)
{
    ALuint buffer;
    AL_TEST_ERROR_RET(alGenBuffers(1, &buffer), AL_NONE);

    /* load data */
    WaveInfo* const wave = WaveOpenFileForReading(filename);
    if (wave == nullptr)
    {
        std::printf("[read_wav_file_to_buffer] FILENAME (%s) NOT FOUND", filename);
        return AL_NONE;
    }

    {
        const int retcode = WaveSeekFile(0, wave);
        if (retcode)
        {
            std::printf("[read_wav_file_to_buffer] FAILED TO SEEK WAVEFILE");
            return AL_NONE;
        }
    }

    char* const buffer_data = (char*)std::malloc(wave->dataSize);
    if (buffer_data == nullptr)
    {
        std::printf("[read_wav_file_to_buffer] FAILED ALLOCATE MEMORY FOR WAVE");
        return AL_NONE;
    }

    {
        const unsigned read_size = WaveReadFile(buffer_data, wave->dataSize, wave);
        if (read_size != wave->dataSize)
        {
            std::printf("[read_wav_file_to_buffer] SHORT READ FOR WAVE FILE (%d) VS EXPECTED (%d)", read_size, wave->dataSize);
            return AL_NONE;
        }
    }

    AL_TEST_ERROR_RET(alBufferData(buffer, to_al_format(wave->channels, wave->bitsPerSample), buffer_data, wave->dataSize, wave->sampleRate), AL_NONE);
    std::free(buffer_data);
    return buffer;
}

static inline ALenum al_replay_sound(const GLuint sound_source, const GLuint sound_buffer)
{
    AL_TEST_ERROR_RET(alSourcei(sound_source, AL_BUFFER, sound_buffer), AL_NONE);
    AL_TEST_ERROR_RET(alSourcePlay(sound_source), AL_NONE);
    return AL_NONE;
}

static inline ALenum al_play_sound(const GLuint sound_source, const GLuint sound_buffer)
{
    ALint source_state;
    AL_TEST_ERROR_RET(alGetSourcei(sound_source, AL_SOURCE_STATE, &source_state), AL_NONE);
    if (source_state != AL_PLAYING)
    {
        return al_replay_sound(sound_source, sound_buffer);
    }
    return AL_NONE;
}

#endif // defined(PLATFORM_SUPPORTS_AUDIO)


int main(int, char**)
{
#if defined(PLATFORM_SUPPORTS_AUDIO)
    // Setup audio device
    ALCdevice* const audio_device = alcOpenDevice(alcGetString(NULL, ALC_DEVICE_SPECIFIER));
    if (audio_device == nullptr)
    {
        std::printf("Unable to initialize default audio device\n");
        return -1;
    }
    else
    {
        std::printf("Initialized default audio device: %s\n", alcGetString(audio_device, ALC_DEVICE_SPECIFIER));
    }

    // Create audio context
    ALCcontext* const audio_context = alcCreateContext(audio_device, NULL);
    if (alcMakeContextCurrent(audio_context))
    {
        std::printf("Prepared audio context\n");
    }
    else
    {
        std::printf("Failed to prepare audio context\n");
        return -1;
    }

    // Set audio listener
    {
        AL_TEST_ERROR(alListener3f(AL_POSITION, 0, 0, 1.0f));
        AL_TEST_ERROR(alListener3f(AL_VELOCITY, 0, 0, 0));
        const ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
        AL_TEST_ERROR(alListenerfv(AL_ORIENTATION, listenerOri));
    }

    // Prepare sfx sourcs
    ALuint sfx_sources[2];
    AL_TEST_ERROR(alGenSources((ALuint)2, sfx_sources));
    for (int i = 0; i < 2; ++i)
    {
        AL_TEST_ERROR(alSourcef(sfx_sources[i], AL_PITCH, 1.0f));
        AL_TEST_ERROR(alSourcef(sfx_sources[i], AL_GAIN, 1.5f));
        AL_TEST_ERROR(alSource3f(sfx_sources[i], AL_POSITION, 0, 0, 0));
        AL_TEST_ERROR(alSource3f(sfx_sources[i], AL_VELOCITY, 0, 0, 0));
        AL_TEST_ERROR(alSourcei(sfx_sources[i], AL_LOOPING, AL_FALSE));
    }

    // Load SFX
    const ALuint sfx_buffers[2] = {
        read_wav_file_to_buffer("assets/smw_coin.wav"),
        read_wav_file_to_buffer("assets/smw_coin.wav"),
    };

#if defined(PLATFORM_SUPPORTS_AUDIO)
    al_play_sound(sfx_sources[0], sfx_buffers[0]);
#endif // defined(PLATFORM_SUPPORTS_AUDIO)

    // Prepare music track sources
    ALuint audio_sources[17];
    AL_TEST_ERROR(alGenSources((ALuint)17, audio_sources));
    for (int i = 0; i < 17; ++i)
    {
        AL_TEST_ERROR(alSourcef(audio_sources[i], AL_PITCH, 0.5f));
        AL_TEST_ERROR(alSourcef(audio_sources[i], AL_GAIN, 0.0f));
        AL_TEST_ERROR(alSource3f(audio_sources[i], AL_POSITION, 0, 0, 0));
        AL_TEST_ERROR(alSource3f(audio_sources[i], AL_VELOCITY, 0, 0, 0));
        AL_TEST_ERROR(alSourcei(audio_sources[i], AL_LOOPING, AL_TRUE));
    }

    // Load music
    const ALuint audio_track_buffers[17] = {
        read_wav_file_to_buffer("assets/track_1.wav"),
        read_wav_file_to_buffer("assets/track_2.wav"),
        read_wav_file_to_buffer("assets/track_3.wav"),
        read_wav_file_to_buffer("assets/track_4.wav"),
        read_wav_file_to_buffer("assets/track_5.wav"),
        read_wav_file_to_buffer("assets/track_6.wav"),
        read_wav_file_to_buffer("assets/track_7.wav"),
        read_wav_file_to_buffer("assets/track_8.wav"),
        read_wav_file_to_buffer("assets/track_9.wav"),
        read_wav_file_to_buffer("assets/track_10.wav"),
        read_wav_file_to_buffer("assets/track_11.wav"),
        read_wav_file_to_buffer("assets/track_12.wav"),
        read_wav_file_to_buffer("assets/track_13.wav"),
        read_wav_file_to_buffer("assets/track_14.wav"),
        read_wav_file_to_buffer("assets/track_15.wav"),
        read_wav_file_to_buffer("assets/track_1.wav"),
        read_wav_file_to_buffer("assets/track_14.wav")
    };

    // Start playing all tracks
    for (int i = 0; i < 17; ++i)
    {
        AL_TEST_ERROR(alSourcei(audio_sources[i], AL_BUFFER, audio_track_buffers[i]));
        AL_TEST_ERROR(alSourcePlay(audio_sources[i]));
    }
#endif // defined(PLATFORM_SUPPORTS_AUDIO)

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

    environment_add_boundary(
        &env,
        Vec2{-0.2f * BOUNDARY_LIMIT, -0.2 * BOUNDARY_LIMIT},
        Vec2{+0.2f * BOUNDARY_LIMIT, +0.2 * BOUNDARY_LIMIT}
    );

    env.goal = aabb_create(
        Vec2{BOUNDARY_LIMIT - 0.4, BOUNDARY_LIMIT - 0.4},
        Vec2{BOUNDARY_LIMIT - 0.1, BOUNDARY_LIMIT - 0.1}
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
    render_pipeline_initialize(
        &render_pipeline_data,
        window,
        &planets,
        &particles,
        &env
    );

    // Update current used input states
    UserInputState input_state;
    user_input_state_initialized(&input_state, window);

    float freq_min = 60.f;
    float dt_max = 1.f / freq_min;
    float next_planet_mass = 0.5f;
    bool next_planet_assymetric_grav = false;

    int score = -1;
    const int min_required_score = 100;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        const float dt_raw = ImGui::GetIO().DeltaTime;
        const float dt = std::fmin(dt_max, dt_raw);

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();



        // Update game user input stuff first
        user_input_state_update(&input_state, window);

        ImGui::Begin("Debug stuff", nullptr, ImGuiWindowFlags_NoTitleBar);
        if (score < 0)
        {
            ImGui::OpenPopup("start-menu-proto");
            ImGui::BeginPopupModal("start-menu-proto");
            ImGui::Text("The goal is to sling particles to the upper right corner.\n"
                        "Doing so accumulates points. Planets are added to create\n"
                        "gravitational fields to pull particles.");
            ImGui::Dummy(ImVec2{1, 30});
            ImGui::Text("Controls");
            ImGui::Dummy(ImVec2{1, 30});
            ImGui::Text("L-CTRL  + LMB : Spawn a single particle");
            ImGui::Text("L-SHIFT + LMB : Spawn a MANY particles");
            ImGui::Text("          LMB : Spawn a planet");
            ImGui::Dummy(ImVec2{1, 30});
            if (ImGui::Button("START"))
            {
                score = 0;
            }
            ImGui::EndPopup();
        }
        else if (score < min_required_score)
        {
            // Update/reset environment state
            environment_update(&env, dt);

            // Don't allow game interation if the debug panel is hovered
            if (ImGui::IsWindowHovered())
            {
                // TRAP
            }
            // Spawn single particle on click
            else if (input_state.current.fields.left_ctrl && input_state.pressed.fields.left_mouse_button)
            {
                const Vec2 mouse_position_world =
                    render_pipeline_get_screen_mouse_position(&render_pipeline_data);
                particles_spawn_at(&particles, mouse_position_world);
            }
            // Spew particles from mouse
            else if (input_state.current.fields.left_shift && input_state.current.fields.left_mouse_button)
            {
                const Vec2 mouse_position_world =
                    render_pipeline_get_screen_mouse_position(&render_pipeline_data);
                particles_spawn_at(&particles, mouse_position_world);
            }
            // Spawn single planet on click
            else if (input_state.pressed.fields.left_mouse_button)
            {
                const Vec2 mouse_position_world =
                    render_pipeline_get_screen_mouse_position(&render_pipeline_data);

                if (next_planet_assymetric_grav)
                {
                    planets_spawn_at(&planets, mouse_position_world, Vec2{0, 1}, next_planet_mass);
                }
                else
                {
                    planets_spawn_at(&planets, mouse_position_world, Vec2{0, 0}, next_planet_mass);
                }
            }
            // Spawn / grab single planet which follows the cursor
            else if (input_state.current.fields.key_f)
            {
                const Vec2 mouse_position_world =
                    render_pipeline_get_screen_mouse_position(&render_pipeline_data);
                if (planets.n_active > 0)
                {
                    planets.positions[0] = mouse_position_world;
                }
                else if (next_planet_assymetric_grav)
                {
                    planets_spawn_at(&planets, mouse_position_world, Vec2{0, 1}, next_planet_mass);
                }
                else
                {
                    planets_spawn_at(&planets, mouse_position_world, Vec2{0, 0}, next_planet_mass);
                }
            }

            // Apply planet gravity to particles
            planets_apply_to_particles(&planets, &env, &particles);

            // Check for particles in the goal region
            for (int i = 0; i < particles.n_active; ++i)
            {
                // Stop these particles here
                if (aabb_within(&env.goal, particles.positions + i))
                {
                    if (vec2_length_squared(particles.velocities + i) > 0.f)
                    {
#if defined(PLATFORM_SUPPORTS_AUDIO)
                        al_replay_sound(sfx_sources[0], sfx_buffers[0]);
#endif // defined(PLATFORM_SUPPORTS_AUDIO)

                        ++score;
                    }
                    vec2_set_zero(particles.forces + i);
                    vec2_set_zero(particles.velocities + i);
                }
            }

            // Do planet update
            planets_update(&planets, dt);

            // Do particle update
            particles_update(&particles, &env, dt);

#if defined(PLATFORM_SUPPORTS_AUDIO)
            // Play sounds based on positions
            unsigned in_zone[16];
            std::memset(in_zone, 0, sizeof(unsigned) * 16);
            for (int i = 0; i < particles.n_active; ++i)
            {
                const int xd = ((particles.positions + i)->x + 1.f) / 0.5f;
                const int yd = ((particles.positions + i)->y + 1.f) / 0.5f;
                in_zone[xd * 4 + yd] += 1;
            }
            for (int z = 0; z < 16; ++z)
            {
                const float gain = std::fmin(1.f, (float)in_zone[z] / 4.f);
                AL_TEST_ERROR(alSourcef(audio_sources[z], AL_GAIN, gain));
            }

            // Update background track
            {
                const float gain = std::fmin(1.f, (float)planets.n_active / 3.f);
                AL_TEST_ERROR(alSourcef(audio_sources[16], AL_GAIN, gain));
            }
#endif // defined(PLATFORM_SUPPORTS_AUDIO)

            ImGui::Text("SCORE (%d) of (%d)", score, min_required_score);
            ImGui::Dummy(ImVec2{1, 30});
            ImGui::Text("Particles  : (%d)", particles.n_active);
            ImGui::Text("Boundaries : (%d)", env.n_boundaries);
            if (ImGui::SliderFloat("min update rate", (float*)(&freq_min), 30.0, 120.0))
            {
                dt_max = 1./ freq_min;
            }
            ImGui::InputFloat2("gravity", (float*)(&env.gravity));
            ImGui::SliderFloat("dampening", &env.dampening, 0.1f, 1.f);
            ImGui::SliderFloat("max particle velocity", &particles.max_velocity, 0.5f, 5.f);
            ImGui::SliderFloat("next planet mass", &next_planet_mass, 0.1f, 2.f);
            ImGui::Checkbox("next gravity assymetric", &next_planet_assymetric_grav);
            if (ImGui::SmallButton("Clear particles"))
            {
                particles_clear(&particles);
            }
            if (ImGui::SmallButton("Clear planets"))
            {
                planets_clear(&planets);
            }
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }
        // Create a popup on win
        else
        {
            ImGui::OpenPopup("YOU WIN!");
            ImGui::BeginPopupModal("YOU WIN!");
            ImGui::Text("Thanks for playing!");
            if (ImGui::Button("Reset?"))
            {
#if defined(PLATFORM_SUPPORTS_AUDIO)
                // Reset all tracks to spoopy speed
                for (int z = 0; z < 17; ++z)
                {
                    AL_TEST_ERROR(alSourcef(audio_sources[z], AL_PITCH, 0.5f));
                    AL_TEST_ERROR(alSourcef(audio_sources[z], AL_GAIN, 0.f));
                }
#endif  // defined(PLATFORM_SUPPORTS_AUDIO)
                score = 0;
                planets_clear(&planets);
                particles_clear(&particles);
            }
            else
            {
#if defined(PLATFORM_SUPPORTS_AUDIO)
                // Reset all tracks to spoopy speed
                for (int z = 0; z < 17; ++z)
                {
                    AL_TEST_ERROR(alSourcef(audio_sources[z], AL_GAIN, 0.f));
                }
                // Play some tracks at normal speed on win
                AL_TEST_ERROR(alSourcef(audio_sources[5], AL_PITCH, 1.00f));
                AL_TEST_ERROR(alSourcef(audio_sources[5], AL_GAIN, 0.25f));
                AL_TEST_ERROR(alSourcef(audio_sources[9], AL_PITCH, 1.00f));
                AL_TEST_ERROR(alSourcef(audio_sources[9], AL_GAIN, 0.25f));
                AL_TEST_ERROR(alSourcef(audio_sources[11], AL_PITCH, 1.00f));
                AL_TEST_ERROR(alSourcef(audio_sources[11], AL_GAIN, 0.25f));
                AL_TEST_ERROR(alSourcef(audio_sources[13], AL_PITCH, 1.00f));
                AL_TEST_ERROR(alSourcef(audio_sources[14], AL_GAIN, 0.25f));
#endif  // defined(PLATFORM_SUPPORTS_AUDIO)
            }
            ImGui::EndPopup();
        }
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
        render_pipeline_update(&render_pipeline_data, display_w, display_h);
        render_pipeline_draw_environment(&render_pipeline_data, &env);
        render_pipeline_draw_planets(&render_pipeline_data, &planets);
        render_pipeline_draw_particles(&render_pipeline_data, &particles);

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

#if defined(PLATFORM_SUPPORTS_AUDIO)
    // Cleanup audio
    alDeleteSources(16, audio_sources);
    alDeleteBuffers(16, audio_track_buffers);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(audio_context);
    alcCloseDevice(audio_device);
#endif // defined(PLATFORM_SUPPORTS_AUDIO)

    return 0;
}
