// Termm-Fall 2020

#include "JointNode.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"

#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;
using namespace std;
//---------------------------------------------------------------------------------------
JointNode::JointNode(const std::string &name)
	: SceneNode(name)
{
	m_nodeType = NodeType::JointNode;
}

//---------------------------------------------------------------------------------------
JointNode::~JointNode()
{
}
//---------------------------------------------------------------------------------------
void JointNode::set_joint_x(double min, double init, double max)
{
	m_joint_x.min = min;
	m_joint_x.init = init;
	m_joint_x.max = max;
	init_x = init;
}

//---------------------------------------------------------------------------------------
void JointNode::set_joint_y(double min, double init, double max)
{
	m_joint_y.min = min;
	m_joint_y.init = init;
	m_joint_y.max = max;
	init_y = init;
}

void JointNode::update_x(double val)
{
	double temp = m_joint_x.init + val;
	if (temp <= m_joint_x.max && temp >= m_joint_x.min && temp != m_joint_x.init)
	{
		// std::cout << "new val: " << temp << std::endl;
		m_joint_x.init = temp;
		update = true;
	}
}

void JointNode::update_y(double val)
{
	double temp = m_joint_y.init + val;
	if (temp <= m_joint_y.max && temp >= m_joint_y.min && temp != m_joint_y.init)
	{
		// std::cout << "new val: " << temp << std::endl;
		m_joint_y.init = temp;
		update = true;
	}
}

mat4 JointNode::get_rot(mat4 parent)
{
	mRotation = glm::rotate(parent * trans, (radians)((float)m_joint_x.init), vec3(1.0f, 0.0f, 0.0f));
	mRotation = glm::rotate(mRotation, (radians)((float)m_joint_y.init), vec3(0.0f, 1.0f, 0.0f));
	return mRotation;
}
