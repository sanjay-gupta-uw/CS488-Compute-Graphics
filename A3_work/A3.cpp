// Termm-Fall 2020

#include "A3.hpp"
#include "scene_lua.hpp"
using namespace std;

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"

#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

static bool show_gui = true;

int iteration = 0;
const size_t CIRCLE_PTS = 48;
const int JOINT_MODE = 1;
const int POSITION_MODE = 0;
int transform_mode = 0;
int node_count = 0;
vector<int> joints;

//----------------------------------------------------------------------------------------
// Constructor
A3::A3(const std::string &luaSceneFile)
	: m_luaSceneFile(luaSceneFile),
	  m_positionAttribLocation(0),
	  m_normalAttribLocation(0),
	  m_vao_meshData(0),
	  m_vbo_vertexPositions(0),
	  m_vbo_vertexNormals(0),
	  m_vao_arcCircle(0),
	  m_vbo_arcCircle(0)
{
}

//----------------------------------------------------------------------------------------
// Destructor
A3::~A3()
{
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A3::init()
{
	// Set the background colour.
	glClearColor(0.32, 0.34, 0.33, 1.0);

	createShaderProgram();

	glGenVertexArrays(1, &m_vao_arcCircle);
	glGenVertexArrays(1, &m_vao_meshData);
	enableVertexShaderInputSlots();

	processLuaSceneFile(m_luaSceneFile);

	// Load and decode all .obj files at once here.  You may add additional .obj files to
	// this list in order to support rendering additional mesh types.  All vertex
	// positions, and normals will be extracted and stored within the MeshConsolidator
	// class.
	unique_ptr<MeshConsolidator> meshConsolidator(new MeshConsolidator{
		getAssetFilePath("cube.obj"),
		getAssetFilePath("sphere.obj"),
		getAssetFilePath("luffy_face.obj"),
		getAssetFilePath("luffy_neck.obj"),
		getAssetFilePath("luffy_torso.obj"),
		getAssetFilePath("luffy_right_upperarm.obj"),
		getAssetFilePath("luffy_left_upperarm.obj"),
		getAssetFilePath("luffy_right_forearm.obj"),
		getAssetFilePath("luffy_left_forearm.obj"),
		getAssetFilePath("luffy_right_hand.obj"),
		getAssetFilePath("luffy_left_hand.obj"),
		getAssetFilePath("luffy_right_thigh.obj"),
		getAssetFilePath("luffy_left_thigh.obj"),
		getAssetFilePath("luffy_right_calf.obj"),
		getAssetFilePath("luffy_left_calf.obj"),
		getAssetFilePath("luffy_right_foot.obj"),
		getAssetFilePath("luffy_left_foot.obj"),
		getAssetFilePath("suzanne.obj"),
	});

	// Acquire the BatchInfoMap from the MeshConsolidator.
	meshConsolidator->getBatchInfoMap(m_batchInfoMap);

	// Take all vertex data within the MeshConsolidator and upload it to VBOs on the GPU.
	uploadVertexDataToVbos(*meshConsolidator);

	mapVboDataToVertexShaderInputLocations();

	initPerspectiveMatrix();

	initViewMatrix();

	initLightSources();

	zBuffer = true;

	renderCircle = false;

	backFaceCulling = false;
	frontFaceCulling = false;

	node_count = m_rootNode->totalSceneNodes();
	selected.resize(node_count, false);

	nodes.resize(node_count);
	initial_positions.resize(node_count, vec3(0.0f));
	initial_transforms.resize(node_count);
	initial_transforms_unscaled.resize(node_count);
	initializeNodes(*m_rootNode);

	initial_angles_x.resize(joints.size());
	initial_angles_y.resize(joints.size());
	for (int i = 0; i < nodes.size(); ++i)
	{
		if (nodes[i]->m_nodeType == NodeType::JointNode)
		{
			joints.push_back(i);
			initial_angles_x.push_back(((JointNode *)nodes[i])->init_x);
			initial_angles_y.push_back(((JointNode *)nodes[i])->init_y);
		}
	}
	update_stack();

	// Exiting the current scope calls delete automatically on meshConsolidator freeing
	// all vertex data resources.  This is fine since we already copied this data to
	// VBOs on the GPU.  We have no use for storing vertex data on the CPU side beyond
	// this point.
}

void A3::initializeNodes(SceneNode &node)
{
	int id = node.m_nodeId;
	nodes[id] = &node;
	if (node.m_name == "head")
	{
		headNode = &node;
	}
	initial_positions[id] = vec3(node.get_transform()[3]);
	initial_transforms[id] = node.trans;
	for (SceneNode *child : node.children)
	{
		initializeNodes(*child);
	}
}

//----------------------------------------------------------------------------------------
void A3::processLuaSceneFile(const std::string &filename)
{
	// This version of the code treats the Lua file as an Asset,
	// so that you'd launch the program with just the filename
	// of a puppet in the Assets/ directory.
	std::string assetFilePath = getAssetFilePath(filename.c_str());
	m_rootNode = std::shared_ptr<SceneNode>(import_lua(assetFilePath));

	// This version of the code treats the main program argument
	// as a straightforward pathname.
	// m_rootNode = std::shared_ptr<SceneNode>(import_lua(filename));
	// if (!m_rootNode)
	// {
	// 	std::cerr << "Could Not Open " << filename << std::endl;
	// }
}

//----------------------------------------------------------------------------------------
void A3::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(getAssetFilePath("Phong.vs").c_str());
	m_shader.attachFragmentShader(getAssetFilePath("Phong.fs").c_str());

	m_shader.link();

	m_shader_arcCircle.generateProgramObject();
	m_shader_arcCircle.attachVertexShader(getAssetFilePath("arc_VertexShader.vs").c_str());
	m_shader_arcCircle.attachFragmentShader(getAssetFilePath("arc_FragmentShader.fs").c_str());
	m_shader_arcCircle.link();
}

//----------------------------------------------------------------------------------------
void A3::enableVertexShaderInputSlots()
{
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(m_vao_meshData);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_positionAttribLocation = m_shader.getAttribLocation("position");
		glEnableVertexAttribArray(m_positionAttribLocation);

		// Enable the vertex shader attribute location for "normal" when rendering.
		m_normalAttribLocation = m_shader.getAttribLocation("normal");
		glEnableVertexAttribArray(m_normalAttribLocation);

		CHECK_GL_ERRORS;
	}

	//-- Enable input slots for m_vao_arcCircle:
	{
		glBindVertexArray(m_vao_arcCircle);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_arc_positionAttribLocation = m_shader_arcCircle.getAttribLocation("position");
		glEnableVertexAttribArray(m_arc_positionAttribLocation);

		CHECK_GL_ERRORS;
	}

	// Restore defaults
	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------
void A3::uploadVertexDataToVbos(
	const MeshConsolidator &meshConsolidator)
{
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &m_vbo_vertexPositions);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);

		glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexPositionBytes(),
					 meshConsolidator.getVertexPositionDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store all vertex normal data
	{
		glGenBuffers(1, &m_vbo_vertexNormals);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);

		glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexNormalBytes(),
					 meshConsolidator.getVertexNormalDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store the trackball circle.
	{
		glGenBuffers(1, &m_vbo_arcCircle);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_arcCircle);

		float *pts = new float[2 * CIRCLE_PTS];
		for (size_t idx = 0; idx < CIRCLE_PTS; ++idx)
		{
			float ang = 2.0 * M_PI * float(idx) / CIRCLE_PTS;
			pts[2 * idx] = cos(ang);
			pts[2 * idx + 1] = sin(ang);
		}

		glBufferData(GL_ARRAY_BUFFER, 2 * CIRCLE_PTS * sizeof(float), pts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
void A3::mapVboDataToVertexShaderInputLocations()
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_meshData);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);
	glVertexAttribPointer(m_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexNormals" into the
	// "normal" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);
	glVertexAttribPointer(m_normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;

	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_arcCircle);

	// Tell GL how to map data from the vertex buffer "m_vbo_arcCircle" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_arcCircle);
	glVertexAttribPointer(m_arc_positionAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void A3::initPerspectiveMatrix()
{
	float aspect = ((float)m_windowWidth) / m_windowHeight;
	m_perpsective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 100.0f);
}

//----------------------------------------------------------------------------------------
void A3::initViewMatrix()
{
	// front view
	m_view = glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f),
						 vec3(0.0f, 1.0f, 0.0f));
	// top view
	// m_view = glm::lookAt(
	// 	glm::vec3(0.0f, 10.0f, 0.0f), // Camera is 10 units above the origin
	// 	glm::vec3(0.0f, 0.0f, -1.0f), // Looking at the origin
	// 	glm::vec3(0.0f, 0.0f, -1.0f)  // 'Up' is in the negative Z direction
	// );
}

//----------------------------------------------------------------------------------------
void A3::initLightSources()
{
	// World-space position
	// m_light.position = vec3(10.0f, 10.0f, 10.0f);
	m_light.position = vec3(-2.0f, 0.0f, 0.5f);

	// m_light.rgbIntensity = vec3(0.0f); // light
	m_light.rgbIntensity = vec3(0.8f); // white light
}

//----------------------------------------------------------------------------------------
void A3::uploadCommonSceneUniforms()
{
	m_shader.enable();
	{
		//-- Set Perpsective matrix uniform for the scene:
		GLint location = m_shader.getUniformLocation("Perspective");
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perpsective));
		CHECK_GL_ERRORS;

		location = m_shader.getUniformLocation("picking");
		glUniform1i(location, (transform_mode == JOINT_MODE) ? 1 : 0);

		if (transform_mode != JOINT_MODE)
		{
			//-- Set LightSource uniform for the scene:
			location = m_shader.getUniformLocation("light.position");
			glUniform3fv(location, 1, value_ptr(m_light.position));
			location = m_shader.getUniformLocation("light.rgbIntensity");
			glUniform3fv(location, 1, value_ptr(m_light.rgbIntensity));
			CHECK_GL_ERRORS;

			//-- Set background light ambient intensity
			location = m_shader.getUniformLocation("ambientIntensity");
			vec3 ambientIntensity(0.05f);
			glUniform3fv(location, 1, value_ptr(ambientIntensity));
			CHECK_GL_ERRORS;
		}
	}
	m_shader.disable();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A3::appLogic()
{
	// Place per frame, application logic here ...

	uploadCommonSceneUniforms();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A3::guiLogic()
{
	if (!show_gui)
	{
		return;
	}

	static bool firstRun(true);
	if (firstRun)
	{
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		firstRun = false;
	}

	static bool showDebugWindow(true);
	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	windowFlags |= ImGuiWindowFlags_MenuBar;
	float opacity(0.5f);

	ImGui::Begin("Properties", &showDebugWindow, ImVec2(100, 100), opacity,
				 windowFlags);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Application"))
		{
			if (ImGui::MenuItem("Reset Position", "I"))
			{
				reset_position();
			}
			if (ImGui::MenuItem("Reset Orientation", "O"))
			{
			}
			if (ImGui::MenuItem("Reset Joints", "S"))
			{
				reset_joints();
			}
			if (ImGui::MenuItem("Reset All", "A"))
			{
				reset_all();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "U"))
			{
				undo();
			}
			if (ImGui::MenuItem("Redo", "R"))
			{
				redo();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Options"))
		{
			if (ImGui::MenuItem("Circle", "C"))
			{
				renderCircle = !renderCircle;
			}
			if (ImGui::MenuItem("Z-buffer", "Z"))
			{
				zBuffer = !zBuffer;
			}
			if (ImGui::MenuItem("Backface culling", "B"))
			{
				backFaceCulling = !backFaceCulling;
			}
			if (ImGui::MenuItem("Frontface culling", "F"))
			{
				frontFaceCulling = !frontFaceCulling;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	ImGui::PushID(0);
	ImGui::Text("Position/Orientation (P)");
	ImGui::SameLine();
	if (ImGui::RadioButton("##Radio", &transform_mode, POSITION_MODE))
	{
	}
	ImGui::PopID();

	ImGui::PushID(1);
	ImGui::Text("Joints (J)");
	ImGui::SameLine();
	if (ImGui::RadioButton("##Radio", &transform_mode, JOINT_MODE))
	{
	}
	ImGui::PopID();

	if (ImGui::Button("Quit Application"))
	{
		glfwSetWindowShouldClose(m_window, GL_TRUE);
	}

	ImGui::Text("Framerate: %.1f FPS", ImGui::GetIO().Framerate);

	if (no_undo)
		ImGui::Text("Can't undo");
	if (no_redo)
		ImGui::Text("Can't redo");
	ImGui::End();
}

void A3::reset_position()
{
	// cout << "Resetting position" << endl;
	for (int i = 0; i < nodes.size(); ++i)
	{
		nodes[i]->trans[3] = vec4(initial_positions[i], 1.0f);
		nodes[i]->unscaled[3] = vec4(initial_positions[i], 1.0f);
	}
}

void A3::reset_orientation()
{
	// cout << "Resetting orientation" << endl;
	for (int i = 0; i < nodes.size(); ++i)
	{
		nodes[i]->trans[0] = initial_transforms[i][0];
		nodes[i]->trans[1] = initial_transforms[i][1];
		nodes[i]->trans[2] = initial_transforms[i][2];

		nodes[i]->unscaled[0] = initial_transforms_unscaled[i][0];
		nodes[i]->unscaled[1] = initial_transforms_unscaled[i][1];
		nodes[i]->unscaled[2] = initial_transforms_unscaled[i][2];
	}
}

void A3::reset_joints()
{
	// cout << "Resetting joints" << endl;
	for (int i = 0; i < joints.size(); ++i)
	{
		JointNode *joint = (JointNode *)nodes[joints[i]];
		joint->m_joint_x.init = initial_angles_x[i];
		joint->m_joint_y.init = initial_angles_y[i];
		selected[joints[i]] = false;
	}
	stack_index = -1;
	joint_angle_stack.resize(0);
	update_stack();
	no_redo = true;
	no_undo = true;
}

void A3::reset_all()
{
	// cout << "Resetting all" << endl;
	reset_position();
	reset_orientation();
	reset_joints();
	transform_mode = POSITION_MODE;

	zBuffer = true;
	renderCircle = false;
	backFaceCulling = false;
	frontFaceCulling = false;
}

void A3::update_stack()
{
	vector<glm::vec2> updated_angles;
	for (int i = 0; i < joints.size(); ++i)
	{
		JointNode *joint = (JointNode *)nodes[joints[i]];
		updated_angles.push_back(vec2(joint->m_joint_x.init, joint->m_joint_y.init));
	}
	joint_angle_stack.resize(stack_index + 1);
	joint_angle_stack.push_back(updated_angles);
	stack_index++;
}

void A3::undo()
{
	if (stack_index > 0)
	{
		stack_index--;
		vector<vec2> angles = joint_angle_stack[stack_index];
		for (int i = 0; i < joints.size(); ++i)
		{
			JointNode *joint = (JointNode *)nodes[joints[i]];
			joint->m_joint_x.init = angles[i].x;
			joint->m_joint_y.init = angles[i].y;
		}
	}
	else
	{
		// output message (not console) that there are no more undos
		// cout << "No more undos" << endl;
		no_undo = true;
	}
}

void A3::redo()
{
	if (stack_index < joint_angle_stack.size() - 1)
	{
		stack_index++;
		vector<vec2> angles = joint_angle_stack[stack_index];
		for (int i = 0; i < joints.size(); ++i)
		{
			JointNode *joint = (JointNode *)nodes[joints[i]];
			joint->m_joint_x.init = angles[i].x;
			joint->m_joint_y.init = angles[i].y;
		}
	}
	else
	{
		// output message (not console) that there are no more redos
		// cout << "No more redos" << endl;
		no_redo = true;
	}
}

//----------------------------------------------------------------------------------------
// Update mesh specific shader uniforms:
void updateShaderUniforms(
	const ShaderProgram &shader,
	const GeometryNode &node,
	const glm::mat4 &viewMatrix,
	const glm::mat4 &trans,
	bool parent_joint_selected)
{
	shader.enable();
	{
		//-- Set ModelView matrix:
		GLint location = shader.getUniformLocation("ModelView");
		mat4 modelView = viewMatrix * trans;
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));
		CHECK_GL_ERRORS;

		if (transform_mode == JOINT_MODE)
		{
			int idx = node.m_nodeId;
			float r = float(idx & 0xff) / 255.0f;
			float g = float((idx >> 8) & 0xff) / 255.0f;
			float b = float((idx >> 16) & 0xff) / 255.0f;

			if (parent_joint_selected)
			{
				r = 1.0f;
				g = float((((255 - node_count + idx + 1) * 255) >> 8) & 0xff) / 255.0f;
				b = 0.0f;
			}

			location = shader.getUniformLocation("material.kd");
			glUniform3f(location, r, g, b);
			CHECK_GL_ERRORS;
		}
		else
		{
			//-- Set NormMatrix:
			location = shader.getUniformLocation("NormalMatrix");
			mat3 normalMatrix = glm::transpose(glm::inverse(mat3(modelView)));
			glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));
			CHECK_GL_ERRORS;

			//-- Set Material values:
			location = shader.getUniformLocation("material.kd");
			vec3 kd = node.material.kd;
			if (parent_joint_selected)
			{
				kd = vec3(1.0f, 1.0f, 0.0f);
			}
			glUniform3fv(location, 1, value_ptr(kd));
			CHECK_GL_ERRORS;
		}
	}
	shader.disable();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A3::draw()
{

	if (zBuffer)
	{
		glEnable(GL_DEPTH_TEST);
	}
	if (backFaceCulling && frontFaceCulling)
	{
		// If both are true, this is likely an error state, as you cannot cull both front and back faces.
		// You need to decide what to do in this case.
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT_AND_BACK);
	}
	else if (backFaceCulling)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
	else if (frontFaceCulling)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}

	// Bind the VAO once here, and reuse for all GeometryNode rendering below.
	glBindVertexArray(m_vao_meshData);
	renderSceneGraph(*m_rootNode);
	iteration++;

	glBindVertexArray(0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	if (renderCircle)
	{
		renderArcCircle();
	}
}

void A3::updateTransformations(float oldX = 0.0f, float newX = 0.0f, float oldY = 0.0f, float newY = 0.0f, bool drag = true)
{

	if (transform_mode == POSITION_MODE)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (mouse_input[i])
			{
				if (i == 0) // model x,y translations for left mouse click
				{
					// cout << "x_change[i]: " << x_change[i] << endl;
					m_rootNode->translate(vec3(x_change[i] / 100, y_change[i] / 100, 0));
				}
				if (i == 2) // model z translation for right mouse click
				{
					m_rootNode->translate(vec3(0, 0, -y_change[i] / 100));
				}
				if (i == 1) // model rotation for middle mouse click
				{
					updateRotationTransformations(oldX, newX, oldY, newY);
				}
			}
		}
	}
	else if (transform_mode == JOINT_MODE)
	{
		if (mouse_input[0] && !drag)
		{
			double xpos, ypos;
			glfwGetCursorPos(m_window, &xpos, &ypos);
			uploadCommonSceneUniforms();
			// glClearColor(1.0, 1.0, 1.0, 1.0);
			// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			// glClearColor(0.35, 0.35, 0.35, 1.0);

			draw();

			xpos *= double(m_framebufferWidth) / double(m_windowWidth);
			ypos = m_windowHeight - ypos;
			ypos *= double(m_framebufferHeight) / double(m_windowHeight);

			// cout << "xpos: " << xpos << ", ypos: " << ypos << endl;

			GLubyte buffer[4] = {0, 0, 0, 0};
			glReadBuffer(GL_BACK);
			glReadPixels(int(xpos), int(ypos), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

			int r = buffer[0];
			int g = buffer[1];
			int b = buffer[2];

			int potential_idx = r + (g << 8) + (b << 16);
			int selected_idx = buffer[1] - 255 + node_count;
			// 255 - node_count + idx + 1

			int idx = (potential_idx < node_count) ? potential_idx : selected_idx;
			if (idx <= nodes.size() - 1 && idx >= 0)
			{
				// cout << "idx: " << idx << endl;
				// cout << "node name: " << nodes[idx]->m_name << endl;
				int parent = nodes[idx]->parent->m_nodeId;
				// cout << "Selected Parent Node: " << nodes[parent]->m_name << endl;
				selected[parent] = !selected[parent];
				// cout << "Selected Parent Node: " << selected[parent] << endl;
			}
			CHECK_GL_ERRORS;
		}
		if (mouse_input[2] && drag) // increase/decrease joints
		{
			// go through selected
			double val_x = x_change[2];
			double val_y = y_change[2];
			// cout << "val_x: " << val_x << ", val_y: " << val_y << endl;
			for (int i = 0; i < selected.size(); ++i)
			{
				if (selected[i])
				{
					JointNode *joint = (JointNode *)nodes[i];
					if (headNode != nullptr && headNode->parent->m_nodeId == i)
					{
						cout << "head" << endl;
						joint->update_x(val_y);
					}
					else
					{
						joint->update_x(val_x);
						joint->update_y(val_y);
					}
				}
			}
		}
		if (mouse_input[1] && drag)
		{
			// rotate head
			double val_y = x_change[1];
			if (headNode != nullptr)
			{
				JointNode *joint = (JointNode *)nodes[headNode->parent->m_nodeId];
				joint->update_y(val_y);
			}
		}
	}
}
void vCalcRotVec(float fNewX, float fNewY,
				 float fOldX, float fOldY,
				 float fDiameter,
				 float *fVecX, float *fVecY, float *fVecZ)
{
	long nXOrigin, nYOrigin;
	float fNewVecX, fNewVecY, fNewVecZ, /* Vector corresponding to new mouse location */
		fOldVecX, fOldVecY, fOldVecZ,	/* Vector corresponding to old mouse location */
		fLength;

	/* Vector pointing from center of virtual trackball to
	 * new mouse position
	 */

	fNewVecX = fNewX * 2.0 / fDiameter;
	fNewVecY = fNewY * 2.0 / fDiameter;
	fNewVecZ = (1.0 - fNewVecX * fNewVecX - fNewVecY * fNewVecY);

	/* If the Z component is less than 0, the mouse point
	 * falls outside of the trackball which is interpreted
	 * as rotation about the Z axis.
	 */
	if (fNewVecZ < 0.0)
	{
		// cout << "cursor out of ball" << endl;
		fLength = sqrt(1.0 - fNewVecZ);
		fNewVecZ = 0.0;
		fNewVecX /= fLength;
		fNewVecY /= fLength;
	}
	else
	{
		fNewVecZ = sqrt(fNewVecZ);
	}

	/* Vector pointing from center of virtual trackball to
	 * old mouse position
	 */
	fOldVecX = fOldX * 2.0 / fDiameter;
	fOldVecY = fOldY * 2.0 / fDiameter;
	fOldVecZ = (1.0 - fOldVecX * fOldVecX - fOldVecY * fOldVecY);

	/* If the Z component is less than 0, the mouse point
	 * falls outside of the trackball which is interpreted
	 * as rotation about the Z axis.
	 */
	if (fOldVecZ < 0.0)
	{
		fLength = sqrt(1.0 - fOldVecZ);
		fOldVecZ = 0.0;
		fOldVecX /= fLength;
		fOldVecY /= fLength;
	}
	else
	{
		fOldVecZ = sqrt(fOldVecZ);
	}

	/* Generate rotation vector by calculating cross product:
	 *
	 * fOldVec x fNewVec.
	 *
	 * The rotation vector is the axis of rotation
	 * and is non-unit length since the length of a crossproduct
	 * is related to the angle between fOldVec and fNewVec which we need
	 * in order to perform the rotation.
	 */
	*fVecX = fOldVecY * fNewVecZ - fNewVecY * fOldVecZ;
	*fVecY = fOldVecZ * fNewVecX - fNewVecZ * fOldVecX;
	*fVecZ = fOldVecX * fNewVecY - fNewVecX * fOldVecY;
}

void vAxisRotMatrix(float fVecX, float fVecY, float fVecZ, glm::mat4 &mNewMat)
{
	float fRadians, fInvLength, fNewVecX, fNewVecY, fNewVecZ;

	/* Find the length of the vector which is the angle of rotation
	 * (in radians)
	 */
	fRadians = sqrt(fVecX * fVecX + fVecY * fVecY + fVecZ * fVecZ);

	/* If the vector has zero length - return the identity matrix */
	if (fRadians > -0.000001 && fRadians < 0.000001)
	{
		return;
	}

	/* Normalize the rotation vector now in preparation for making
	 * rotation matrix.
	 */
	fInvLength = 1 / fRadians;
	fNewVecX = fVecX * fInvLength;
	fNewVecY = fVecY * fInvLength;
	fNewVecZ = fVecZ * fInvLength;

	/* Create the arbitrary axis rotation matrix */
	double dSinAlpha = sin(fRadians);
	double dCosAlpha = cos(fRadians);
	double dT = 1 - dCosAlpha;

	mNewMat = {
		vec4(dCosAlpha + fNewVecX * fNewVecX * dT,
			 fNewVecX * fNewVecY * dT + fNewVecZ * dSinAlpha,
			 fNewVecX * fNewVecZ * dT - fNewVecY * dSinAlpha,
			 0),

		vec4(fNewVecX * fNewVecY * dT - dSinAlpha * fNewVecZ,
			 dCosAlpha + fNewVecY * fNewVecY * dT,
			 fNewVecY * fNewVecZ * dT + dSinAlpha * fNewVecX,
			 0),
		vec4(fNewVecZ * fNewVecX * dT + dSinAlpha * fNewVecY,
			 fNewVecZ * fNewVecY * dT - dSinAlpha * fNewVecX,
			 dCosAlpha + fNewVecZ * fNewVecZ * dT,
			 0),
		vec4(0, 0, 0, 1),
	};
}

void A3::updateRotationTransformations(float fOldX, float fNewX, float fOldY, float fNewY)
{
	/*
	 * Track ball rotations are being used.
	 */
	float fRotVecX, fRotVecY, fRotVecZ;
	float fDiameter;
	int iCenterX, iCenterY;
	float fNewModX, fNewModY, fOldModX, fOldModY;

	/* vCalcRotVec expects new and old positions in relation to the center of the
	 * trackball circle which is centered in the middle of the window and
	 * half the smaller of m_framebufferWidth or m_framebufferHeight.
	 */

	// cout << "iCenterX: " << iCenterX << ", iCenterY: " << iCenterY << endl;
	// cout << "oldx: " << prev_x[2] << ", oldy: " << prev_y[2] << endl;

	fDiameter = (m_framebufferWidth < m_framebufferHeight) ? m_framebufferWidth * 0.5 : m_framebufferHeight * 0.5;
	iCenterX = m_framebufferWidth / 2;
	iCenterY = m_framebufferHeight / 2;
	fOldModX = iCenterX - fOldX;
	fOldModY = iCenterY - fOldY;
	fNewModX = iCenterX - fNewX;
	fNewModY = iCenterY - fNewY;

	vCalcRotVec(fNewModX, fNewModY,
				fOldModX, fOldModY,
				fDiameter,
				&fRotVecX, &fRotVecY, &fRotVecZ);
	/* Negate Y component since Y axis increases downwards
	 * in screen space and upwards in OpenGL.
	 */

	mat4 mRotations = mat4(1.0f);
	vAxisRotMatrix(fRotVecX, -fRotVecY, fRotVecZ, mRotations);

	m_rootNode->trans = m_rootNode->trans * mRotations;

	m_rootNode->unscaled = m_rootNode->unscaled * mRotations;

	// rotate view when outside of circle
	// m_view.rotate(fRotVecX, -fRotVecY, fRotVecZ);
}

//----------------------------------------------------------------------------------------
void A3::renderSceneGraph(const SceneNode &root)
{
	// You'll want to turn this into recursive code that walks over the tree.
	// You can do that by putting a method in SceneNode, overridden in its
	// subclasses, that renders the subtree rooted at every node.  Or you
	// could put a set of mutually recursive functions in this class, which
	// walk down the tree from nodes of different types.

	// cout << "root trans: " << root.get_transform() << endl;
	for (SceneNode *node : root.children)
	{
		processSceneNode(*node, root.unscaled);
	}
	// exit(0);
	CHECK_GL_ERRORS;
}

void A3::processSceneNode(SceneNode &node, glm::mat4 parentTransform)
{
	// if (iteration == 0)
	// 	cout << "node name: " << node.m_name << ", ID: " << node.m_nodeId << endl;
	if (node.m_nodeType == NodeType::GeometryNode)
	{
		GeometryNode *geometryNode = (GeometryNode *)&node;
		renderGeometryNode(*geometryNode, parentTransform);
	}
	else if (node.m_nodeType == NodeType::JointNode)
	{
		JointNode *jointNode = (JointNode *)&node;
		mat4 rot = jointNode->get_rot(parentTransform);
		for (SceneNode *child : node.children)
		{
			processSceneNode(*child, rot);
			// processSceneNode(*child, rot * parentTransform);
		}
	}
}

void A3::renderGeometryNode(GeometryNode &node, mat4 parentTransform)
{
	SceneNode *parent = node.parent;
	updateShaderUniforms(m_shader,
						 node,
						 m_view,
						 parentTransform * node.trans,
						 (parent->m_nodeType == NodeType::JointNode && selected[parent->m_nodeId]));

	// // Get the BatchInfo corresponding to the GeometryNode's unique MeshId.
	// cout << "node, meshId: " << node.meshId << endl;
	// cout << "node.name: " << node.m_name << endl;
	// cout << "transform: " << trans << endl;
	// cout << endl;
	BatchInfo batchInfo = m_batchInfoMap.at(node.meshId);

	//-- Now render the mesh:
	m_shader.enable();
	glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
	m_shader.disable();

	for (SceneNode *child : node.children)
	{
		processSceneNode(*child, parentTransform * node.unscaled);
	}
}

//-----------------------------------s-----------------------------------------------------
// Draw the trackball circle.
void A3::renderArcCircle()
{
	glBindVertexArray(m_vao_arcCircle);

	m_shader_arcCircle.enable();
	GLint m_location = m_shader_arcCircle.getUniformLocation("M");
	float aspect = float(m_framebufferWidth) / float(m_framebufferHeight);
	glm::mat4 M;
	if (aspect > 1.0)
	{
		M = glm::scale(glm::mat4(), glm::vec3(0.5 / aspect, 0.5, 1.0));
	}
	else
	{
		M = glm::scale(glm::mat4(), glm::vec3(0.5, 0.5 * aspect, 1.0));
	}
	glUniformMatrix4fv(m_location, 1, GL_FALSE, value_ptr(M));
	glDrawArrays(GL_LINE_LOOP, 0, CIRCLE_PTS);
	m_shader_arcCircle.disable();

	glBindVertexArray(0);
	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A3::cleanup()
{
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A3::cursorEnterWindowEvent(
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
bool A3::mouseMoveEvent(
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
				if (prev_x[i] != -1 || prev_y[i] != -1)
				{
					x_change[i] = xPos - prev_x[i];
					y_change[i] = prev_y[i] - yPos;
					updateTransformations(prev_x[1], xPos, prev_y[1], yPos, (transform_mode == POSITION_MODE) ? false : true);
				}
				prev_x[i] = xPos;
				prev_y[i] = yPos;
			}
		}
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A3::mouseButtonInputEvent(
	int button,
	int actions,
	int mods)
{
	bool eventHandled(false);
	no_redo = false;
	no_undo = false;
	// button 0 = left mouse button
	// button 1 = right mouse button
	// button 2 = middle mouse button

	// actions 0 = release
	// actions 1 = press
	if (actions == 1)
	{
		mouse_input[button] = true;
		if (transform_mode == JOINT_MODE)
			updateTransformations(0.0, 0.0, 0.0, 0.0, false);
	}
	else if (actions == 0)
	{
		mouse_input[button] = false;
		prev_x[button] = -1;
		x_change[button] = 0;
		prev_y[button] = -1;
		y_change[button] = 0;

		vector<glm::vec2> top_stack = joint_angle_stack[stack_index];
		for (int i = 0; i < joints.size(); ++i)
		{
			JointNode *joint = (JointNode *)nodes[joints[i]];
			if (top_stack[i] != vec2(joint->m_joint_x.init, joint->m_joint_y.init))
			{
				update_stack();
				cout << "updating stack" << endl;
				break;
			}
		}
	}

	eventHandled = true;

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A3::mouseScrollEvent(
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
bool A3::windowResizeEvent(
	int width,
	int height)
{
	bool eventHandled(false);
	initPerspectiveMatrix();
	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A3::keyInputEvent(
	int key,
	int action,
	int mods)
{
	bool eventHandled(false);
	no_redo = false;
	no_undo = false;
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_M)
		{
			show_gui = !show_gui;
			eventHandled = true;
		}
		if (key == GLFW_KEY_Q)
		{
			eventHandled = true;
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}
		if (key == GLFW_KEY_C)
		{
			renderCircle = !renderCircle;
			eventHandled = true;
		}
		if (key == GLFW_KEY_Z)
		{
			zBuffer = !zBuffer;
			eventHandled = true;
		}
		if (key == GLFW_KEY_B)
		{
			backFaceCulling = !backFaceCulling;
			eventHandled = true;
		}
		if (key == GLFW_KEY_F)
		{
			frontFaceCulling = !frontFaceCulling;
			eventHandled = true;
		}
		if (key == GLFW_KEY_J)
		{
			transform_mode = JOINT_MODE;
			eventHandled = true;
		}
		if (key == GLFW_KEY_P)
		{
			transform_mode = POSITION_MODE;
			eventHandled = true;
		}
		if (key == GLFW_KEY_I)
		{
			reset_position();
			eventHandled = true;
		}
		if (key == GLFW_KEY_O)
		{
			reset_orientation();
			eventHandled = true;
		}
		if (key == GLFW_KEY_S)
		{
			reset_joints();
			eventHandled = true;
		}
		if (key == GLFW_KEY_A)
		{
			reset_all();
			eventHandled = true;
		}
		if (key == GLFW_KEY_U)
		{
			undo();
			eventHandled = true;
		}
		if (key == GLFW_KEY_R)
		{
			redo();
			eventHandled = true;
		}
	}
	// Fill in with event handling code...

	return eventHandled;
}
