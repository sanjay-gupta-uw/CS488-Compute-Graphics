#include <vector>
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>
#include "sphere.hpp"
using namespace std;
using namespace glm;

Sphere::Sphere()
{
}

Sphere::~Sphere()
{
}

vector<vec3> Sphere::createIcosahedronVertices()
{
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f; // The golden ratio
    const float a = 1.0f;                              // The side length of the icosahedron
    const float b = 1.0f / phi;

    vector<vec3> vertices = {
        // 5 vertices around point (0, 1, 0)
        vec3(-a, b, 0),
        vec3(a, b, 0),
        vec3(-a, -b, 0),
        vec3(a, -b, 0),

        // 5 vertices around point (1, 0, 0)
        vec3(0, -a, b),
        vec3(0, a, b),
        vec3(0, -a, -b),
        vec3(0, a, -b),

        // 5 vertices around point (0, 0, 1)s
        vec3(b, 0, -a),
        vec3(b, 0, a),
        vec3(-b, 0, -a),
        vec3(-b, 0, a)};

    // Normalize the vertices so that they lie on the sphere's surface
    for (auto &vertex : vertices)
    {
        vertex = normalize(vertex);
    }

    return vertices;
}
