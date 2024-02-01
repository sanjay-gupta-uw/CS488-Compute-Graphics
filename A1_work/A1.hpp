// Termm--Fall 2020

#pragma once

#include <glm/glm.hpp>

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include <vector>

#include "maze.hpp"

class A1 : public CS488Window
{
public:
	A1();
	virtual ~A1();

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

	void uploadUniformsToShader();

private:
	void initGrid();
	void initFloor();
	void initCubes(bool redraw);
	void initAvatar();
	void initGame();
	void reset();
	void dig();
	bool valid_move(char direction, bool shift);
	bool valid_move_not_dug(char direction, bool shift);

	// Fields related to the shader and uniforms.
	ShaderProgram avatar_shader;
	ShaderProgram maze_shader;
	ShaderProgram grid_shader;
	GLint P_uni[3];	  // Uniform location for Projection matrix.
	GLint V_uni[3];	  // Uniform location for View matrix.
	GLint M_uni[3];	  // Uniform location for Model matrix.
	GLint col_uni[3]; // Uniform location for cube colour.
	GLint scale_uniform;
	GLint avatar;
	GLuint offsetAttrib[2];
	GLint posAttrib[3];

	// Fields related to grid geometry.
	GLuint m_grid_vao; // Vertex Array Object
	GLuint m_grid_vbo; // Vertex Buffer Object

	GLuint m_floor_vao;
	GLuint m_floor_vbo;

	GLuint m_maze_vao;
	GLuint m_maze_vbo;

	GLuint m_avatar_vao;
	GLuint m_avatar_vbo;

	// Matrices controlling the camera and projection.
	glm::mat4 proj;
	glm::mat4 view;

	glm::vec3 colour[3];
	int current_col;
	int num_cubes;
	Maze *maze;
	bool dug;
	float avatar_pos[2];
	float m_scale[3];
	float shape_rotation;
	glm::vec2 avatar_cell;
	float delta;

	bool draggingJustEnded = false;
	bool dragging = false;
	int counter;
	double dragEndTime;
	double x_points[20];
	bool persist = false;
};
