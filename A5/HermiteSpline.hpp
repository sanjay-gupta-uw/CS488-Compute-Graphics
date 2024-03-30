#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

struct HermitePoint
{
    glm::vec3 pos;
    glm::vec3 tangent;
};

class HermiteSpline
{
public:
    HermiteSpline();
    ~HermiteSpline();
    void setUpSpline(float height, glm::vec3 bottom, double rotationRange);
    float getRotation(float t);
    glm::vec3 getPos(float t);

private:
    HermitePoint start, end;
    float rotationRange;
};