#include "ParticleSystem.hpp"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "json.hpp"
#include <fstream>

using namespace glm;

static const GLfloat g_vertex_buffer_data[] = {-0.5f, -0.5f,
                                               0.5f, -0.5f,
                                               -0.5f, 0.5f,
                                               0.5f, 0.5f};

static std::random_device rd;  // Obtain a random number from hardware
static std::mt19937 eng(rd()); // Seed the generator
static std::uniform_real_distribution<> distr(0.0f, 1.0f);

vec3 ParticleSystem::GetCircularVelocity(Particle &particle)
{
    vec3 to_center = center_of_rotation - particle.m_position;
    to_center.y = 0.0f;

    float distance = glm::length(to_center);
    vec3 tangent = glm::normalize(glm::cross(to_center, vec3(0.0f, 1.0f, 0.0f)));

    vec3 velocity = tangent * particle.velocity;
    return velocity;
}

ParticleSystem::ParticleSystem()
{
    m_particles.resize(MAX_PARTICLES);
    for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
    {
        available_particles.push_back(i);
    }
    particle_count = 0;
}

ParticleSystem::~ParticleSystem()
{
    glDeleteBuffers(1, &square_model_vbo);
    glDeleteBuffers(1, &instance_transform_vbo);
    // glDeleteBuffers(1, &instance_sprite_vbo);
    glDeleteVertexArrays(1, &particle_system_vao);
    CHECK_GL_ERRORS;
}

void ParticleSystem::InitParticleSystem(ShaderProgram &shader)
{

    m_shader = shader;
    m_shader.enable();
    // setup vao
    glGenVertexArrays(1, &particle_system_vao);
    glBindVertexArray(particle_system_vao);

    VP_location = m_shader.getUniformLocation("VP");

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

    // set up sprite index vbo
    glGenBuffers(1, &instance_sprite_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, instance_sprite_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(int), nullptr, GL_STREAM_DRAW);
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 1, GL_INT, 0, (void *)0);
    glVertexAttribDivisor(5, 1);

    /////
    // Load the texture
    {

        int width,
            height, nrChannels;
        unsigned char *data = stbi_load("./Assets/cloud_texture-0.png", &width, &height, &nrChannels, 0);
        if (!data)
        {
            std::cerr << "Failed to load texture" << std::endl;
            return;
        }

        glGenTextures(1, &texture_atlas_id);
        glBindTexture(GL_TEXTURE_2D, texture_atlas_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);

        // bind image aswell

        ////////
        // parse json
        std::ifstream i("./Assets/cloud_data-0.json");
        nlohmann::json j;
        i >> j;

        int spriteSheetWidth = j["meta"]["size"]["w"];
        int spriteSheetHeight = j["meta"]["size"]["h"];

        for (const auto &frame : j["frames"])
        {
            SpriteData sprite;
            sprite.filename = frame["filename"];
            int x = frame["frame"]["x"];
            int y = frame["frame"]["y"];
            int w = frame["frame"]["w"];
            int h = frame["frame"]["h"];
            sprite.rotated = frame["rotated"];

            // Calculate UVs normally if not rotated
            sprite.uv_x = x / (float)spriteSheetWidth;
            sprite.uv_y = y / (float)spriteSheetHeight;
            sprite.uv_w = w / (float)spriteSheetWidth;
            sprite.uv_h = h / (float)spriteSheetHeight;

            // Add sprite data to the vector
            sprites.push_back(sprite);
        }

        std::vector<glm::vec4> sprite_uvs;
        for (auto &sprite : sprites)
        {
            sprite_uvs.push_back(glm::vec4(sprite.uv_x, sprite.uv_y, sprite.uv_w, sprite.uv_h));
        }
        ///////
        // setup sprite uv vbo
        GLuint sprite_uvs_vbo;
        glGenBuffers(1, &sprite_uvs_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, sprite_uvs_vbo);
        glBufferData(GL_ARRAY_BUFFER, sprite_uvs.size() * sizeof(glm::vec4), sprite_uvs.data(), GL_STATIC_DRAW);

        glGenTextures(1, &sprite_uvs_texture);
        glBindTexture(GL_TEXTURE_2D, sprite_uvs_texture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, sprite_uvs_vbo);
    }

    m_shader.disable();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECK_GL_ERRORS;
}

void ParticleSystem::EmitParticle(ParticleAttribs attribs)
{
    if (available_particles.empty())
    {
        return;
    }
    int particle_index = available_particles.front();
    available_particles.pop_front();

    Particle &particle = m_particles[particle_index];
    particle.m_position = attribs.m_position;
    particle.velocity = attribs.velocity;
    particle.velocity_variation = attribs.velocity_variation;
    particle.angle = distr(eng) * 2.0f * glm::pi<float>();
    particle.m_color_start = attribs.m_color_start;
    particle.m_color_end = attribs.m_color_end;

    particle.life_time = 4.0f;
    // particle.life_time = 1.0f;
    particle.life_remaining = particle.life_time;

    particle.size_begin = attribs.size_begin; // + attribs.size_variation * (distr(eng) - 0.5f);
    particle.size_end = attribs.size_end;

    particle.alive = true;
    active_particles.push_back(particle_index);
    ++particle_count;
}

void ParticleSystem::UpdateParticles(float elapsed_time, const mat4 &view_projection)
{
    while (!active_particles.empty())
    {
        int index = active_particles.front();
        Particle &particle = m_particles[index];
        if (particle.life_remaining - elapsed_time <= 0.0f)
        {
            particle.alive = false;
            particle.life_remaining = -1.0f;
            available_particles.push_back(index);
            active_particles.pop_front();
            --particle_count;
            continue;
        }
        break;
    }
    // now all particles in active are still alive
    for (int index : active_particles)
    {
        Particle &particle = m_particles[index];
        particle.life_remaining -= elapsed_time;

        vec3 velocity = is_circle ? GetCircularVelocity(particle) : particle.velocity;
        particle.m_position += (velocity + vec3(particle.velocity_variation.x * 100 * (distr(eng) - 0.5f),
                                                particle.velocity_variation.y * 100 * (distr(eng) - 0.5f),
                                                particle.velocity_variation.z * 100 * (distr(eng) - 0.5f))) *
                               (float)elapsed_time;
        particle.angle += 0.01f * elapsed_time;

        float life_factor = particle.life_remaining / particle.life_time;
        particle.size = glm::lerp(particle.size_end, particle.size_begin, life_factor);
        particle.color = glm::lerp(particle.m_color_end, particle.m_color_start, life_factor);
        particle.update_transform();
        particle.sprite_index = int(9.0f * (1.0f - life_factor));
    }

    m_view_projection = view_projection;
}

void ParticleSystem::DrawParticles()
{
    if (active_particles.empty())
    {
        return;
    }

    m_shader.enable();
    glBindVertexArray(particle_system_vao);

    std::vector<glm::mat4> transforms;
    std::vector<int> sprite_indices;
    for (int index : active_particles)
    {
        Particle &particle = m_particles[index];

        transforms.push_back(particle.transform);
        sprite_indices.push_back(particle.sprite_index);

        // std::cout << "position: " << particle.m_position << std::endl;
    }

    glBindBuffer(GL_ARRAY_BUFFER, instance_transform_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, transforms.size() * sizeof(glm::mat4), transforms.data());

    glBindBuffer(GL_ARRAY_BUFFER, instance_sprite_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sprite_indices.size() * sizeof(int), sprite_indices.data());

    glUniformMatrix4fv(VP_location, 1, GL_FALSE, &m_view_projection[0][0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_atlas_id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sprite_uvs_texture);

    glUniform1i(glGetUniformLocation(m_shader.getProgramObject(), "SpriteAtlas"), 0);
    glUniform1i(glGetUniformLocation(m_shader.getProgramObject(), "SpriteUVS"), 1);

    CHECK_GL_ERRORS;

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particle_count);
    CHECK_GL_ERRORS;

    m_shader.disable();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECK_GL_ERRORS;
}