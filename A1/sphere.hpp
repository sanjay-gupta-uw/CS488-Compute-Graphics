#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <iostream>

struct Triangle
{
  int v1, v2, v3;
  Triangle(int v1, int v2, int v3) : v1(v1), v2(v2), v3(v3) {}
};
struct PairHash
{
  size_t operator()(const std::pair<int, int> &p) const
  {
    auto hash1 = std::hash<int>{}(p.first);
    auto hash2 = std::hash<int>{}(p.second);
    return hash1 ^ (hash2 << 1); // Left shift hash2 before XOR
  }
};

class Icosphere
{
public:
  std::vector<glm::vec3> vertices;
  int index = -1;
  std::unordered_map<std::pair<int, int>, int, PairHash> hash_table;

  // fix position of vertex to be on unit sphere, return index
  int addVertex(float x, float y, float z)
  {
    float length = glm::sqrt(x * x + y * y + z * z);
    vertices.push_back(glm::vec3(x / length, y / length, z / length));
    return ++index;
  }

  // return index of point in the middle of p1 and p2
  int getMiddlePoint(int v1, int v2)
  {
    std::pair<int, int> key = std::pair<int, int>((v1 <= v2) ? v1 : v2, (v1 <= v2) ? v2 : v1);
    glm::vec3 vert1 = vertices[v1];
    glm::vec3 vert2 = vertices[v2];

    float x = (vert1.x + vert2.x) / 2.0;
    float y = (vert1.y + vert2.y) / 2.0;
    float z = (vert1.z + vert2.z) / 2.0;

    if (hash_table.find(key) != hash_table.end())
    {
      int idx = hash_table[key];
      float length = glm::sqrt(x * x + y * y + z * z);
      glm::vec3 candidate = vertices[idx];
      if (abs(candidate.x - (x / length)) < 0.001 && abs(candidate.y - (y / length)) < 0.001 && abs(candidate.z - (z / length)) < 0.001)
        return idx;
    }

    int idx = addVertex(x, y, z);

    hash_table[key] = idx;
    return idx;
  }

public:
  void generateIcosphere(int recursionLevel)
  {
    // create 12 vertices of a icosahedron
    const float phi = (1.0 + glm::sqrt(5.0)) / 2.0;

    addVertex(0, -1, phi);
    addVertex(0, -1, -phi);
    addVertex(0, 1, phi);
    addVertex(0, 1, -phi);

    addVertex(-1, phi, 0);
    addVertex(-1, -phi, 0);
    addVertex(1, phi, 0);
    addVertex(1, -phi, 0);

    std::vector<Triangle> faces;

    faces.emplace_back(0, 11, 5);
    faces.emplace_back(0, 5, 1);
    faces.emplace_back(0, 1, 7);
    faces.emplace_back(0, 7, 10);
    faces.emplace_back(0, 10, 11);
    faces.emplace_back(1, 5, 9);
    faces.emplace_back(5, 11, 4);
    faces.emplace_back(11, 10, 2);
    faces.emplace_back(10, 7, 6);
    faces.emplace_back(7, 1, 8);
    faces.emplace_back(3, 9, 4);
    faces.emplace_back(3, 4, 2);
    faces.emplace_back(3, 2, 6);
    faces.emplace_back(3, 6, 8);
    faces.emplace_back(3, 8, 9);
    faces.emplace_back(4, 9, 5);
    faces.emplace_back(2, 4, 11);
    faces.emplace_back(6, 2, 10);
    faces.emplace_back(8, 6, 7);
    faces.emplace_back(9, 8, 1);

    for (int i = 0; i < recursionLevel; i++)
    {
      std::vector<Triangle> new_faces;
      for (auto triangle : faces)
      {
        // recursive step, split triangle into 4 smaller triangles
        int a = getMiddlePoint(triangle.v1, triangle.v2);
        int b = getMiddlePoint(triangle.v2, triangle.v3);
        int c = getMiddlePoint(triangle.v3, triangle.v1);

        new_faces.emplace_back(triangle.v1, a, c);
        new_faces.emplace_back(a, triangle.v2, b);
        new_faces.emplace_back(c, b, triangle.v3);
        new_faces.emplace_back(a, b, c);
      }
      faces = new_faces;
    }
  }
  void printVertices()
  {
    std::cout << vertices.size() << std::endl;
    // for (auto vert : vertices)
    // {
    //   std::cout << vert.x << " " << vert.y << " " << vert.z << std::endl;
    // }
  }
};

// Recursion Depth 0 (Initial Icosahedron): 12 vertices.
// Recursion Depth 1: 27.
// Recursion Depth 2: 87.
// Recursion Depth 3: 327.