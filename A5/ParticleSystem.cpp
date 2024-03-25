#include "ParticleSystem.hpp"

using namespace glm;

static const GLfloat g_vertex_buffer_data[] = {-0.5f, -0.5f, 0.0f,
                                               0.5f, -0.5f, 0.0f,
                                               -0.5f, 0.5f, 0.0f,
                                               0.5f, 0.5f, 0.0f};

ParticleSystem::ParticleSystem()
{
 m_particles.resize(MAX_PARTICLES);
 InitParticleSystem();
 for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
 {
  available_particles.push(i);
 }
 particle_count = 0;
}

ParticleSystem::~ParticleSystem()
{
 glDeleteBuffers(1, &square_model_vbo);
 glDeleteBuffers(1, &instance_transform_vbo);
 glDeleteBuffers(1, &instance_color_vbo);
 glDeleteVertexArrays(1, &particle_system_vao);
 CHECK_GL_ERRORS;
}

void ParticleSystem::InitParticleSystem()
{
 // setup shader
 m_shader.generateProgramObject();
 m_shader.attachVertexShader(getAssetFilePath("Particle.vs").c_str());
 m_shader.attachFragmentShader(getAssetFilePath("Particle.fs").c_str());
 m_shader.link();
 // setup vao
 glGenVertexArrays(1, &particle_system_vao);
 glBindVertexArray(particle_system_vao);

 VP_location = m_shader.getUniformLocation("view_projection");

 // setup square vert
 glGenBuffers(1, &square_model_vbo);
 glBindBuffer(GL_ARRAY_BUFFER, square_model_vbo);
 glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
 glEnableVertexAttribArray(0);
 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
 glVertexAttribDivisor(0, 0);

 // setup vbo
 glGenBuffers(1, &instance_transform_vbo);
 glBindBuffer(GL_ARRAY_BUFFER, instance_transform_vbo);
 glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(glm::mat4), nullptr, GL_STREAM_DRAW);

 for (size_t i = 0; i < 4; ++i)
 {
  glEnableVertexAttribArray(i + 1); // Starts from 1 since 0 is already used for vertex positions
  glVertexAttribPointer(i + 1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const void *)(sizeof(glm::vec4) * i));
  glVertexAttribDivisor(i + 1, 1);
 }

 glGenBuffers(1, &instance_color_vbo);
 glBindBuffer(GL_ARRAY_BUFFER, instance_color_vbo);
 glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(glm::vec4), nullptr, GL_STREAM_DRAW);
 glEnableVertexAttribArray(5);
 glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)0);
 glVertexAttribDivisor(5, 1);

 glBindVertexArray(0);
 glBindBuffer(GL_ARRAY_BUFFER, 0);
 CHECK_GL_ERRORS;
}

void ParticleSystem::UpdateVBOs(const mat4 &view_projection)
{
 if (active_particles.empty())
 {
  return;
 }

 glBindVertexArray(particle_system_vao);

 std::vector<glm::mat4> transforms;
 std::vector<glm::vec4> colors;
 for (int &index : active_particles)
 {
  Particle &particle = m_particles[index];
  transforms.push_back(particle.transform);
  colors.push_back(particle.color);
 }

 glBindBuffer(GL_ARRAY_BUFFER, instance_transform_vbo);
 glBufferSubData(GL_ARRAY_BUFFER, 0, transforms.size() * sizeof(glm::mat4), transforms.data());
 glBindBuffer(GL_ARRAY_BUFFER, instance_color_vbo);
 glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(glm::vec4), colors.data());

 glUniformMatrix4fv(VP_location, 1, GL_FALSE, &view_projection[0][0]);

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
 particle.velocity = attribs.velocity + vec3(attribs.velocity_variation.x * (Random::Float() - 0.5f),
                                             attribs.velocity_variation.y * (Random::Float() - 0.5f),
                                             attribs.velocity_variation.z * (Random::Float() - 0.5f));

 particle.angle = Random::Float() * 2.0f * glm::pi<float>();
 particle.color_start = attribs.m_color_start;
 particle.color_end = attribs.m_color_end;

 particle.life_time = 1.0f;
 particle.life_remaining = 1.0f;

 particle.size_begin = attribs.size_begin + attribs.size_variation * (Random::Float() - 0.5f);
 particle.size_end = attribs.size_end;

 particle.alive = true;
 active_particles.push(particle_index);
 ++particle_count;
}

void ParticleSystem::UpdateParticles(double elapsed_time, const mat4 &view_projection)
{
 while (!active_particles.empty())
 {
  int index = active_particles.front();
  Particle &particle = m_particles[index];
  particle.life_remaining -= elapsed_time;
  if (particle.life_remaining <= 0.0f)
  {
   particle.alive = false;
   particle.life_remaining = -1.0f;
   available_particles.push(index);
   active_particles.pop_front();
   --particle_count;
   continue;
  }
  break;
 }
 // now all particles in active are still alive
 for (int &index : active_particles)
 {
  Particle &particle = m_particles[index];
  particle.m_position += particle.velocity * (float)elapsed_time;
  particle.angle += 0.01f * elapsed_time;
  particle.size = lerp(particle.size_end, particle.size_begin, particle.life_remaining / particle.life_time);
  particle.color = lerp(particle.color_end, particle.color_start, particle.life_remaining / particle.life_time);
  particle.update_transform();
 }

 UpdateVBOs(view_projection);
}

void ParticleSystem::DrawParticles()
{
 if (particle_count == 0)
 {
  return;
 }

 m_shader.bind();
 glBindVertexArray(particle_system_vao);
 glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particle_count);
 glBindVertexArray(0);
 glBindBuffer(GL_ARRAY_BUFFER, 0);
 m_shader.unbind();
 CHECK_GL_ERRORS;
}