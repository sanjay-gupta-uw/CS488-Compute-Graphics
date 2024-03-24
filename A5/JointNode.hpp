// Termm-Fall 2020

#pragma once

#include "SceneNode.hpp"

class JointNode : public SceneNode
{
public:
	JointNode(const std::string &name);
	virtual ~JointNode();

	void set_joint_x(double min, double init, double max);
	double init_x;
	double init_y;
	void set_joint_y(double min, double init, double max);
	void update_x(double val);
	void update_y(double val);
	glm::mat4 get_rot(glm::mat4 parent);
	struct JointRange
	{
		double min, init, max;
	};
	glm::mat4 mRotation;
	bool update = false;

	JointRange m_joint_x, m_joint_y;
};
