#include "ParticleSystem.hpp"
#include <iostream>

using namespace glm;
using namespace std;

static const GLfloat g_vertex_buffer_data[] = {-0.5f, -0.5f,
                                               0.5f, -0.5f,
                                               -0.5f, 0.5f,
                                               0.5f, 0.5f};

static std::random_device rd;  // Obtain a random number from hardware
static std::mt19937 eng(rd()); // Seed the generator
static std::uniform_real_distribution<> distr(0.0f, 1.0f);
static float size_end = 0.1f;
static vec3 color_end = vec3(0.1f, 0.1f, 0.1f);
static const float RING_DENSIY = 4.0f;
static float VELOCITY_CONTROL;
static uint NUM_EMITTERS = 0;
static vec3 explosion_position;
static const vec3 world_up = vec3(0.0f, 1.0f, 0.0f);

ParticleSystem::ParticleSystem()
{
    m_particles.resize(MAX_PARTICLES);
    for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
    {
        m_particles[i].index = i;
        available_particles.push(i);
    }
    particle_count = 0;
}

ParticleSystem::~ParticleSystem()
{
    glDeleteBuffers(1, &square_model_vbo);
    glDeleteBuffers(1, &instance_transform_vbo);
    glDeleteVertexArrays(1, &particle_system_vao);
    CHECK_GL_ERRORS;
}

void ParticleSystem::InitParticleSystem(ShaderProgram &shader, vec3 explosion_origin, float velocity_control, vec3 bg_colour)
{
    VELOCITY_CONTROL = velocity_control;
    explosion_position = explosion_origin;
    m_shader = shader;

    m_shader.enable();
    // setup vao
    glGenVertexArrays(1, &particle_system_vao);
    glBindVertexArray(particle_system_vao);

    VP_location = m_shader.getUniformLocation("VP");

    glUniform3fv(m_shader.getUniformLocation("backgroundColor"), 1, &bg_colour[0]);

    // setup square vert
    glGenBuffers(1, &square_model_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, square_model_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glVertexAttribDivisor(0, 0);

    // setup vbo
    glGenBuffers(1, &instance_transform_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, instance_transform_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(glm::mat4), nullptr, GL_STREAM_DRAW);

    for (size_t i = 0; i < 4; ++i)
    {
        glEnableVertexAttribArray(i + 1);
        glVertexAttribPointer(i + 1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const void *)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(i + 1, 1);
    }

    // color attrib
    glGenBuffers(1, &instance_color_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, instance_color_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(glm::vec4), nullptr, GL_STREAM_DRAW);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *)0);
    glVertexAttribDivisor(5, 1);

    m_shader.disable(); // this is causing an error

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECK_GL_ERRORS;
}

void ParticleSystem::UpdateParticles(const vec3 camera_pos, const mat4 &view_projection, const mat4 &view)
{
    // sort first
    this->camera_pos = camera_pos;
    m_view_projection = view_projection;
    for (auto it = active_particles.begin(); it != active_particles.end();)
    {
        Particle &particle = m_particles[it->index];
        if (particle.frames_remaining <= 0)
        {
            NUM_EMITTERS--;
            available_particles.push(it->index);
            particle.children.clear();
            it = active_particles.erase(it); // Erase returns the iterator following the last removed element
            continue;
        }
        break; // Since the set is ordered, no need to check further
    }
    for (auto it = active_particles.begin(); it != active_particles.end(); ++it)
    {
        Particle &particle = m_particles[it->index];
        uint parent_idx = particle.index;
        // cout << "children size: " << particle.children.size() << endl;
        UpdateData(parent_idx, parent_idx);
        for (uint child : particle.children)
        {
            UpdateData(child, parent_idx);
        }
        // cout << "children size: " << m_particles[parent_idx].children.size() << endl;
    }
}

void ParticleSystem::UpdateData(uint index, uint parent_index)
{
    Particle &particle = m_particles[index];
    particle.frames_remaining -= 1;
    particle.position += particle.velocity;

    float life_factor = float(particle.frames_remaining) / float(particle.life_frames);
    particle.size = glm::lerp(size_end, particle.size_begin, life_factor);
    particle.color = glm::lerp(particle.color_end, particle.color_begin, life_factor);
    particle.alpha = glm::lerp(0.3f, particle.alpha, life_factor);

    if (parent_index == index && particle.frames_remaining % 10 == 0)
    {
        cout << "EMMITING PARTICLES" << endl;
        ParticleAttribs attrib = {};
        attrib.velocity = particle.velocity;
        attrib.size_begin = particle.size_begin;
        attrib.size = particle.size;
        attrib.color_begin = particle.color_begin;
        attrib.color = particle.color;
        attrib.color_end = particle.color_end;
        attrib.alpha = particle.alpha;
        attrib.creation_time = particle.creation_time;
        attrib.parent_index = parent_index;

        glm::vec3 direction = particle.position - explosion_position;
        float radius = glm::length(direction);
        float initial_angle = atan2(direction.z, direction.x);

        int ring_count = NUM_EMITTERS;
        if (!particle.children.empty())
        {
            ring_count *= particle.children.size();
        }
        // cout << "Radius " << radius << " Ring count " << ring_count << endl;
        const int EMISSION_RATE = floor(RING_DENSIY * 2.0f * glm::pi<float>() * radius / float(ring_count));
        const float angle_step = 2 * glm::pi<float>() / EMISSION_RATE;
        // cout << "Emission rate: " << EMISSION_RATE << endl;

        for (int i = 0; i < EMISSION_RATE; ++i)
        {
            float angle = initial_angle + i * angle_step;
            glm::vec3 offset = glm::vec3(cos(angle) * radius, 0.0f, sin(angle) * radius);
            glm::vec3 emit_pos = explosion_position + offset;
            // cout << "Emit pos: " << emit_pos.x << " " << emit_pos.y << " " << emit_pos.z << endl;
            attrib.position = emit_pos;
            // UPDATE VELOCITY TO SPREAD PARTICLES UP/OUT
            // cout << "Explosion pos: " << explosion_position.x << " " << explosion_position.y << " " << explosion_position.z << endl;
            attrib.velocity = glm::normalize(emit_pos - explosion_position) * VELOCITY_CONTROL; // velocity magnitude
            attrib.velocity.y = 0.5f;                                                           // upwards velocity
            EmitParticle(attrib, false);
        }
    }

    // update rotation
    vec3 forward = normalize(particle.position - camera_pos);
    vec3 right = cross(world_up, forward);
    vec3 up = cross(forward, right);

    particle.rotation = glm::mat4(1.0f);
    particle.rotation[0] = vec4(right, 0.0f);
    particle.rotation[1] = vec4(up, 0.0f);
    particle.rotation[2] = vec4(forward, 0.0f);

    // // cout << "updating transform for index: " << index << endl;
    particle.update_transform();
    // // cout << "transform: " << particle.transform << endl;
}

static vec3 random_vec3()
{
    return vec3(distr(eng) - 0.5f, distr(eng) - 0.5f, distr(eng) - 0.5f);
}

void ParticleSystem::EmitParticle(ParticleAttribs attribs, bool isEmitter)
{
    if (available_particles.empty())
    {
        return;
    }
    int particle_index = available_particles.front();
    available_particles.pop();

    Particle &particle = m_particles[particle_index];
    particle.position = min(attribs.position, vec3(15, 15, 15)) + random_vec3();
    if (!isEmitter)
    {
        // cout << "pos': " << particle.position.x << " " << particle.position.y << " " << particle.position.z << endl;
        // exit(0);
    }
    particle.velocity = attribs.velocity + random_vec3();

    particle.size_begin = attribs.size_begin;   // + distr(eng) - 0.5f;
    particle.size = attribs.size;               // + distr(eng) - 0.5f;
    particle.color_begin = attribs.color_begin; // + random_vec3();
    particle.color = attribs.color;             // + random_vec3();
    particle.color_end = attribs.color_end;     // + random_vec3();
    particle.alpha = attribs.alpha;             // + distr(eng) - 0.5f;
    particle.isEmitter = isEmitter;

    particle.life_frames = 100; // keep particle alive for 100 frames
    particle.frames_remaining = particle.life_frames;

    if (isEmitter)
    {
        NUM_EMITTERS++;
        ParticleInfo pInfo(particle_index, attribs.creation_time);
        active_particles.insert(pInfo);
    }
    else
    {
        m_particles[attribs.parent_index].children.push_back(particle_index);
        // cout << "child index: " << particle_index << endl;
    }
    particle.update_transform();
    ++particle_count;
}

void ParticleSystem::DrawParticles()
{

    std::vector<glm::mat4> transforms;
    std::vector<glm::vec4> colors;
    for (auto pInfo : active_particles)
    {
        Particle &particle = m_particles[pInfo.index];
        transforms.push_back(particle.transform);
        colors.push_back(glm::vec4(particle.color, particle.alpha));

        for (uint child_idx : particle.children)
        {
            Particle &c_particle = m_particles[child_idx];
            vec3 particle_center_ws = vec3(c_particle.transform * vec4(0.0f, 0.0f, 0.0f, 1.0f));
            // cout << "child pos: " << particle_center_ws.x << " " << particle_center_ws.y << " " << particle_center_ws.z << endl;
            transforms.push_back(c_particle.transform);
            colors.push_back(glm::vec4(c_particle.color, c_particle.alpha));
        }
        if (!particle.children.empty())
        {
            // exit(0);
        }
    }

    if (transforms.empty())
    {
        return;
    }

    m_shader.enable();
    glBindVertexArray(particle_system_vao);

    glBindBuffer(GL_ARRAY_BUFFER, instance_transform_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, transforms.size() * sizeof(glm::mat4), transforms.data());

    glBindBuffer(GL_ARRAY_BUFFER, instance_color_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(glm::vec4), colors.data());

    glUniformMatrix4fv(VP_location, 1, GL_FALSE, &m_view_projection[0][0]);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particle_count);
    m_shader.disable();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECK_GL_ERRORS;
    // cout << "NUM EMITTERS: " << NUM_EMITTERS << endl;
}