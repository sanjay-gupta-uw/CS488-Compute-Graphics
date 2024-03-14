// Termm--Fall 2020

#pragma once

#include "Material.hpp"

#include <glm/glm.hpp>

#include <list>
#include <string>
#include <iostream>

enum class NodeType
{
    SceneNode,
    GeometryNode,
    JointNode
};

class SceneNode
{
public:
    SceneNode(const std::string &name);

    SceneNode(const SceneNode &other);
    glm::vec3 scale_amount = glm::vec3(1.0f, 1.0f, 1.0f);

    virtual ~SceneNode();

    int totalSceneNodes() const;

    const glm::mat4 &get_transform() const;
    const glm::mat4 &get_inverse() const;

    void set_transform(const glm::mat4 &m);

    void add_child(SceneNode *child);

    void remove_child(SceneNode *child);

    //-- Transformations:
    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    glm::mat4 translation_matrix = glm::mat4(1.0f);
    glm::mat4 scale_matrix = glm::mat4(1.0f);
    void rotate(char axis, float angle);
    void scale(const glm::vec3 &amount);
    void translate(const glm::vec3 &amount);

    friend std::ostream &operator<<(std::ostream &os, const SceneNode &node);

    // Transformations
    glm::mat4 trans;
    glm::mat4 invtrans;

    std::list<SceneNode *> children;

    NodeType m_nodeType;
    std::string m_name;
    unsigned int m_nodeId;

    unsigned int index = -1;

private:
    // The number of SceneNode instances.
    static unsigned int nodeInstanceCount;
};
