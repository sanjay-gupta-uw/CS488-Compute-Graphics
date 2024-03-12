// Termm--Fall 2020

#pragma once

#include <glm/glm.hpp>

class Primitive
{
public:
  virtual ~Primitive();
  unsigned int type = 0;
};

class Sphere : public Primitive
{
public:
  Sphere() { type = 1; }
  virtual ~Sphere();
};

class Cube : public Primitive
{
public:
  Cube() { type = 2; }
  virtual ~Cube();
};

class NonhierSphere : public Primitive
{
public:
  NonhierSphere(const glm::vec3 &pos, double radius)
      : m_pos(pos), m_radius(radius)
  {
    type = 3;
  }
  virtual ~NonhierSphere();

  // private:
  glm::vec3 m_pos;
  double m_radius;
};

class NonhierBox : public Primitive
{
public:
  NonhierBox(const glm::vec3 &pos, double size)
      : m_pos(pos), m_size(size)
  {
    type = 4;
  }

  virtual ~NonhierBox();

  // private:
  glm::vec3 m_pos;
  double m_size;
};