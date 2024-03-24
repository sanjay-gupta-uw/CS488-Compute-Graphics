#include <glm/glm.hpp>
#include <algorithm>

struct Particle
{
    glm::vec3 pos, speed;
    unsigned char r, g, b, a; // Color
    float size, angle, weight;
    float life;           // Remaining life of the particle. if <0 : dead and unused.
    float cameradistance; // *Squared* distance to the camera. if dead : -1.0f

    Particle()
    {
        life = -1.0f;
        cameradistance = -1.0f;
    }

    bool operator<(const Particle &that) const
    {
        // Sort in reverse order : far particles drawn first.
        return this->cameradistance > that.cameradistance;
    }
};

class ParticleData
{
public:
    static const int MAX_PARTICLES = 10000;
    GLuint billboard_vertex_buffer;
    GLuint particles_position_buffer;
    GLuint particles_color_buffer;
    GLfloat *g_particle_position_size_data;
    GLubyte *g_particle_color_data;

    Particle particles_container[MAX_PARTICLES];
    int last_used_particle;
    int ParticlesCount;

    ParticleData()
    {
        g_particle_position_size_data = new GLfloat[MAX_PARTICLES * 4];
        g_particle_color_data = new GLubyte[MAX_PARTICLES * 4];
        last_used_particle = 0;
    }

    ~ParticleData()
    {
        delete[] g_particle_position_size_data;
        delete[] g_particle_color_data;
    }

    int FindUnusedParticle()
    {
        for (int i = last_used_particle; i < MAX_PARTICLES; ++i)
        {
            if (particles_container[i].life < 0)
            {
                last_used_particle = i;
                return i;
            }
        }

        for (int i = 0; i < last_used_particle; i++)
        {
            if (particles_container[i].life < 0)
            {
                last_used_particle = i;
                return i;
            }
        }

        return 0; // All particles are taken, override the first one
    }

    void SortParticles()
    {
        std::sort(&particles_container[0], &particles_container[MAX_PARTICLES]);
    }
};