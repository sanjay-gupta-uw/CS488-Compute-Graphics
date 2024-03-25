#include <glm/glm.hpp>
#include <algorithm>
#include <vector>
#include <deque>

struct ParticleAttribs
{
 glm::vec3 m_position, velocity, velocity_variation;
 glm::vec4 m_color_start, m_color_end;
 float size_begin, size_end, size_variation;
};

class ParticleSystem
{
public:
 ParticleSystem();

 void EmitParticle(ParticleAttribs attribs);

 void UpdateParticles(double elapsed_time, const glm::mat4 &view_projection);
 void DrawParticles();

private:
 struct Particle
 {
  glm::vec3 m_position, velocity;
  glm::vec4 m_color_start, m_color_end, color;
  float angle, size_begin, size_end, size;
  float life_time = -1.0f;
  float life_remaining = -1.0f;
  bool alive = false;
  glm::mat4 transform;

  void update_transform()
  {
   transform = glm::mat4(1.0f);
   transform = glm::translate(transform, m_position);
   transform = glm::rotate(transform, angle, glm::vec3(0.0f, 0.0f, 1.0f));
   transform = glm::scale(transform, glm::vec3(size));
  }
 };

 GLuint particle_system_vao;
 GLuint square_model_vbo, instance_transform_vbo, instance_color_vbo;
 GLuint VP_location;
 ShaderProgram m_shader;

 static const uint32_t MAX_PARTICLES = 10000;
 std::vector<Particle> m_particles;
 uint32_t particle_count;
 deque<uint32_t> available_particles;
 deque<uint32_t> active_particles;

 void InitParticleSystem();
 void UpdateVBOs(const mat4 &view_projection);
};
