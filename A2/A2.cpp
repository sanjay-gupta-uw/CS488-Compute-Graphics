// Termm--Fall 2020

#include "A2.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>
using namespace std;

#include <imgui/imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

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

void Camera::updateCoordinateSystem()
{
	vec3 direction;
	direction.x = cos(radians(yaw)) * cos(radians(pitch));
	direction.y = sin(radians(pitch));
	direction.z = sin(radians(yaw)) * cos(radians(pitch));
	vec3 front = normalize(direction);

	vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);
	vec3 right = normalize(cross(front, worldUp));
	vec3 up_ = normalize(cross(right, front));

	if (roll != 0)
	{
		up_ = cos(radians(roll)) * up_ + sin(radians(roll)) * cross(front, up_);
		right = cross(up_, front);
	}

	camera_coordinateSystem.x = vec4(right, 0.0f);
	camera_coordinateSystem.y = vec4(up_, 0.0f);
	camera_coordinateSystem.z = vec4(-front, 0.0f);

	up = vec4(up_, 0.0f);
	lookAt = position - vec4(front, 0.0f); // RHCS
}

Camera::Camera(vec4 position, vec4 lookAt, vec4 up, float near, float far, float fov)
	: position(position),
	  lookAt(lookAt),
	  up(up),
	  near(near),
	  far(far),
	  fov(fov)
{
	// updateCoordinateSystem();
}

void printM(mat4 &M, char *name)
{
	cout << name << ":" << endl;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			cout << M[j][i] << " ";
		}
		cout << endl;
	}
	cout << endl;
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

	// Set up the camera in a default position.
	// position, lookat, up, near, far, fov
	m_camera = Camera(vec4(1.0f, 1.0f, 3.0f, 1.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 1.0f, 0.0f, 0.0f), 0.1f, 20.0f, 30);

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
	g_scale = {
		vec4(0.1f, 0.0f, 0.0f, 0.0f),
		vec4(0.0f, 0.1f, 0.0f, 0.0f),
		vec4(0.0f, 0.0f, 0.1f, 0.0f),
		vec4(0.0f, 0.0f, 0.0f, 1.0f),
	};
	generateRotationMatrix(true);
	generateTranslationMatrix(true);

	// View:
	generateViewMatrix();
	initializeViewport();
	generatePerspectiveMatrix();

	// updateTransformations();
	M_model = m_translate[MODEL] * m_rotate[MODEL][0] * m_rotate[MODEL][1] * m_rotate[MODEL][2];

	M_viewport = mat4(1.0f);
}
void A2::reset()
{
	m_cube.reset();
	// setup variable to track initial camera state
	m_camera = Camera(vec4(1.0f, 1.0f, 3.0f, 1.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 1.0f, 0.0f, 0.0f), 0.1f, 20.0f, 30);
	m_camera.pitch = 375.3f;
	m_camera.yaw = 792.4f;
	m_camera.roll = 0.2f;

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
	generateViewMatrix();
	initializeViewport();
	generatePerspectiveMatrix();

	// updateTransformations();
	M_model = m_translate[MODEL] * m_rotate[MODEL][0] * m_rotate[MODEL][1] * m_rotate[MODEL][2];

	M_viewport = mat4(1.0f);
	transform_mode = 3;
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

	m_rotate[MODE][LOCAL_Z_AXIS] = {
		vec4(cos(z_rad), sin(z_rad), 0.0f, 0.0f),
		vec4(-sin(z_rad), cos(z_rad), 0.0f, 0.0f),
		vec4(0.0f, 0.0f, 1.0f, 0.0f),
		vec4(0.0f, 0.0f, 0.0f, 1.0f),
	};

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

	if (MODE == VIEW)
	{
		m_camera.pitch += rotate[MODE][LOCAL_X_AXIS];
		m_camera.yaw += rotate[MODE][LOCAL_Y_AXIS];
		m_camera.roll += rotate[MODE][LOCAL_Z_AXIS];
	}
}

void A2::generateTranslationMatrix(bool isModel)
{
	int MODE = isModel ? MODEL : VIEW;
	m_translate[MODE] = mat4(1.0f);
	m_translate[MODE][3] = vec4(translate[MODE][0], translate[MODE][1], translate[MODE][2], 1.0f);

	if (!isModel)
	{
		for (int i = 0; i < 3; ++i)
		{
			m_camera.position[i] += translate[MODE][i];
		}
	}
}

void A2::generateViewMatrix()
{
	m_camera.updateCoordinateSystem();
	V = glm::lookAt(vec3(m_camera.position), vec3(m_camera.lookAt), vec3(m_camera.up));
}

void A2::generatePerspectiveMatrix()
{
	float aspect_ratio = (float)viewportWidth / (float)viewportHeight;
	float scale_x = (aspect_ratio < 1.0f) ? aspect_ratio : 1.0f;
	float scale_y = (aspect_ratio > 1.0f) ? (1.0f / aspect_ratio) : 1.0f;
	float theta = radians(m_camera.fov) / 2;
	P = {
		vec4(scale_x * ((float)(1 / tan(theta)) / ((float)viewportWidth / (float)viewportHeight)), 0.0f, 0.0f, 0.0f),
		vec4(0.0f, scale_y * (float)(1 / tan(theta)), 0.0f, 0.0f),
		vec4(0.0f, 0.0f, -(m_camera.far + m_camera.near) / (m_camera.far - m_camera.near), -1),
		vec4(0.0f, 0.0f, (-2 * m_camera.far * m_camera.near) / (m_camera.far - m_camera.near), 0.0f),
	};
}

void A2::initializeViewport()
{
	float factor = 0.05f;

	viewportWidth = (int)(window_width * (1 - (factor * 2)));
	viewportHeight = (int)(window_height * (1 - (factor * 2)));

	float margin_left = window_width * factor;
	float margin_top = window_height * factor;

	corners_new[0] = vec2(margin_left, margin_top);
	corners_new[1] = vec2(margin_left + viewportWidth, margin_top + viewportHeight);
	updateViewport();
}
void A2::updateViewport()
{
	// (Top LEFT:) (0, 0)
	// (Bottom LEFT:) (0, window_height)
	// (Top RIGHT:) (window_width, 0)
	// (Bottom RIGHT:) (window_width, window_height)

	float left = corners_new[0].x < corners_new[1].x ? corners_new[0].x : corners_new[1].x;
	float right = corners_new[0].x > corners_new[1].x ? corners_new[0].x : corners_new[1].x;
	float bottom = corners_new[0].y > corners_new[1].y ? corners_new[0].y : corners_new[1].y;
	float top = corners_new[0].y < corners_new[1].y ? corners_new[0].y : corners_new[1].y;

	if (abs(left - right) < 0.01)
	{
		if (abs(left) < 0.01)
		{
			right += 1;
		}
		else
		{
			left -= 1;
		}
	}
	if (abs(bottom - top) < 0)
	{
		if (abs(top) < 0.01)
		{
			bottom += 1;
		}
		else
		{
			top -= 1;
		}
	}

	viewportWidth = (int)(right - left);
	viewportHeight = (int)(bottom - top);

	// NDCS
	left = ((left * 2) / (float)window_width) - 1;
	right = ((right * 2) / (float)window_width) - 1;
	bottom = 1 - ((bottom * 2) / (float)window_height);
	top = 1 - ((top * 2) / (float)window_height);

	corners[0] = vec2(left, bottom);
	corners[1] = vec2(right, top);
	generatePerspectiveMatrix();
}

void A2::drawViewportOutline()
{
	float left = corners[0].x;
	float right = corners[1].x;
	float bottom = corners[0].y;
	float top = corners[1].y;

	setLineColour(vec3(1.0f, 1.0f, 1.0f));
	drawLine(vec2(left, bottom), vec2(right, bottom));
	drawLine(vec2(right, bottom), vec2(right, top));
	drawLine(vec2(right, top), vec2(left, top));
	drawLine(vec2(left, top), vec2(left, bottom));
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
	if (P0.z < P1.z)
	{
		std::swap(P0, P1);
	}
	int outcode_0 = m_camera.computeOutcode(P0);
	int outcode_1 = m_camera.computeOutcode(P1);

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
		int t = (-m_camera.near - P0.z) / (P1.z - P0.z);
		P0.x = P0.x + t * (P1.x - P0.x);
		P0.y = P0.y + t * (P1.y - P0.y);
		P0.z = -m_camera.near;
		valid = true;
	}
	return valid;
}

bool A2::clipCube(glm::vec4 &P0,
				  glm::vec4 &P1)
{
	int outcode[2] = {0, 0};
	// normalize points to homogenous coordinates:
	P0 = P0 / P0.w;
	P1 = P1 / P1.w;
	vec4 points[2] = {P0, P1};
	float x;
	float y;
	float z;
	float w;

	for (int i = 0; i < 2; ++i)
	{
		float x = points[i].x;
		float y = points[i].y;
		float z = points[i].z;
		float w = points[i].w;
		if (x < -1) // left
		{
			outcode[i] |= 1;
		}
		if (x > 1) // right
		{
			outcode[i] |= 2;
		}
		if (y < -1) // bottom
		{
			outcode[i] |= 4;
		}
		if (y > 1) // top
		{
			outcode[i] |= 8;
		}
		if (z < -1) // near
		{
			outcode[i] |= 16;
		}
		if (z > 1) // far
		{
			outcode[i] |= 32;
		}
	}
	if ((outcode[0] | outcode[1]) == 0)
	{
		// trivial accept
		return true;
	}
	else if ((outcode[0] & outcode[1]) != 0)
	{
		// trivial reject
		return false;
	}
	else
	{
		vec3 normals[6] = {
			vec3(1.0f, 0.0f, 0.0f),	 // Left plane (x = -1 in NDC)
			vec3(-1.0f, 0.0f, 0.0f), // Right plane (x = 1 in NDC)
			vec3(0.0f, 1.0f, 0.0f),	 // Bottom plane (y = -1 in NDC)
			vec3(0.0f, -1.0f, 0.0f), // Top plane (y = 1 in NDC)
			vec3(0.0f, 0.0f, 1.0f),	 // Near plane (z = -1 in NDC)
			vec3(0.0f, 0.0f, -1.0f), // Far plane (z = 1 in NDC)
		};

		int plane_codes[6] = {1, 2, 4, 8, 16, 32};

		float t = 0;
		float A = 0;
		float B = 0;
		float C = 0;
		bool P0_is_outside = false;
		for (int i = 0; i < 6; ++i)
		{
			vec4 out = vec4(0.0f, 0.0f, 0.0f, 1.0f);
			vec4 in = vec4(0.0f, 0.0f, 0.0f, 1.0f);
			if ((outcode[0] & plane_codes[i]) == plane_codes[i])
			{
				// P0 is outside the plane
				// P1 must be inside the plane
				P0_is_outside = true;
				out = P0;
				in = P1;
			}
			else if ((outcode[1] & plane_codes[i]) == plane_codes[i])
			{
				// P1 is outside the plane
				// P0 must be inside the plane
				out = P1;
				in = P0;
			}
			else
			{
				// both points are inside the plane
				continue;
			}

			A = normals[i].x;
			B = normals[i].y;
			C = normals[i].z;

			vec4 direction = in - out;

			t = -(A * out.x + B * out.y + C * out.z + 1) / (A * direction.x + B * direction.y + C * direction.z);
			if (t < 0 || t > 1)
			{
				continue;
			}

			vec4 intersection = out + t * direction;

			if (P0_is_outside)
			{
				P0 = intersection;
			}
			else
			{
				P1 = intersection;
			}
			P0_is_outside = false;
		}
		return true;
	}
}

array<vec2, 2> A2::getNDC(vec4 &P0, vec4 &P1)
{
	vec4 P0_view = V * M * Scale * P0;
	vec4 P1_view = V * M * Scale * P1;

	bool res = clipNearPlane(P0_view, P1_view);
	if (!res) // points are invalid after clipping
	{

		return {vec2(2.0f), vec2(2.0f)};
	}

	vec4 V0_persp = P * P0_view;
	vec4 V1_persp = P * P1_view;

	// clip against normalized cube
	res = clipCube(V0_persp, V1_persp);
	if (!res)
	{
		return {vec2(2.0f), vec2(2.0f)};
	}

	transformToViewport(V0_persp);
	transformToViewport(V1_persp);

	return {vec2(V0_persp), vec2(V1_persp)};
}

void A2::transformToViewport(vec4 &P)
{
	// Get the viewport bounds from the corners array
	float left = corners[0].x;
	float bottom = corners[0].y;
	float right = corners[1].x;
	float top = corners[1].y;

	float ndc_width = right - left;
	float ndc_height = top - bottom;

	float aspect_ratio_vp = ndc_width / ndc_height;

	P.x = (P.x + 1.0f) / 2.0f;
	P.x *= ndc_width;

	P.y = (P.y + 1.0f) / 2.0f;
	P.y *= ndc_height;

	// if (aspect_ratio_vp > 1.0f)
	// {
	// 	P.y /= aspect_ratio_vp;
	// }
	// else
	// {
	// 	P.x *= aspect_ratio_vp;
	// }
	P.x += left;
	P.y += bottom;
	if (P.x < left || P.x > right || P.y < bottom || P.y > top)
	{
		bool solved = false;
		if (abs(P.x - left) < 0.01)
		{
			P.x = left;
			solved = true;
		}
		else if (abs(P.x - right) < 0.01)
		{
			P.x = right;
			solved = true;
		}
		if (abs(P.y - bottom) < 0.01)
		{
			P.y = bottom;
			solved = true;
		}
		else if (abs(P.y - top) < 0.01)
		{
			P.y = top;
			solved = true;
		}
		if (!solved)
		{
			P.x = 2.0f;
			P.y = 2.0f;
		}
	}
}

//---------------------------------------------------------------------------------------
void A2::drawLine(
	const glm::vec2 &V0, // Line Start (NDC coordinate)
	const glm::vec2 &V1	 // Line End (NDC coordinate)
)
{
	m_vertexData.positions[m_vertexData.index] = V0;
	m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
	++m_vertexData.index;
	m_vertexData.positions[m_vertexData.index] = V1;
	m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
	++m_vertexData.index;

	m_vertexData.numVertices += 2;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A2::appLogic()
{
	// Place per frame, application logic here ...
	initLineData();
	Scale = m_scale;
	drawModel();

	M = mat4(1.0f);
	// draw world gnomons
	vec4 points[4] = {
		world_coordinateSystem.origin,
		vec4(vec3(world_coordinateSystem.x), 1.0f),
		vec4(vec3(world_coordinateSystem.y), 1.0f),
		vec4(vec3(world_coordinateSystem.z), 1.0f),
	};

	vec3 colours[3] = {
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f),
	};
	for (int i = 1; i < 4; ++i)
	{
		array<vec2, 2> p = getNDC(points[0], points[i]);
		if (p[0].x > 1.0f)
		{
			continue;
		}
		setLineColour(colours[i - 1]);
		drawLine(p[0], p[1]);
	}

	drawViewportOutline();
}

void A2::drawModel()
{
	M = M_model;
	// draw CUBE
	setLineColour(vec3(1.0f, 0.7f, 0.8f));
	int ct = 0;
	for (int i = 0; i < 2; ++i)
	{
		for (int j = i * 4; j < (i + 1) * 4; ++j)
		{
			array<vec2, 2> points = getNDC(m_cube.cube_vertices[j], m_cube.cube_vertices[(i * 4) + ((j + 1) % 4)]);
			if (points[0].x > 1.0f)
			{
				continue;
			}
			drawLine(points[0], points[1]);
		}
		ct += 4;
	}
	for (int i = 0; i < 4; ++i)
	{
		array<vec2, 2> points = getNDC(m_cube.cube_vertices[i], m_cube.cube_vertices[i + 4]);
		if (points[0].x > 1.0f)
		{
			continue;
		}
		drawLine(points[0], points[1]);
	}

	Scale = g_scale;
	// draw cube gnomons
	vec4 points[4] = {
		world_coordinateSystem.origin,
		vec4(vec3(world_coordinateSystem.x), 1.0f),
		vec4(vec3(world_coordinateSystem.y), 1.0f),
		vec4(vec3(world_coordinateSystem.z), 1.0f),
	};

	vec3 colours[3] = {
		vec3(1.0f, 1.0f, 0.0f),
		vec3(0.0f, 1.0f, 1.0f),
		vec3(1.0f, 0.0f, 1.0f),
	};
	for (int i = 1; i < 4; ++i)
	{
		array<vec2, 2> p = getNDC(points[0], points[i]);
		if (p[0].x > 1.0f)
		{
			continue;
		}
		setLineColour(colours[i - 1]);
		drawLine(p[0], p[1]);
	}
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
				rotate[VIEW][i] = x_change[i] * angle_mult;
			}
			else
			{
				rotate[VIEW][i] = 0;
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
				translate[VIEW][i] = x_change[i] * 0.01;
			}
			else
			{
				translate[VIEW][i] = 0;
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
		if (mouse_input[LEFT])
		{
			// cout << "x_change[LEFT]: " << x_change[LEFT] << endl;
			m_camera.fov += x_change[LEFT] * 0.5;
			if (m_camera.fov < 5.0f)
			{
				m_camera.fov = 5.0f;
			}
			else if (m_camera.fov > 160.0f)
			{
				m_camera.fov = 160.0f;
			}
			else if (m_camera.fov < 5.0f)
			{
				m_camera.fov = 5.0f;
			}
		}
		if (mouse_input[MIDDLE])
		{
			m_camera.near += x_change[MIDDLE] * 0.01;

			if (m_camera.near < 0.1f)
			{
				m_camera.near = 0.1f;
			}
			if (m_camera.near > 20.0f)
			{
				m_camera.near = 20.0f;
			}
		}
		else if (mouse_input[RIGHT])
		{
			m_camera.far += x_change[RIGHT] * 0.01;
			if (m_camera.far > 100.0f)
			{
				m_camera.far = 100.0f;
			}
		}
		if (m_camera.far - m_camera.near < 0.5f)
		{
			m_camera.far = m_camera.near + 0.5f;
		}
		generatePerspectiveMatrix();
	}
	else if (transform_mode == 3)
	{
		// rotate model
		for (int i = 0; i < 3; ++i)
		{
			if (mouse_input[i])
			{
				rotate[MODEL][i] = x_change[i] * angle_mult;
			}
			else
			{
				rotate[MODEL][i] = 0;
			}
		}
		generateRotationMatrix(true);
		mat4 T = m_rotate[MODEL][0] * m_rotate[MODEL][1] * m_rotate[MODEL][2];
		M_model *= T;
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
				translate[MODEL][i] = x_change[i] * 0.01;
			}
			else
			{
				translate[MODEL][i] = 0;
			}
		}
		generateTranslationMatrix(true);
		M_model *= m_translate[MODEL];
	}

	else if (transform_mode == 5)
	{
		// scale model
		for (int i = 0; i < 3; ++i)
		{
			if (mouse_input[i])
			{
				scale[i] += x_change[i] * 0.01;
				if (scale[i] < 0.1f)
				{
					scale[i] = 0.1f;
				}
			}
		}
		generateScaleMatrix();
	}
	else if (transform_mode == 6)
	{
		// VIEWPORT
	}
}
//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A2::cursorEnterWindowEvent(int entered)
{
	bool eventHandled(false);
	if (transform_mode == 6 && drawing && entered == 0)
	{
		float xPos = 0.0f;
		float yPos = 0.0f;
		for (int i = 0; i < 2; ++i)
		{
			xPos = corners_new[i].x;
			yPos = corners_new[i].y;

			corners_new[i].x = xPos < 0 ? 0 : (xPos > window_width ? window_width : xPos);
			corners_new[i].y = yPos < 0 ? 0 : (yPos > window_height ? window_height : yPos);
		}
		drawing = false;
		end_draw = false;
		mouse_input[LEFT] = false;
		updateViewport();
		eventHandled = true;
	}
	return eventHandled;
}

// //----------------------------------------------------------------------------------------
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
		if (transform_mode == 6 && drawing)
		{
			if (start_draw)
			{
				// set first corner
				start_draw = false;
				corners_new[0] = vec2(xPos, yPos);
			}
			corners_new[1] = vec2(xPos, yPos);
			if (end_draw)
			{
				end_draw = false;
				drawing = false;
			}

			for (int i = 0; i < 2; ++i)
			{
				xPos = corners_new[i].x;
				yPos = corners_new[i].y;

				corners_new[i].x = xPos < 0 ? 0 : (xPos > window_width ? window_width : xPos);
				corners_new[i].y = yPos < 0 ? 0 : (yPos > window_height ? window_height : yPos);
			}
			updateViewport();
		}
		else
		{
			for (int i = 0; i < 3; ++i)
			{
				if (mouse_input[i])
				{
					if (prev_x[i] != -1)
					{
						x_change[i] = xPos - prev_x[i];
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
		if (transform_mode == 6 && mouse_input[LEFT] && !drawing)
		{
			start_draw = true;
			drawing = true;
		}
	}
	else if (actions == RELEASE)
	{
		mouse_input[button] = false;
		prev_x[button] = -1;
		x_change[button] = 0;
		if (transform_mode == 6 && drawing)
		{
			end_draw = true;
		}
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

	initializeViewport();
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