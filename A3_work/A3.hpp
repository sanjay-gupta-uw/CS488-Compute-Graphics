// Termm-Fall 2020

#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"

#include "SceneNode.hpp"
#include "GeometryNode.hpp"
#include "JointNode.hpp"

#include <glm/glm.hpp>
#include <memory>

struct LightSource
{
	glm::vec3 position;
	glm::vec3 rgbIntensity;
};

class A3 : public CS488Window
{
public:
	A3(const std::string &luaSceneFile);
	virtual ~A3();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	//-- Virtual callback methods
	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

	//-- One time initialization methods:
	void processLuaSceneFile(const std::string &filename);
	void createShaderProgram();
	void enableVertexShaderInputSlots();
	void uploadVertexDataToVbos(const MeshConsolidator &meshConsolidator);
	void mapVboDataToVertexShaderInputLocations();
	void initViewMatrix();
	void initLightSources();

	void initPerspectiveMatrix();
	void uploadCommonSceneUniforms();
	void renderSceneGraph(const SceneNode &node);
	void renderArcCircle();
	void updateTransformations(float oldX, float newX, float oldY, float newY, bool drag);
	void updateRotationTransformations(float fOldX, float fNewX, float fOldY, float fNewY);

	bool zBuffer;
	bool backFaceCulling;
	bool frontFaceCulling;
	bool renderCircle;

	void processSceneNode(SceneNode &node, glm::mat4 parentTransform);
	void renderGeometryNode(GeometryNode &node, glm::mat4 parentTransform);

	bool mouse_input[3] = {false, false, false};
	double x_change[3] = {0, 0, 0}; // need additional vars for mid/right
	double y_change[3] = {0, 0, 0}; // need additional vars for mid/right
	double prev_x[3] = {-1, -1, -1};
	double prev_y[3] = {-1, -1, -1};

	glm::mat4 m_perpsective;
	glm::mat4 m_view;

	LightSource m_light;

	//-- GL resources for mesh geometry data:
	GLuint m_vao_meshData;
	GLuint m_vbo_vertexPositions;
	GLuint m_vbo_vertexNormals;
	GLint m_positionAttribLocation;
	GLint m_normalAttribLocation;
	ShaderProgram m_shader;

	//-- GL resources for trackball circle geometry:
	GLuint m_vbo_arcCircle;
	GLuint m_vao_arcCircle;
	GLint m_arc_positionAttribLocation;
	ShaderProgram m_shader_arcCircle;

	// BatchInfoMap is an associative container that maps a unique MeshId to a BatchInfo
	// object. Each BatchInfo object contains an index offset and the number of indices
	// required to render the mesh with identifier MeshId.
	BatchInfoMap m_batchInfoMap;

	std::string m_luaSceneFile;

	std::shared_ptr<SceneNode> m_rootNode;
	SceneNode *headNode = nullptr;
	std::vector<bool> selected;
	void initializeNodes(SceneNode &node);
	std::vector<SceneNode *> nodes;
	std::vector<glm::vec3> initial_positions;
	std::vector<float> initial_angles_x;
	std::vector<float> initial_angles_y;
	std::vector<glm::mat4> initial_transforms;
	std::vector<glm::mat4> initial_transforms_unscaled;

	void reset_position();
	void reset_orientation();
	void reset_joints();
	void reset_all();
	void update_stack();
	void undo();
	void redo();
	std::vector<std::vector<glm::vec2>> joint_angle_stack;
	int stack_index = -1;
	bool no_undo = true;
	bool no_redo = true;
};
