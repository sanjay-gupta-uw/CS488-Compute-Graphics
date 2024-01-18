// Termm--Fall 2020

#pragma once

#include <vector>
#include <glm/glm.hpp>
class Sphere
{
public:
    Sphere();
    ~Sphere();
    std::vector<glm::vec3> createIcosahedronVertices();

private:
    unsigned int radius;
};
