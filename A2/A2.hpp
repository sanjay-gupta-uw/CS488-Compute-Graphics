// Termm--Fall 2020

#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include <glm/glm.hpp>

#include <vector>

// Set a global maximum number of vertices in order to pre-allocate VBO data
// in one shot, rather than reallocating each frame.
const GLsizei kMaxVertices = 1000;

// Convenience class for storing vertex data in CPU memory.
// Data should be copied over to GPU memory via VBO storage before rendering.
class VertexData
{
public:
	VertexData();

	std::vector<glm::vec2> positions;
	std::vector<glm::vec3> colours;
	GLuint index;
	GLsizei numVertices;
};

class CoordinateSystem3D
{
public:
	CoordinateSystem3D(){};

	glm::vec4 origin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 x = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec4 y = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	glm::vec4 z = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
};

class Cube
{
public:
	// apply rotation/translation to the coordinate system
	// apply scaling to the vertices
	CoordinateSystem3D cube_coordinateSystem;
	glm::vec4 cube_vertices[8] = {
		glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
		glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
		glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
		glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),

		glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),
		glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),
		glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
		glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),
	};
	void reset()
	{
		cube_coordinateSystem.origin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		cube_coordinateSystem.x = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		cube_coordinateSystem.y = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		cube_coordinateSystem.z = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
	}
};

class Camera
{
public:
	void updateCoordinateSystem();
	int computeOutcode(glm::vec4 &v)
	{
		int outcode = 0;
		// camera looks down negative z axis
		if (v.z > -near)
			outcode |= 1;

		return outcode;
	};

	Camera(){};
	Camera(glm::vec4 position, glm::vec4 lookAt, glm::vec4 up, float near, float far, float fov);

	glm::vec4 position;
	glm::vec4 lookAt;
	glm::vec4 up;
	float near;
	float far;
	float fov;

	float pitch = 375.3f;
	float yaw = 792.4f;
	float roll = 0.0f;

	CoordinateSystem3D camera_coordinateSystem;
};

class A2 : public CS488Window
{
public:
	A2();
	virtual ~A2();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

	void createShaderProgram();
	void enableVertexAttribIndices();
	void generateVertexBuffers();
	void mapVboDataToVertexAttributeLocation();
	void uploadVertexDataToVbos();

	void initLineData();

	void setLineColour(const glm::vec3 &colour);

	void drawModel();
	void drawViewportOutline();
	std::array<glm::vec2, 2> getNDC(glm::vec4 &P0, glm::vec4 &P1);

	void drawLine(
		const glm::vec2 &V0,
		const glm::vec2 &V1);

	void generateScaleMatrix();
	void generateRotationMatrix(bool isModel);
	void generateTranslationMatrix(bool isModel);
	void generateViewMatrix();
	void generatePerspectiveMatrix();

	bool clipNearPlane(glm::vec4 &v0,
					   glm::vec4 &v1);
	bool clipFarPlane(glm::vec4 &v0,
					  glm::vec4 &v1);

	bool
	clipCube(glm::vec4 &P0,
			 glm::vec4 &P1);

	void updateTransformations();

	void reset();

	ShaderProgram m_shader;

	glm::mat4 F;
	glm::mat4 P;
	glm::mat4 V;
	glm::mat4 M;
	glm::mat4 M_model;
	glm::mat4 M_viewport;

	glm::mat4 m_translate[2];
	glm::mat4 m_rotate[2][3];
	glm::mat4 Scale;
	glm::mat4 m_scale;
	glm::mat4 g_scale;

	float scale[3];
	float rotate[2][3];
	float translate[2][3];

	GLuint m_vao;			// Vertex Array Object
	GLuint m_vbo_positions; // Vertex Buffer Object
	GLuint m_vbo_colours;	// Vertex Buffer Object

	VertexData m_vertexData;

	glm::vec3 m_currentLineColour;

	CoordinateSystem3D world_coordinateSystem;
	Cube m_cube;
	Camera m_camera;

	int transform_mode = 3;

	bool translate_model = false;
	bool mouse_input[3] = {false, false, false};
	double x_change[3] = {0, 0, 0}; // need additional vars for mid/right
	double prev_x[3] = {-1, -1, -1};

	int viewportHeight;
	int viewportWidth;

	int window_width;
	int window_height;
	bool start_draw = false;
	bool end_draw = false;
	bool drawing = false;
	glm::vec2 corners[2];
	glm::vec2 corners_new[2];

	void updateViewport();
	void initializeViewport();
	void transformToViewport(glm::vec4 &P);
};