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
	colour[0] = 0.0f;
	colour[1] = 0.0f;
	colour[2] = 0.0f;
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
	Icosphere sphere;
	sphere.generateIcosphere(3);
	avatar_vertices = sphere.faces_vertices;

	// Initialize random number generator
	int rseed = getpid();
	srandom(rseed);
	// Print random number seed in case we want to rerun with
	// same random numbers
	cout << "Random number seed = " << rseed << endl;

	// Set the background colour.
	glClearColor(0.3, 0.5, 0.7, 1.0);

	// Build the shader
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		getAssetFilePath("VertexShader.vs").c_str());
	m_shader.attachFragmentShader(
		getAssetFilePath("FragmentShader.fs").c_str());
	m_shader.link();

	// Set up the uniforms
	P_uni = m_shader.getUniformLocation("P");
	V_uni = m_shader.getUniformLocation("V");
	M_uni = m_shader.getUniformLocation("M");
	col_uni = m_shader.getUniformLocation("colour");
	scale_uniform = m_shader.getUniformLocation("scale");
	offsetAttrib = m_shader.getAttribLocation("offset");
	posAttrib = m_shader.getAttribLocation("position");

	m_scale = 1.0;

	maze = new Maze(DIM);

	initGrid();

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
	// size_t sz = 3 * 2 * 2 * (DIM + 3);
	for (int x = 0; x < DIM; ++x)
	{
		for (int y = 0; y < DIM; ++y)
		{
			num_cubes += dug ? maze->getValue(x, y) : 1;
		}
	}
	cout << "num cubes: " << num_cubes << endl;
	glm::vec2 *translations = new glm::vec2[num_cubes];

	size_t ct = 0;
	vec2 cellPosition(0, 0);
	for (int x = 0; x < DIM; ++x)
	{
		for (int z = 0; z < DIM; ++z)
		{
			if (!dug || maze->getValue(x, z) == 1)
			{
				cellPosition.x = x;
				cellPosition.y = z;
				translations[ct++] = cellPosition;
			}
		}
	}

	// offsets for each cube
	GLuint instanceVBO;
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, num_cubes * sizeof(vec2), &translations[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// pass cube data
	glGenVertexArrays(1, &m_grid_vao);
	glBindVertexArray(m_grid_vao);

	glGenBuffers(1, &m_grid_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_grid_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	glEnableVertexAttribArray(posAttrib);
	// nullptr represents offset where attribute is stored (null means none), 3 represents the x,y,z vertex inputs, 0 since no padding between vertices
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr); // this will store the information in vao -> reference to vbo and how to retrieve attributes

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glEnableVertexAttribArray(offsetAttrib);
	glVertexAttribPointer(offsetAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
	glVertexAttribDivisor(offsetAttrib, 1); // instanced vertex attribute.

	// Reset state to prevent rogue code from messing with *my* stuff!
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// OpenGL has the buffer now, there's no need for us to keep a copy.
	// delete[] verts;
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
	glBufferData(GL_ARRAY_BUFFER, avatar_vertices.size() * sizeof(vec3),
				 vertices, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Reset state to prevent rogue code from messing with *my*
	// stuff!
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	CHECK_GL_ERRORS;
}

void A1::dig()
{
	maze->digMaze();
	dug = true;
	maze->printMaze();
	initGrid();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A1::appLogic()
{
	// Place per frame, application logic here ...
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

	// Eventually you'll create multiple colour widgets with
	// radio buttons.  If you use PushID/PopID to give them all
	// unique IDs, then ImGui will be able to keep them separate.
	// This is unnecessary with a single colour selector and
	// radio button, but I'm leaving it in as an example.

	// Prefixing a widget name with "##" keeps it from being
	// displayed.

	ImGui::PushID(0);
	ImGui::ColorEdit3("##Colour", colour);
	ImGui::SameLine();
	if (ImGui::RadioButton("##Col", &current_col, 0))
	{
		// Select this colour.
	}
	ImGui::PopID();

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

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A1::draw()
{
	// Create a global transformation for the model (centre it).
	mat4 W;
	W = glm::translate(W, vec3(-float(DIM) / 2.0f, 0, -float(DIM) / 2.0f));

	m_shader.enable();
	glEnable(GL_DEPTH_TEST);

	glUniformMatrix4fv(P_uni, 1, GL_FALSE, value_ptr(proj));
	glUniformMatrix4fv(V_uni, 1, GL_FALSE, value_ptr(view));
	glUniformMatrix4fv(M_uni, 1, GL_FALSE, value_ptr(W));

	// Just draw the grid for now.

	// glBindVertexArray(m_grid_vao);
	// glDrawArraysInstanced(GL_LINES, 0, 36, num_cubes);

	glBindVertexArray(m_avatar_vao);
	glUniform3f(col_uni, 1, 1, 1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLES, 0, avatar_vertices.size());
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// glUniform1f(scale_uniform, m_scale);

	// Draw the cubes
	// Highlight the active square.clear
	m_shader.disable();

	// Restore defaults
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
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
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool A1::mouseMoveEvent(double xPos, double yPos)
{
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow())
	{
		// Put some code here to handle rotations.  Probably need to
		// check whether we're *dragging*, not just moving the mouse.
		// Probably need some instance variables to track the current
		// rotation amount, and maybe the previous X position (so
		// that you can rotate relative to the *change* in X.
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A1::mouseButtonInputEvent(int button, int actions, int mods)
{
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow())
	{
		// The user clicked in the window.  If it's the left
		// mouse button, initiate a rotation.
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A1::mouseScrollEvent(double xOffSet, double yOffSet)
{
	bool eventHandled(false);

	// Zoom in or out.

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
			cout << "Q key pressed" << endl;
			glfwSetWindowShouldClose(m_window, GL_TRUE);
			eventHandled = true;
		}
		if (key == GLFW_KEY_D)
		{
			cout << "D key pressed" << endl;
			dig();
			eventHandled = true;
		}
		// add 0 check(lower/upper bound)
		if (key == GLFW_KEY_SPACE)
		{
			cout << "SPACE key pressed" << endl;
			m_scale *= 1.25;
			eventHandled = true;
		}
		if (key == GLFW_KEY_BACKSPACE)
		{
			cout << "BACKSPACE key pressed" << endl;
			m_scale *= 0.8;
			eventHandled = true;
		}
	}

	return eventHandled;
}
