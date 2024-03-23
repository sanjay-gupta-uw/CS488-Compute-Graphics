// Termm-Fall 2020

#include "GeometryNode.hpp"

using namespace std;
//---------------------------------------------------------------------------------------
GeometryNode::GeometryNode(
	const std::string &meshId,
	const std::string &name)
	: SceneNode(name),
	  meshId(meshId)
{
	m_nodeType = NodeType::GeometryNode;
}
