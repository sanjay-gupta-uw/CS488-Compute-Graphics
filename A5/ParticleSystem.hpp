#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>

#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/CS488Window.hpp"

#include <memory>
#include <algorithm>
#include <vector>
#include <deque>
#include <random>
#include <string>

struct ParticleAttribs
{
    glm::vec3 m_position, velocity, velocity_variation;
    glm::vec4 m_color_start, m_color_end;
    float size_begin, size_end, size_variation;
};

class ParticleSystem : public CS488Window
{
public:
    ParticleSystem();
    ~ParticleSystem();

    void InitParticleSystem(ShaderProgram &shader);

    void EmitParticle(ParticleAttribs attribs);

    void UpdateParticles(float elapsed_time, const glm::mat4 &view_projection);
    void DrawParticles();

    glm::vec3 center_of_rotation;
    bool is_circle = false;

private:
    struct Particle
    {
        glm::vec3 m_position, velocity, velocity_variation;
        glm::vec4 m_color_start, m_color_end, color;
        float angle, size_begin, size_end, size;
        float life_time = -1.0f;
        float life_remaining = -1.0f;
        bool alive = false;
        glm::mat4 transform;
        int sprite_index = 9;

        void update_transform()
        {
            transform = glm::mat4(1.0f);                                            // Start with identity matrix
            transform = glm::translate(transform, m_position);                      // Apply translation
            transform = glm::rotate(transform, angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Apply rotation
            transform = glm::scale(transform, glm::vec3(size));                     // Apply scaling
        }
    };
    struct SpriteData
    {
        std::string filename;
        float uv_x, uv_y, uv_w, uv_h;
        bool rotated;
    };

    GLuint particle_system_vao;
    GLuint square_model_vbo, instance_transform_vbo, instance_sprite_vbo;
    GLuint VP_location;
    ShaderProgram m_shader;

    GLuint texture_atlas_id;
    GLuint sprite_uvs_texture;

    glm::mat4 m_view_projection;

    static const uint32_t MAX_PARTICLES = 10000;
    std::vector<Particle> m_particles;
    uint32_t particle_count;
    std::deque<uint32_t> available_particles;
    std::deque<uint32_t> active_particles;

    std::vector<SpriteData> sprites;

    void UpdateVBOs();
    glm::vec3 GetCircularVelocity(Particle &particle);
};
