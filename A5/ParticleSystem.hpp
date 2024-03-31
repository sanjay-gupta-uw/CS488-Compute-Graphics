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
#include <queue>
#include <set>
#include <random>
#include <string>

struct ParticleAttribs
{
    glm::vec3 position;
    glm::vec3 velocity;
    float size_begin;
    float size;
    glm::vec3 color_begin;
    glm::vec3 color;
    glm::vec3 color_end;
    float alpha;
    double creation_time;
    int parent_index = -1;
};

struct ParticleInfo
{
    uint index;
    double creation_time;

    ParticleInfo(int idx, double creation_time) : index(idx), creation_time(creation_time) {}
};

struct Particle
{
    glm::vec3 position;
    glm::vec3 velocity;
    float size_begin;
    float size;
    glm::vec3 color_begin;
    glm::vec3 color;
    glm::vec3 color_end;
    float alpha;
    int life_frames;
    int frames_remaining;
    bool isEmitter = false;
    std::vector<int> children;
    uint index;
    double creation_time;
    // assume were using a square to model the particle

    glm::mat4 transform;
    glm::mat4 rotation;
    void update_transform()
    {
        transform = glm::mat4(1.0f);                        // Start with identity matrix
        transform = glm::translate(transform, position);    // Apply translation
        transform = transform * rotation;                   // Apply rotation
        transform = glm::scale(transform, glm::vec3(size)); // Apply scaling
    }
};

class Comparator
{
public:
    bool operator()(const ParticleInfo &a, const ParticleInfo &b) const
    {
        return a.creation_time < b.creation_time;
    }
};

class ParticleSystem : public CS488Window
{
public:
    ParticleSystem();
    ~ParticleSystem();

    void InitParticleSystem(ShaderProgram &shader, glm::vec3 explosion_origin, float velocity_control, glm::vec3 bg_colour);

    void EmitParticle(ParticleAttribs attribs, bool isEmitter = false);

    void UpdateParticles(const glm::vec3 camera_pos, const glm::mat4 &view_projection, const glm::mat4 &view);
    void DrawParticles();

    glm::vec3 center_of_rotation;
    bool is_circle = false;

private:
    GLuint particle_system_vao;
    // pass alpha as well
    GLuint square_model_vbo, instance_transform_vbo, instance_color_vbo;
    ShaderProgram m_shader;

    GLuint VP_location;
    glm::mat4 m_view_projection;

    static const uint32_t MAX_PARTICLES = 10000;
    std::vector<Particle> m_particles;
    uint32_t particle_count;

    void UpdateVBOs();
    void UpdateData(uint index, uint parent_index);

    std::queue<uint32_t> available_particles;
    std::multiset<ParticleInfo, Comparator> active_particles;
    glm::vec3 camera_pos;
};
