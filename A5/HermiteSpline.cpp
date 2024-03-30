#include "HermiteSpline.hpp"
#include <iostream>
#include <cmath>

HermiteSpline::HermiteSpline()
{
}

HermiteSpline::~HermiteSpline()
{
}

void HermiteSpline::setUpSpline(float height, glm::vec3 bottom, double rotationRange)
{
    start.pos = bottom;
    start.tangent = glm::vec3(0, 0, 0);
    end.pos = bottom + glm::vec3(0, height, 0);
    end.tangent = glm::vec3(0, 0, 0);
    this->rotationRange = rotationRange;
}

glm::vec3 HermiteSpline::getPos(float t)
{
    float h_00 = 2 * t * t * t - 3 * t * t + 1;
    float h_10 = t * t * t - 2 * t * t + t;
    float h_01 = -2 * t * t * t + 3 * t * t;
    float h_11 = t * t * t - t * t;

    glm::vec3 pos = h_00 * start.pos +
                    h_10 * start.tangent +
                    h_01 * end.pos +
                    h_11 * end.tangent;

    return pos;
}

float HermiteSpline::getRotation(float t)
{
    // bottom (t=0) should be fully rotated, top should be unrotated
    // non linear rotation

    float angle = rotationRange * (-0.5f * (cos(glm::pi<float>() * t) + 1.0f));
    if (t == 0 || t == 1)
    {
        std::cout << "t: " << t << ", angle: " << angle << std::endl;
    }
    return angle;
}