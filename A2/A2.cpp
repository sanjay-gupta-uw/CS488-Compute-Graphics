// Termm--Fall 2020

#include "A2.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>
using namespace std;

#include <imgui/imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace glm;

const int LOCAL_X_AXIS = 0;
const int LOCAL_Y_AXIS = 1;
const int LOCAL_Z_AXIS = 2;

const int MODEL = 0;
const int VIEW = 1;

const int LEFT = 0;
const int RIGHT = 1;
const int MIDDLE = 2;

const int RELEASE = 0;
const int PRESS = 1;
//----------------------------------------------------------------------------------------
// Constructor
VertexData::VertexData()
	: numVertices(0),
	  index(0)
{
	positions.resize(kMaxVertices);
	colours.resize(kMaxVertices);
}

CoordinateSystem3D::CoordinateSystem3D(vec3 origin, vec3 x, vec3 y, vec3 z)
	: origin(origin),
	  x(x),
	  y(y),
	  z(z)
{
}

void Camera::updateCoordinateSystem()
{
	vec3 v_z = normalize(position - lookAt);
	vec3 v_x = normalize(cross(up, v_z));
	vec3 v_y = cross(v_z, v_x);

	camera_coordinateSystem = CoordinateSystem3D(position, v_x, v_y, v_z);
}

Camera::Camera(vec3 position, vec3 lookAt, vec3 up, float near, float far, float fov)
	: position(position),
	  lookAt(lookAt),
	  up(up),
	  near(near),
	  far(far),
	  fov(fov)
{
	updateCoordinateSystem();
}
//----------------------------------------------------------------------------------------
// Constructor
A2::A2()
	: m_currentLineColour(vec3(0.0f))
{
}

//----------------------------------------------------------------------------------------
// Destructor
A2::~A2()
{
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A2::init()
{
	// Set the background colour.
	glClearColor(0.3, 0.5, 0.7, 1.0);

	createShaderProgram();

	glGenVertexArrays(1, &m_vao);

	enableVertexAttribIndices();

	generateVertexBuffers();

	mapVboDataToVertexAttributeLocation();
	world_coordinateSystem = CoordinateSystem3D(vec3(0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));

	// Set up the camera in a default position.
	// position, lookat, up, near, far, fov
	m_camera = Camera(vec3(5.0f, 5.0f, 5.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), -8.0f, -10.0f, 120);

	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			translate[i][j] = 0.0f;
			rotate[i][j] = 0.0f;
			scale[j] = 0.25f;
		}
	}

	// Model:
	generateScaleMatrix();
	generateRotationMatrix(true);
	generateTranslationMatrix(true);

	// View:
	generateRotationMatrix(false);
	generateTranslationMatrix(false);
	generateViewMatrix();
	generatePerspectiveMatrix();

	updateTransformations();

	M_viewport = mat4(1.0f);
}

void A2::generateScaleMatrix()
{
	m_scale = {
		vec4(scale[LOCAL_X_AXIS], 0.0f, 0.0f, 0.0f),
		vec4(0.0f, scale[LOCAL_Y_AXIS], 0.0f, 0.0f),
		vec4(0.0f, 0.0f, scale[LOCAL_Z_AXIS], 0.0f),
		vec4(0.0f, 0.0f, 0.0f, 1.0f)};
}

void A2::generateRotationMatrix(bool isModel)
{
	CoordinateSystem3D coor_system = isModel ? m_cube.cube_coordinateSystem : m_camera.camera_coordinateSystem;
	int MODE = isModel ? MODEL : VIEW;

	float x_rad = radians(rotate[MODE][LOCAL_X_AXIS]);
	float y_rad = radians(rotate[MODE][LOCAL_Y_AXIS]);
	float z_rad = radians(rotate[MODE][LOCAL_Z_AXIS]);

	m_rotate[MODE][LOCAL_X_AXIS] = {
		vec4(1.0f, 0.0f, 0.0f, 0.0f),
		vec4(0.0f, cos(x_rad), sin(x_rad), 0.0f),
		vec4(0.0f, -sin(x_rad), cos(x_rad), 0.0f),
		vec4(0.0f, 0.0f, 0.0f, 1.0f),
	};

	m_rotate[MODE][LOCAL_Y_AXIS] = {
		vec4(cos(y_rad), 0.0f, -sin(y_rad), 0.0f),
		vec4(0.0f, 1.0f, 0.0f, 0.0f),
		vec4(sin(y_rad), 0.0f, cos(y_rad), 0.0f),
		vec4(0.0f, 0.0f, 0.0f, 1.0f),
	};

	m_rotate[MODE][LOCAL_Z_AXIS] = {
		vec4(cos(z_rad), sin(z_rad), 0.0f, 0.0f),
		vec4(-sin(z_rad), cos(z_rad), 0.0f, 0.0f),
		vec4(0.0f, 0.0f, 1.0f, 0.0f),
		vec4(0.0f, 0.0f, 0.0f, 1.0f),
	};

	if (!isModel)
	{
		vec3 axis[3] = {coor_system.x, coor_system.y, coor_system.z};
		mat4 R = m_rotate[VIEW][0] * m_rotate[VIEW][1] * m_rotate[VIEW][2];
		m_camera.camera_coordinateSystem.x = vec3(R * vec4(coor_system.x, 1.0f));
		m_camera.camera_coordinateSystem.y = vec3(R * vec4(coor_system.y, 1.0f));
		m_camera.camera_coordinateSystem.z = vec3(R * vec4(coor_system.z, 1.0f));
	}
}

void A2::generateTranslationMatrix(bool isModel)
{
	int MODE = MODEL;
	vec3 origin = isModel ? m_cube.cube_coordinateSystem.origin : m_camera.camera_coordinateSystem.origin;

	if (!isModel)
	{
		MODE = VIEW;
		origin *= -1;
	}

	m_translate[MODE] = mat4(1.0f);
	m_translate[MODE][3] = vec4(origin, 1.0f);
}

void A2::generateViewMatrix()
{
	vec3 camera_x_axis = m_camera.camera_coordinateSystem.x;
	vec3 camera_y_axis = m_camera.camera_coordinateSystem.y;
	vec3 camera_z_axis = m_camera.camera_coordinateSystem.z;

	mat4 R = {
		vec4(camera_x_axis.x, camera_y_axis.x, camera_z_axis.x, 0.0f),
		vec4(camera_x_axis.y, camera_y_axis.y, camera_z_axis.y, 0.0f),
		vec4(camera_x_axis.z, camera_y_axis.z, camera_z_axis.z, 0.0f),
		vec4(0.0f, 0.0f, 0.0f, 1.0f)};

	V = R * m_translate[VIEW];
}

void A2::generatePerspectiveMatrix()
{
	float theta = radians(m_camera.fov / 2);
	P = {
		vec4((float)((1 / tan(theta)) / (float)(viewportWidth / viewportHeight)), 0.0f, 0.0f, 0.0f),
		vec4(0.0f, (float)(1 / tan(theta)), 0.0f, 0.0f),
		vec4(0.0f, 0.0f, -(m_camera.far + m_camera.near) / (m_camera.near - m_camera.far), -1.0f),
		vec4(0.0f, 0.0f, (-2 * m_camera.far * m_camera.near) / (m_camera.far - m_camera.near), 0.0f),
	};
}

void A2::reset()
{
	m_cube.reset();
	// setup variable to track initial camera state
	m_camera = Camera(vec3(5.0f, 5.0f, 5.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), -8.0f, -10.0f, 120);

	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			translate[i][j] = 0.0f;
			rotate[i][j] = 0.0f;
			scale[j] = 0.25f;
		}
	}

	// Model:
	generateScaleMatrix();
	generateRotationMatrix(true);
	generateTranslationMatrix(true);

	// View:
	generateRotationMatrix(false);
	generateTranslationMatrix(false);
	generateViewMatrix();
	generatePerspectiveMatrix();

	updateTransformations();

	transform_mode = 3;
}
//----------------------------------------------------------------------------------------
void A2::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(getAssetFilePath("VertexShader.vs").c_str());
	m_shader.attachFragmentShader(getAssetFilePath("FragmentShader.fs").c_str());
	m_shader.link();
}

//---------------------------------------------------------------------------------------- Spring 2020
void A2::enableVertexAttribIndices()
{
	glBindVertexArray(m_vao);

	// Enable the attribute index location for "position" when rendering.
	GLint positionAttribLocation = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(positionAttribLocation);

	// Enable the attribute index location for "colour" when rendering.
	GLint colourAttribLocation = m_shader.getAttribLocation("colour");
	glEnableVertexAttribArray(colourAttribLocation);

	// Restore defaults
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void A2::generateVertexBuffers()
{
	// Generate a vertex buffer to store line vertex positions
	{
		glGenBuffers(1, &m_vbo_positions);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);

		// Set to GL_DYNAMIC_DRAW because the data store will be modified frequently.
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * kMaxVertices, nullptr,
					 GL_DYNAMIC_DRAW);

		// Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}

	// Generate a vertex buffer to store line colors
	{
		glGenBuffers(1, &m_vbo_colours);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);

		// Set to GL_DYNAMIC_DRAW because the data store will be modified frequently.
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * kMaxVertices, nullptr,
					 GL_DYNAMIC_DRAW);

		// Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
void A2::mapVboDataToVertexAttributeLocation()
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao);

	// Tell GL how to map data from the vertex buffer "m_vbo_positions" into the
	// "position" vertex attribute index for any bound shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
	GLint positionAttribLocation = m_shader.getAttribLocation("position");
	glVertexAttribPointer(positionAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_colours" into the
	// "colour" vertex attribute index for any bound shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);
	GLint colorAttribLocation = m_shader.getAttribLocation("colour");
	glVertexAttribPointer(colorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//---------------------------------------------------------------------------------------
void A2::initLineData()
{
	m_vertexData.numVertices = 0;
	m_vertexData.index = 0;
}

//---------------------------------------------------------------------------------------
void A2::setLineColour(
	const glm::vec3 &colour)
{
	m_currentLineColour = colour;
}

bool A2::clipNearPlane(glm::vec4 &P0,
					   glm::vec4 &P1)
{
	bool valid = false;
	bool print = false;
	if (P0.z < P1.z)
	{
		std::swap(P0, P1);
	}
	int outcode_0 = m_camera.computeOutcode(P0);
	int outcode_1 = m_camera.computeOutcode(P1);

	// if (P0.z > m_camera.near && P1.z < m_camera.near)
	// {
	// 	cout << "P0.z: " << P0.z << ", P1.z: " << P1.z << endl;
	// 	cout << "outcode_0: " << outcode_0 << endl;
	// 	cout << "outcode_1: " << outcode_1 << endl;
	// 	cout << (outcode_0 | outcode_1) << endl;
	// 	cout << (outcode_0 & outcode_1) << endl;
	// 	print = true;
	// }
	if ((outcode_0 | outcode_1) == 0)
	{
		// trivial accept
		valid = true;
	}
	else if ((outcode_0 & outcode_1) != 0)
	{
		// trivial reject
	}
	else
	{
		// cout << "clipping " << P0.x << "," << P0.y << "," << P0.z << " to near plane." << endl;
		int t = (m_camera.near - P0.z) / (P1.z - P0.z);
		P0.x = P0.x + t * (P1.x - P0.x);
		P0.y = P0.y + t * (P1.y - P0.y);
		P0.z = m_camera.near;
		// cout << "RES: " << P0.x << "," << P0.y << "," << P0.z << endl;
		valid = true;
		print = true;
	}
	// if (print)
	// 	cout << endl;
	return valid;
}

//---------------------------------------------------------------------------------------
void A2::drawLine(
	const glm::vec3 &V0_, // Line Start (NDC coordinate)
	const glm::vec3 &V1_, // Line End (NDC coordinate)
	bool viewport = false)
{
	vec2 V0;
	vec2 V1;
	if (viewport)
	{
		V0 = vec2(V0_);
		V1 = vec2(V1_);
	}
	else
	{
		// apply transformations on V0, V1
		// V = mat4(1.0f);
		vec4 v0_trans = V * M * vec4(V0_, 1.0f);
		vec4 v1_trans = V * M * vec4(V1_, 1.0f);
		// vec4 v0_trans = M * vec4(V0_, 1.0f);
		// vec4 v1_trans = M * vec4(V1_, 1.0f);

		// clipping against near/far planes
		bool res = clipNearPlane(v0_trans, v1_trans);
		// points are invalid after clipping
		if (!res)
		{
			return;
		}
		vec4 v0_perspect = P * v0_trans;
		vec4 v1_perspect = P * v1_trans;
		// clip against normalized cube

		V0 = vec2(v0_perspect.x, v0_perspect.y);
		V1 = vec2(v1_perspect.x, v1_perspect.y);
	}

	// apply transformations on V0, V1
	m_vertexData.positions[m_vertexData.index] = V0;
	m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
	++m_vertexData.index;
	m_vertexData.positions[m_vertexData.index] = V1;
	m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
	++m_vertexData.index;

	m_vertexData.numVertices += 2;
}

void A2::drawViewportOutline()
{
	float margin_width_ndc = (float)(window_width - viewportWidth) / window_width;
	float margin_height_ndc = (float)(window_height - viewportHeight) / window_height;

	float left = -1.0f + margin_width_ndc;
	float right = 1.0f - margin_width_ndc;
	float bottom = -1.0f + margin_height_ndc;
	float top = 1.0f - margin_height_ndc;

	setLineColour(vec3(1.0f, 1.0f, 1.0f));
	drawLine(vec3(left, bottom, 0.0f), vec3(right, bottom, 0.0f), true);
	drawLine(vec3(right, bottom, 0.0f), vec3(right, top, 0.0f), true);
	drawLine(vec3(right, top, 0.0f), vec3(left, top, 0.0f), true);
	drawLine(vec3(left, top, 0.0f), vec3(left, bottom, 0.0f), true);
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A2::appLogic()
{
	// Place per frame, application logic here ...
	initLineData();

	// draw cube gnomons
	setLineColour(vec3(1.0f, 1.0f, 0.0f));
	// drawLine(m_cube.cube_coordinateSystem.origin, m_cube.cube_coordinateSystem.origin + m_cube.cube_coordinateSystem.x);
	drawLine(world_coordinateSystem.origin, world_coordinateSystem.x);
	setLineColour(vec3(0.0f, 1.0f, 1.0f));
	// drawLine(m_cube.cube_coordinateSystem.origin, m_cube.cube_coordinateSystem.origin + m_cube.cube_coordinateSystem.y);
	drawLine(world_coordinateSystem.origin, world_coordinateSystem.y);
	setLineColour(vec3(1.0f, 0.0f, 1.0f));
	// drawLine(m_cube.cube_coordinateSystem.origin, m_cube.cube_coordinateSystem.origin + m_cube.cube_coordinateSystem.z);
	drawLine(world_coordinateSystem.origin, world_coordinateSystem.z);

	// draw CUBE
	setLineColour(vec3(1.0f, 0.7f, 0.8f));
	int ct = 0;
	for (int i = 0; i < 2; ++i)
	{
		drawLine(m_cube.cube_vertices[ct], m_cube.cube_vertices[ct + 1]);
		drawLine(m_cube.cube_vertices[ct + 1], m_cube.cube_vertices[ct + 2]);
		drawLine(m_cube.cube_vertices[ct + 2], m_cube.cube_vertices[ct + 3]);
		drawLine(m_cube.cube_vertices[ct + 3], m_cube.cube_vertices[ct]);
		ct += 4;
	}
	for (int i = 0; i < 4; ++i)
	{
		drawLine(m_cube.cube_vertices[i], m_cube.cube_vertices[i + 4]);
	}

	// M = mat4(1.0f);
	// // draw world gnomons
	// setLineColour(vec3(1.0f, 0.0f, 0.0f));
	// drawLine(world_coordinateSystem.origin, world_coordinateSystem.x);
	// setLineColour(vec3(0.0f, 1.0f, 0.0f));
	// drawLine(world_coordinateSystem.origin, world_coordinateSystem.y);
	// setLineColour(vec3(0.0f, 0.0f, 1.0f));
	// drawLine(world_coordinateSystem.origin, world_coordinateSystem.z);

	drawViewportOutline();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A2::guiLogic()
{
	static bool firstRun(true);
	if (firstRun)
	{
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		firstRun = false;
	}

	static bool showDebugWindow(true);
	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Properties", &showDebugWindow, ImVec2(100, 100), opacity,
				 windowFlags);

	const char *labels[7] = {
		"Rotate View     (o)",
		"Translate View  (e)",
		"Perspective     (p)",
		"Rotate Model    (r)",
		"Translate Model (t)",
		"Scale Model     (s)",
		"Viewport        (v)"};
	for (int i = 0; i < 7; ++i)
	{
		ImGui::PushID(i);
		ImGui::Text("%s", labels[i]);
		ImGui::SameLine();
		if (ImGui::RadioButton("##Radio", &transform_mode, i))
		{
		}
		ImGui::PopID();
	}

	if (ImGui::Button("Reset (a)"))
	{
		reset();
	}
	// Create Button, and check if it was clicked:
	if (ImGui::Button("Quit Application (q)"))
	{
		glfwSetWindowShouldClose(m_window, GL_TRUE);
	}

	ImGui::Text("Framerate: %.1f FPS", ImGui::GetIO().Framerate);
	ImGui::Text("Near: %.2f", m_camera.near);
	ImGui::SameLine();
	ImGui::Text(", Far: %.2f", m_camera.far);
	ImGui::SameLine();
	ImGui::Text(", FOV: %.2f", m_camera.fov);
	ImGui::End();
}

//----------------------------------------------------------------------------------------
void A2::uploadVertexDataToVbos()
{

	//-- Copy vertex position data into VBO, m_vbo_positions:
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2) * m_vertexData.numVertices,
						m_vertexData.positions.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}

	//-- Copy vertex colour data into VBO, m_vbo_colours:
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * m_vertexData.numVertices,
						m_vertexData.colours.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A2::draw()
{
	uploadVertexDataToVbos();

	glBindVertexArray(m_vao);

	m_shader.enable();
	glDrawArrays(GL_LINES, 0, m_vertexData.numVertices);
	m_shader.disable();

	// Restore defaults
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A2::cleanup()
{
}

void A2::updateTransformations()
{
	float angle_mult = (float)(360.0f / viewportWidth);
	// float translation_mult = (float)(360.0f / viewportWidth);
	if (transform_mode == 0)
	{
		// rotate view
		for (int i = 0; i < 3; ++i)
		{
			if (mouse_input[i])
			{
				rotate[MODEL][i] += x_change[i] * angle_mult;
			}
		}
		generateRotationMatrix(false);
		generateViewMatrix();
	}
	else if (transform_mode == 1)
	{
		// translate view
		for (int i = 0; i < 3; ++i)
		{
			if (mouse_input[i])
			{
				translate[VIEW][i] += x_change[i] * 0.01;
				m_camera.camera_coordinateSystem.origin[i] = translate[VIEW][i];
			}
		}
		generateTranslationMatrix(false);
		generateViewMatrix();
	}
	else if (transform_mode == 2)
	{
		// perspective
		// LEFT_MOUSE_BUTTON: FOV between 5 and 160
		// MIDDLE_MOUSE_BUTTON: translate near plane along view direction
		// RIGHT_MOUSE_BUTTON: translate far plane along view direction
	}
	else if (transform_mode == 3)
	{
		// rotate model
		for (int i = 0; i < 3; ++i)
		{
			if (mouse_input[i])
			{
				rotate[MODEL][i] += x_change[i] * angle_mult;
			}
		}
		generateRotationMatrix(true);
	}
	else if (transform_mode == 4)
	{
		// translate model
		// make sure Z updates
		// bug: holding down and not moving moves the model
		for (int i = 0; i < 3; ++i)
		{
			if (mouse_input[i])
			{
				translate[MODEL][i] += x_change[i] * 0.005;
				m_cube.cube_coordinateSystem.origin[i] = translate[MODEL][i];
			}
		}
		generateTranslationMatrix(true);
	}
	else if (transform_mode == 5)
	{
		// scale model
		for (int i = 0; i < 3; ++i)
		{
			if (mouse_input[i])
			{
				scale[i] += x_change[i] * 0.001;
			}
		}
		generateScaleMatrix();
	}
	else if (transform_mode == 6)
	{
		// VIEWPORT
	}
	M = m_translate[MODEL] * m_rotate[MODEL][0] * m_rotate[MODEL][1] * m_rotate[MODEL][2] * m_scale;
}
//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A2::cursorEnterWindowEvent(
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
bool A2::mouseMoveEvent(
	double xPos,
	double yPos)
{
	bool eventHandled(false);
	if (!ImGui::IsMouseHoveringAnyWindow())
	{
		for (int i = 0; i < 3; ++i)
		{
			if (mouse_input[i])
			{
				if (prev_x[i] != -1)
				{
					x_change[i] = prev_x[i] - xPos;
					updateTransformations();
				}
				else
				{
					x_change[i] = 0;
				}
				prev_x[i] = xPos;
			}
		}
	}
	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A2::mouseButtonInputEvent(
	int button,
	int actions,
	int mods)
{
	bool eventHandled(false);

	// button 0 = left mouse button
	// button 1 = right mouse button
	// button 2 = middle mouse button

	// actions 0 = release
	// actions 1 = press
	if (actions == PRESS)
	{
		mouse_input[button] = true;
	}
	else if (actions == RELEASE)
	{
		mouse_input[button] = false;
		prev_x[button] = -1;
		x_change[button] = 0;
	}

	eventHandled = true;

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A2::mouseScrollEvent(
	double xOffSet,
	double yOffSet)
{
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A2::windowResizeEvent(
	int width,
	int height)
{
	bool eventHandled(false);

	window_height = height;
	window_width = width;

	float factor = 0.05f;

	float marginWidth = factor * width;
	float marginHeight = factor * height;
	viewportWidth = (int)(width * (1 - (factor * 2)));
	viewportHeight = (int)(height * (1 - (factor * 2)));
	glViewport(marginWidth, marginHeight, viewportWidth, viewportHeight);
	eventHandled = true;

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A2::keyInputEvent(
	int key,
	int action,
	int mods)
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
		else if (key == GLFW_KEY_A)
		{
			reset();
			eventHandled = true;
		}
		else if (key == GLFW_KEY_O)
		{
			transform_mode = 0;
			eventHandled = true;
		}
		else if (key == GLFW_KEY_E)
		{
			transform_mode = 1;
			eventHandled = true;
		}
		else if (key == GLFW_KEY_P)
		{
			transform_mode = 2;
			eventHandled = true;
		}
		else if (key == GLFW_KEY_R)
		{
			transform_mode = 3;
			eventHandled = true;
		}
		else if (key == GLFW_KEY_T)
		{
			transform_mode = 4;
			eventHandled = true;
		}
		else if (key == GLFW_KEY_S)
		{
			transform_mode = 5;
			eventHandled = true;
		}
		else if (key == GLFW_KEY_V)
		{
			transform_mode = 6;
			eventHandled = true;
		}
	}
	return eventHandled;
}
