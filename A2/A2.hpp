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
	CoordinateSystem3D(glm::vec3 origin, glm::vec3 x, glm::vec3 y, glm::vec3 z);

	glm::vec3 origin;
	glm::vec3 x;
	glm::vec3 y;
	glm::vec3 z;
};

class Cube
{
public:
	// apply rotation/translation to the coordinate system
	// apply scaling to the vertices
	CoordinateSystem3D cube_coordinateSystem = CoordinateSystem3D(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::vec3 cube_vertices[8] = {
		glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3(-1.0f, -1.0f, 1.0f),
		glm::vec3(1.0f, -1.0f, 1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),

		glm::vec3(-1.0f, 1.0f, -1.0f),
		glm::vec3(-1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, -1.0f),
	};
	void reset()
	{
		cube_coordinateSystem = CoordinateSystem3D(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
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
		if (v.z > near)
			outcode |= 1;

		return outcode;
	};

	Camera(){};
	Camera(glm::vec3 position, glm::vec3 lookAt, glm::vec3 up, float near, float far, float fov);

	glm::vec3 position;
	glm::vec3 lookAt;
	glm::vec3 up;
	float near;
	float far;
	float fov;

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

	void drawLine(
		const glm::vec3 &v0,
		const glm::vec3 &v1,
		bool viewport);

	void generateScaleMatrix();
	void generateRotationMatrix();
	void generateViewMatrix();
	void generatePerspectiveMatrix();

	bool clipNearPlane(glm::vec4 &v0,
					   glm::vec4 &v1);

	void reset();

	ShaderProgram m_shader;

	glm::mat4 F;
	glm::mat4 P;
	glm::mat4 V;
	glm::mat4 M;

	glm::mat4 m_translate;
	glm::mat4 m_rotate[3];
	glm::mat4 m_scale;

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

	int transform_mode = -1;

	bool translate_model = false;
	bool mouse_input[3] = {false, false, false};
	bool updated[3] = {false, false, false};
	double x_change[3] = {0, 0, 0}; // need additional vars for mid/right
	double prev_x[3] = {-1, -1, -1};

	int viewportHeight;
	int viewportWidth;

	int window_width;
	int window_height;

	void drawViewportOutline();
};
