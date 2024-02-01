// Termm--Fall 2020

#include "A1.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>

#include <sys/types.h>
#include <unistd.h>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "sphere.hpp"

using namespace glm;
using namespace std;

static const size_t DIM = 16;
static const int AVATAR = 0;
static const int MAZE = 1;
static const int GRID = 2;
// change this
int prev_x = -1;

vec3 vertices[8] = {
	vec3(-0.5, 0, -0.5),
	vec3(-0.5, 0, 0.5),
	vec3(-0.5, 1, -0.5),
	vec3(-0.5, 1, 0.5),
	vec3(0.5, 0, -0.5),
	vec3(0.5, 0, 0.5),
	vec3(0.5, 1, -0.5),
	vec3(0.5, 1, 0.5)};

// Define each face of the cube (2 triangles per face)
unsigned int faces[12][3] = {
	// middle index is shared between triangles
	{0, 1, 2},
	{2, 1, 3}, // Front face
	{4, 6, 5},
	{5, 6, 7}, // Back face
	{0, 2, 4},
	{4, 2, 6}, // Left face
	{1, 5, 3},
	{3, 5, 7}, // Right face
	{2, 3, 6},
	{6, 3, 7}, // Top face
	{0, 4, 1},
	{1, 4, 5} // Bottom face
};

vec3 cube[36] = {
	// Front face
	vertices[faces[0][0]], vertices[faces[0][1]], vertices[faces[0][2]], // Triangle 1
	vertices[faces[1][0]], vertices[faces[1][1]], vertices[faces[1][2]], // Triangle 2

	// Back face
	vertices[faces[2][0]], vertices[faces[2][1]], vertices[faces[2][2]], // Triangle 1
	vertices[faces[3][0]], vertices[faces[3][1]], vertices[faces[3][2]], // Triangle 2

	// Left face
	vertices[faces[4][0]], vertices[faces[4][1]], vertices[faces[4][2]], // Triangle 1
	vertices[faces[5][0]], vertices[faces[5][1]], vertices[faces[5][2]], // Triangle 2

	// Right face
	vertices[faces[6][0]], vertices[faces[6][1]], vertices[faces[6][2]], // Triangle 1
	vertices[faces[7][0]], vertices[faces[7][1]], vertices[faces[7][2]], // Triangle 2

	// Top face
	vertices[faces[8][0]], vertices[faces[8][1]], vertices[faces[8][2]], // Triangle 1
	vertices[faces[9][0]], vertices[faces[9][1]], vertices[faces[9][2]], // Triangle 2

	// Bottom face
	vertices[faces[10][0]], vertices[faces[10][1]], vertices[faces[10][2]], // Triangle 1
	vertices[faces[11][0]], vertices[faces[11][1]], vertices[faces[11][2]], // Triangle 2
};
vector<glm::vec3> avatar_vertices;
//----------------------------------------------------------------------------------------
// Constructor
A1::A1()
	: current_col(0)
{
	colour[AVATAR] = glm::vec3(0.0f, 0.0f, 1.0f);
	colour[MAZE] = glm::vec3(1.0f, 1.0f, 1.0f);
	colour[GRID] = glm::vec3(0.0f, 1.0f, 0.0f);
	num_cubes = 0;
	dug = false;
}

//----------------------------------------------------------------------------------------
// Destructor
A1::~A1()
{
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A1::init()
{
	// Initialize random number generator
	int rseed = getpid();
	srandom(rseed);
	// Print random number seed in case we want to rerun with
	// same random numbers
	cout << "Random number seed = " << rseed << endl;

	// Set the background colour.
	glClearColor(0.3, 0.5, 0.7, 1.0);

	// Build the shaders
	grid_shader.generateProgramObject();
	grid_shader.attachVertexShader(
		getAssetFilePath("gridVertexShader.vs").c_str());
	grid_shader.attachFragmentShader(
		getAssetFilePath("FragmentShader.fs").c_str());
	grid_shader.link();

	maze_shader.generateProgramObject();
	maze_shader.attachVertexShader(
		getAssetFilePath("mazeVertexShader.vs").c_str());
	maze_shader.attachFragmentShader(
		getAssetFilePath("FragmentShader.fs").c_str());
	maze_shader.link();

	avatar_shader.generateProgramObject();
	avatar_shader.attachVertexShader(
		getAssetFilePath("avatarVertexShader.vs").c_str());
	avatar_shader.attachFragmentShader(
		getAssetFilePath("FragmentShader.fs").c_str());
	avatar_shader.link();

	// Set up the uniforms
	P_uni[GRID] = grid_shader.getUniformLocation("P");
	V_uni[GRID] = grid_shader.getUniformLocation("V");
	M_uni[GRID] = grid_shader.getUniformLocation("M");
	col_uni[GRID] = grid_shader.getUniformLocation("colour");
	posAttrib[GRID] = grid_shader.getAttribLocation("position");

	P_uni[MAZE] = maze_shader.getUniformLocation("P");
	V_uni[MAZE] = maze_shader.getUniformLocation("V");
	M_uni[MAZE] = maze_shader.getUniformLocation("M");
	col_uni[MAZE] = maze_shader.getUniformLocation("colour");
	scale_uniform = maze_shader.getUniformLocation("scale");
	offsetAttrib[MAZE] = maze_shader.getAttribLocation("offset");
	posAttrib[MAZE] = maze_shader.getAttribLocation("position");

	P_uni[AVATAR] = avatar_shader.getUniformLocation("P");
	V_uni[AVATAR] = avatar_shader.getUniformLocation("V");
	M_uni[AVATAR] = avatar_shader.getUniformLocation("M");
	col_uni[AVATAR] = avatar_shader.getUniformLocation("colour");
	offsetAttrib[AVATAR] = avatar_shader.getUniformLocation("offset");
	posAttrib[AVATAR] = avatar_shader.getAttribLocation("position");

	maze = new Maze(DIM);

	Icosphere sphere;
	sphere.generateIcosphere(2);
	avatar_vertices = sphere.faces_vertices;

	initGame();

	// Set up initial view and projection matrices (need to do this here,
	// since it depends on the GLFW window being set up correctly).
	view = glm::lookAt(
		glm::vec3(0.0f, 2. * float(DIM) * 2.0 * M_SQRT1_2, float(DIM) * 2.0 * M_SQRT1_2),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	proj = glm::perspective(
		glm::radians(30.0f),
		float(m_framebufferWidth) / float(m_framebufferHeight),
		1.0f, 1000.0f);
}

void A1::initGrid()
{

	size_t sz = 3 * 2 * 2 * (DIM + 3);

	float *verts = new float[sz];
	size_t ct = 0;
	for (int idx = 0; idx < DIM + 3; ++idx)
	{
		verts[ct] = -1;
		verts[ct + 1] = 0;
		verts[ct + 2] = idx - 1;
		verts[ct + 3] = DIM + 1;
		verts[ct + 4] = 0;
		verts[ct + 5] = idx - 1;
		ct += 6;

		verts[ct] = idx - 1;
		verts[ct + 1] = 0;
		verts[ct + 2] = -1;
		verts[ct + 3] = idx - 1;
		verts[ct + 4] = 0;
		verts[ct + 5] = DIM + 1;
		ct += 6;
	}

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_grid_vao);
	glBindVertexArray(m_grid_vao);

	// Create the cube vertex buffer
	glGenBuffers(1, &m_grid_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_grid_vbo);
	glBufferData(GL_ARRAY_BUFFER, ct * sizeof(float),
				 verts, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	glEnableVertexAttribArray(posAttrib[GRID]);
	glVertexAttribPointer(posAttrib[GRID], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Reset state to prevent rogue code from messing with *my*
	// stuff!
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	// OpenGL has the buffer now, there's no need for us to keep a copy.
	delete[] verts;
}
void A1::initFloor()
{

	size_t sz = 4 * 3;

	float *verts = new float[sz];
	size_t ct = 0;

	verts[ct++] = 0;
	verts[ct++] = 0;
	verts[ct++] = 0;

	verts[ct++] = 0;
	verts[ct++] = 0;
	verts[ct++] = DIM;

	verts[ct++] = DIM;
	verts[ct++] = 0;
	verts[ct++] = DIM;

	verts[ct++] = DIM;
	verts[ct++] = 0;
	verts[ct++] = 0;

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_floor_vao);
	glBindVertexArray(m_floor_vao);

	// Create the cube vertex buffer
	glGenBuffers(1, &m_floor_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_floor_vbo);
	glBufferData(GL_ARRAY_BUFFER, ct * sizeof(float),
				 verts, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	glEnableVertexAttribArray(posAttrib[GRID]);
	glVertexAttribPointer(posAttrib[GRID], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Reset state to prevent rogue code from messing with *my*
	// stuff!
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	// OpenGL has the buffer now, there's no need for us to keep a copy.
	delete[] verts;
}

void A1::initCubes(bool redraw)
{
	num_cubes = 0;
	for (int x = 0; x < DIM; ++x)
	{
		for (int y = 0; y < DIM; ++y)
		{
			num_cubes += maze->getValue(x, y);
		}
	}
	// cout << "num cubes: " << num_cubes << endl;
	glm::vec2 *translations = new glm::vec2[num_cubes];

	size_t ct = 0;
	vec2 cellPosition(0, 0);
	for (int x = 0; x < DIM; ++x)
	{
		for (int z = 0; z < DIM; ++z)
		{
			if (maze->getValue(x, z) == 1)
			{
				cellPosition.x = x + 0.5;
				cellPosition.y = z + 0.5;
				translations[ct++] = cellPosition;
			}
			else if (!redraw && x == 0)
			{
				avatar_cell.x = x;
				avatar_cell.y = z;
				avatar_pos[1] += z;
			}
		}
	}

	GLuint instanceVBO;
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, num_cubes * sizeof(vec2), &translations[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	delete[] translations;

	// pass cube data
	glGenVertexArrays(1, &m_maze_vao);
	glBindVertexArray(m_maze_vao);

	glGenBuffers(1, &m_maze_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_maze_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	glEnableVertexAttribArray(posAttrib[MAZE]);
	// nullptr represents offset where attribute is stored (null means none), 3 represents the x,y,z vertex inputs, 0 since no padding between vertices
	glVertexAttribPointer(posAttrib[MAZE], 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr); // this will store the information in vao -> reference to vbo and how to retrieve attributes

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glEnableVertexAttribArray(offsetAttrib[MAZE]);
	glVertexAttribPointer(offsetAttrib[MAZE], 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
	glVertexAttribDivisor(offsetAttrib[MAZE], 1); // instanced vertex attribute.

	// Reset state to prevent rogue code from messing with *my* stuff!
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void A1::initAvatar()
{
	avatar_pos[0] = -0.5f;
	avatar_pos[1] = -0.5f;
	vec3 vertices[avatar_vertices.size()];
	for (int i = 0; i < avatar_vertices.size(); i++)
	{
		vertices[i] = avatar_vertices[i];
	}
	glGenVertexArrays(1, &m_avatar_vao);
	glBindVertexArray(m_avatar_vao);

	// Create the cube vertex buffer
	glGenBuffers(1, &m_avatar_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_avatar_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
				 vertices, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	glEnableVertexAttribArray(offsetAttrib[AVATAR]);
	glVertexAttribPointer(offsetAttrib[AVATAR], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	CHECK_GL_ERRORS;
}
void A1::initGame()
{
	persist = false;
	m_scale[AVATAR] = 1.0f;
	m_scale[GRID] = 1.0f;
	m_scale[MAZE] = 1.0f;
	shape_rotation = 0.0f;
	counter = 0;
	draggingJustEnded = false;
	dragEndTime = 0.0;

	initGrid();
	initFloor();
	if (dug)
	{
		initCubes(false);
	}
	initAvatar();
}

void A1::dig()
{
	maze->digMaze();
	dug = true;
	avatar_pos[0] = 0.5f;
	avatar_pos[1] = 0.5f;
	initCubes(false);
}

void A1::reset()
{
	maze->reset();
	dug = false;
	initGame();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */

void A1::uploadUniformsToShader()
{
	// Initialize the transformation matrix to identity matrix
	mat4 W = mat4(1.0f);
	vec3 grid_center = vec3(-1 + float(DIM + 2) / 2, 0, -1 + float(DIM + 2) / 2);

	W = glm::scale(W, vec3(m_scale[GRID], m_scale[GRID], m_scale[GRID])) * glm::rotate(W, shape_rotation, vec3(0, 1, 0)) * glm::translate(W, -grid_center);
	grid_shader.enable();
	glUniformMatrix4fv(P_uni[GRID], 1, GL_FALSE, value_ptr(proj));
	glUniformMatrix4fv(V_uni[GRID], 1, GL_FALSE, value_ptr(view));
	glUniformMatrix4fv(M_uni[GRID], 1, GL_FALSE, value_ptr(W));
	grid_shader.disable();

	maze_shader.enable();
	glUniform3f(col_uni[MAZE], colour[MAZE].r, colour[MAZE].g, colour[MAZE].b);
	glUniform1f(scale_uniform, m_scale[MAZE]);
	glUniformMatrix4fv(P_uni[MAZE], 1, GL_FALSE, value_ptr(proj));
	glUniformMatrix4fv(V_uni[MAZE], 1, GL_FALSE, value_ptr(view));
	glUniformMatrix4fv(M_uni[MAZE], 1, GL_FALSE, value_ptr(W));
	maze_shader.disable();

	avatar_shader.enable();
	glEnable(GL_DEPTH_TEST);
	glUniform3f(col_uni[AVATAR], colour[AVATAR].r, colour[AVATAR].g, colour[AVATAR].b);
	glUniform2fv(offsetAttrib[AVATAR], 1, avatar_pos);
	glUniformMatrix4fv(P_uni[AVATAR], 1, GL_FALSE, value_ptr(proj));
	glUniformMatrix4fv(V_uni[AVATAR], 1, GL_FALSE, value_ptr(view));
	glUniformMatrix4fv(M_uni[AVATAR], 1, GL_FALSE, value_ptr(W));
	avatar_shader.disable();

	CHECK_GL_ERRORS;
}
void A1::appLogic()
{
	if (persist)
	{
		shape_rotation += ((x_points[0] - x_points[counter - 1]) > 0 ? -1 : 1) * 0.01;
		// persist = false;
	}
	// Place per frame, application logic here ...
	uploadUniformsToShader();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A1::guiLogic()
{
	// We already know there's only going to be one window, so for
	// simplicity we'll store button states in static local variables.
	// If there was ever a possibility of having multiple instances of
	// A1 running simultaneously, this would break; you'd want to make
	// this into instance fields of A1.
	static bool showTestWindow(false);
	static bool showDebugWindow(true);

	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Debug Window", &showDebugWindow, ImVec2(100, 100), opacity, windowFlags);
	if (ImGui::Button("Quit Application"))
	{
		glfwSetWindowShouldClose(m_window, GL_TRUE);
	}
	if (ImGui::Button("Dig"))
	{
		dig();
	}
	if (ImGui::Button("Reset"))
	{
		reset();
		// should also reset rotation
	}
	// radio buttons for avatar/maze.grid
	ImGui::SliderFloat("Red Channel", &colour[current_col].r, 0.0f, 1.0f);
	ImGui::SliderFloat("Green Channel", &colour[current_col].g, 0.0f, 1.0f);
	ImGui::SliderFloat("Blue Channel", &colour[current_col].b, 0.0f, 1.0f);

	// Eventually you'll create multiple colour widgets with
	// radio buttons.  If you use PushID/PopID to give them all
	// unique IDs, then ImGui will be able to keep them separate.
	// This is unnecessary with a single colour selector and
	// radio button, but I'm leaving it in as an example.

	// Prefixing a widget name with "##" keeps it from being
	// displayed.

	ImGui::ColorEdit3("##Colour", glm::value_ptr(colour[current_col]));
	for (int i = 0; i < 3; ++i)
	{
		if (i == 0)
		{
			ImGui::Text("Avatar");
		}
		else if (i == 1)
		{
			ImGui::Text("Maze");
		}
		else if (i == 2)
		{
			ImGui::Text("Floor");
		}
		ImGui::PushID(i);
		ImGui::SameLine();
		if (ImGui::RadioButton("##Maze", &current_col, i))
		{
			// Select this colour.
		}
		ImGui::PopID();
	}

	/*
		// For convenience, you can uncomment this to show ImGui's massive
		// demonstration window right in your application.  Very handy for
		// browsing around to get the widget you want.  Then look in
		// shared/imgui/imgui_demo.cpp to see how it's done.
		if( ImGui::Button( "Test Window" ) ) {
		  showTestWindow = !showTestWindow;
		}
	*/

	ImGui::Text("Framerate: %.1f FPS", ImGui::GetIO().Framerate);

	ImGui::End();

	if (showTestWindow)
	{
		ImGui::ShowTestWindow(&showTestWindow);
	}
}

bool A1::valid_move_not_dug(char direction, bool shift)
{
	int x = avatar_pos[0] - 0.5f, y = avatar_pos[1] - 0.5f;
	if (direction == 'u')
	{
		if (y - 1 >= -1)
		{
			return true;
		}
	}
	else if (direction == 'd')
	{
		if (y + 1 < DIM + 1)
		{
			return true;
		}
	}
	else if (direction == 'l')
	{
		if (x - 1 >= -1)
		{
			return true;
		}
	}
	else if (direction == 'r')
	{
		if (x + 1 < DIM + 1)
		{
			return true;
		}
	}
	return false;
}
bool A1::valid_move(char direction, bool shift)
{
	if (!dug)
	{
		return valid_move_not_dug(direction, shift);
	}
	int x = avatar_cell.x;
	int y = avatar_cell.y;
	bool valid = false;
	bool redraw = false;
	// cout << "Avatar INITIAL: x: " << x << " y: " << y << endl;
	int ret;
	if (direction == 'u')
	{
		ret = maze->getValue(x, y - 1);
		if (y - 1 >= 0 && (shift || ret == 0))
		{
			avatar_cell.y -= 1;
			valid = true;
			if (ret == 1)
			{
				maze->setValue(x, y - 1, 0);
				redraw = true;
			}
		}
	}
	else if (direction == 'd')
	{
		ret = maze->getValue(x, y + 1);
		if (y + 1 < DIM && (shift || ret == 0))
		{
			avatar_cell.y += 1;
			valid = true;
			if (ret == 1)
			{
				maze->setValue(x, y + 1, 0);
				redraw = true;
			}
		}
	}
	else if (direction == 'l')
	{
		ret = maze->getValue(x - 1, y);
		if (x - 1 >= 0 && (shift || ret == 0))
		{
			avatar_cell.x -= 1;
			valid = true;
			if (ret == 1)
			{
				maze->setValue(x - 1, y, 0);
				redraw = true;
			}
		}
	}
	else if (direction == 'r')
	{
		ret = maze->getValue(x + 1, y);
		if (x + 1 < DIM && (shift || ret == 0))
		{
			avatar_cell.x += 1;
			valid = true;
			if (ret == 1)
			{
				maze->setValue(x + 1, y, 0);
				redraw = true;
			}
		}
	}
	// cout << "valid: " << valid << endl;
	if (valid)
	{
		if (redraw)
		{
			initCubes(true);
		}

		return true;
	}
	return false;
}
//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A1::draw()
{
	grid_shader.enable();
	glUniform3f(col_uni[GRID], 0.0f, 0.0f, 0.0f);
	glBindVertexArray(m_grid_vao);
	glDrawArrays(GL_LINES, 0, 2 * 2 * (DIM + 3));
	grid_shader.disable();

	grid_shader.enable();
	glUniform3f(col_uni[GRID], colour[GRID].r, colour[GRID].g, colour[GRID].b);
	glBindVertexArray(m_floor_vao);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	grid_shader.disable();

	if (dug)
	{
		maze_shader.enable();
		glBindVertexArray(m_maze_vao);
		glDrawArraysInstanced(GL_LINES, 0, 36, num_cubes);
		maze_shader.disable();
	}

	avatar_shader.enable();
	glBindVertexArray(m_avatar_vao);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLES, 0, avatar_vertices.size());
	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	avatar_shader.disable();

	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A1::cleanup()
{
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A1::cursorEnterWindowEvent(
	int entered)
{
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
bool A1::mouseMoveEvent(double xPos, double yPos)
{
	bool eventHandled = false;
	if (!ImGui::IsMouseHoveringAnyWindow())
	{
		if (draggingJustEnded)
		{
			double currentTime = glfwGetTime();
			if ((counter < 20) && (currentTime - dragEndTime < 0.01))
			{
				// cout << "xPos: " << xPos << endl;
				x_points[counter++] = xPos;
				return true;
			}

			if (abs(x_points[0] - x_points[counter - 1]) > 10.0)
			{
				// cout << x_points[0] << " " << x_points[counter - 1] << endl;
				persist = true;
			}
		}
		if (ImGui::IsMouseDragging())
		{
			draggingJustEnded = false;
			dragging = true;
			if (prev_x != -1)
			{
				delta = xPos - prev_x;
				shape_rotation += delta * 0.01;
			}
			prev_x = xPos;
		}
		if (!ImGui::IsMouseDown(0) && prev_x != -1 && dragging)
		{
			dragging = false;
			prev_x = -1;
			// cout << "Mouse released - shape_rotation: " << shape_rotation << endl;
			counter = 0;
			x_points[counter++] = xPos;
			dragEndTime = glfwGetTime();
			// cout << "Start timer" << endl;
			draggingJustEnded = true;
		}
		eventHandled = true;
	}

	return eventHandled;
}

// Event handler for mouse release
bool A1::mouseButtonInputEvent(int button, int actions, int mods)
{
	draggingJustEnded = false;
	dragEndTime = 0.0;
	counter = 0;
	persist = false;

	return true;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A1::mouseScrollEvent(double xOffSet, double yOffSet)
{
	bool eventHandled(false);

	// Zoom in or out.
	if (yOffSet < 0)
	{
		m_scale[GRID] -= 0.1;
		eventHandled = true;
	}
	else if (yOffSet > 0)
	{
		m_scale[GRID] += 0.1;
		eventHandled = true;
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A1::windowResizeEvent(int width, int height)
{
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A1::keyInputEvent(int key, int action, int mods)
{
	bool eventHandled(false);

	// Fill in with event handling code...
	if (action == GLFW_PRESS)
	{
		// Respond to some key events.
		if (key == GLFW_KEY_Q)
		{
			// cout << "Q key pressed" << endl;
			glfwSetWindowShouldClose(m_window, GL_TRUE);
			eventHandled = true;
		}
		if (key == GLFW_KEY_D)
		{
			// cout << "D key pressed" << endl;
			dig();
			eventHandled = true;
		}
		if (key == GLFW_KEY_R)
		{
			// cout << "D key pressed" << endl;
			reset();
			eventHandled = true;
		}
		// add 0 check(lower/upper bound)
		if (key == GLFW_KEY_SPACE)
		{
			// cout << "SPACE key pressed" << endl;
			m_scale[MAZE] += 1; // UPDATE TO ONLY DO UNIT INCREASE
			eventHandled = true;
		}
		if (key == GLFW_KEY_BACKSPACE)
		{
			// cout << "BACKSPACE key pressed" << endl;
			if (m_scale[MAZE] > -1)
			{
				m_scale[MAZE] -= 1;
			}
			eventHandled = true;
		}
		if ((key == GLFW_KEY_UP) || (key == GLFW_KEY_DOWN) || (key == GLFW_KEY_LEFT) || (key == GLFW_KEY_RIGHT))
		{
			bool shift = (mods & GLFW_MOD_SHIFT) != 0;
			if (key == GLFW_KEY_UP)
			{
				// cout << "UP Arrow pressed" << endl;
				if (valid_move('u', shift))
					avatar_pos[1] -= 1.0f; // check why its switching
				eventHandled = true;
			}
			if (key == GLFW_KEY_DOWN)
			{
				// cout << "Down Arrow pressed" << endl;
				if (valid_move('d', shift))
					avatar_pos[1] += 1.0f;
				eventHandled = true;
			}
			if (key == GLFW_KEY_LEFT)
			{
				// cout << "Left Arrow pressed" << endl;
				if (valid_move('l', shift))
					avatar_pos[0] -= 1.0f;
				eventHandled = true;
			}
			if (key == GLFW_KEY_RIGHT)
			{
				// cout << "Right Arrow pressed" << endl;
				if (valid_move('r', shift))
					avatar_pos[0] += 1.0f;
				eventHandled = true;
			}
		}
		return eventHandled;
	}
}
