// Termm-Fall 2020

#include "A5.hpp"
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

int node_count = 0;
vector<int> joints;

// last time:
static double lastTime = glfwGetTime();

int iter = 0;

static const GLfloat g_vertex_buffer_data[] = {
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
	-0.5f, 0.5f, 0.0f,
	0.5f, 0.5f, 0.0f};

//----------------------------------------------------------------------------------------
// Constructor
A5::A5(const std::string &luaSceneFile)
	: m_luaSceneFile(luaSceneFile),
	  m_positionAttribLocation(0),
	  m_normalAttribLocation(0),
	  m_vao_meshData(0),
	  m_vao_particleData(0),
	  m_vbo_vertexPositions(0),
	  m_vbo_vertexNormals(0)
{
}

//----------------------------------------------------------------------------------------
// Destructor
A5::~A5()
{
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A5::init()
{
	// Set the background colour.
	glClearColor(0.32, 0.34, 0.33, 1.0);

	createShaderProgram();

	glGenVertexArrays(1, &m_vao_meshData);
	glGenVertexArrays(1, &m_vao_particleData);
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
	uploadVertexDataToVbos(*meshConsolidator); // renders scene objects

	mapVboDataToVertexShaderInputLocations();

	initPerspectiveMatrix();

	initViewMatrix();

	initLightSources();

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
	// Exiting the current scope calls delete automatically on meshConsolidator freeing
	// all vertex data resources.  This is fine since we already copied this data to
	// VBOs on the GPU.  We have no use for storing vertex data on the CPU side beyond
	// this point.
}

void A5::initializeNodes(SceneNode &node)
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
void A5::processLuaSceneFile(const std::string &filename)
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
void A5::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(getAssetFilePath("Phong.vs").c_str());
	m_shader.attachFragmentShader(getAssetFilePath("Phong.fs").c_str());

	m_shader.link();

	m_particleShader.generateProgramObject();
	m_particleShader.attachVertexShader(getAssetFilePath("Particle.vs").c_str());
	m_particleShader.attachFragmentShader(getAssetFilePath("Particle.fs").c_str());
	m_particleShader.link();
}

//----------------------------------------------------------------------------------------
void A5::enableVertexShaderInputSlots()
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

	{
		glBindVertexArray(m_vao_particleData);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		CHECK_GL_ERRORS;
	}
	// Restore defaults
	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------
void A5::uploadVertexDataToVbos(
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
	// particle system
	{
		glGenBuffers(1, &particle_data.billboard_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particle_data.billboard_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

		glGenBuffers(1, &particle_data.particles_position_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particle_data.particles_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, particle_data.MAX_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

		glGenBuffers(1, &particle_data.particles_color_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particle_data.particles_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, particle_data.MAX_PARTICLES * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
void A5::mapVboDataToVertexShaderInputLocations()
{
	{
		m_shader.enable();
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
		m_shader.disable();
	}

	{
		// particle system
		m_particleShader.enable();
		glBindVertexArray(m_vao_particleData);
		glBindBuffer(GL_ARRAY_BUFFER, particle_data.billboard_vertex_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
		CHECK_GL_ERRORS;

		glBindBuffer(GL_ARRAY_BUFFER, particle_data.particles_position_buffer);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void *)0);

		glBindBuffer(GL_ARRAY_BUFFER, particle_data.particles_color_buffer);
		glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void *)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		m_particleShader.disable();
	}
}

//----------------------------------------------------------------------------------------
void A5::initPerspectiveMatrix()
{
	float aspect = ((float)m_windowWidth) / m_windowHeight;
	m_perpsective = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
}

//----------------------------------------------------------------------------------------
void A5::initViewMatrix()
{
	// front view
	camera_position = vec3(0.0f, 0.0f, 5.0f);
	m_view = lookAt(camera_position, vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));

	// m_view = glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f),
	// 					 vec3(0.0f, 1.0f, 0.0f));
}

//----------------------------------------------------------------------------------------
void A5::initLightSources()
{
	// World-space position
	// m_light.position = vec3(10.0f, 10.0f, 10.0f);
	m_light.position = vec3(-2.0f, 0.0f, 0.5f);

	// m_light.rgbIntensity = vec3(0.0f); // light
	m_light.rgbIntensity = vec3(0.8f); // white light
}

//----------------------------------------------------------------------------------------
void A5::uploadCommonSceneUniforms()
{
	m_shader.enable();
	{
		//-- Set Perpsective matrix uniform for the scene:
		GLint location = m_shader.getUniformLocation("Perspective");
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perpsective));
		CHECK_GL_ERRORS;

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

	m_particleShader.enable();
	{
		// pass texture
		// pass camera world space
		glBindVertexArray(m_vao_particleData);

		GLint location = m_particleShader.getUniformLocation("CameraRight_worldspace");
		glUniform3f(location, m_view[0][0], m_view[1][0], m_view[2][0]);

		location = m_particleShader.getUniformLocation("CameraUp_worldspace");
		glUniform3f(location, m_view[0][1], m_view[1][1], m_view[2][1]);

		location = m_particleShader.getUniformLocation("VP");
		glUniformMatrix4fv(location, 1, GL_FALSE, &m_view_proj[0][0]);

		glBindBuffer(GL_ARRAY_BUFFER, particle_data.particles_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, particle_data.MAX_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, particle_data.ParticlesCount * sizeof(GLfloat) * 4, particle_data.g_particle_position_size_data);

		glBindBuffer(GL_ARRAY_BUFFER, particle_data.particles_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, particle_data.MAX_PARTICLES * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, particle_data.ParticlesCount * sizeof(GLubyte) * 4, particle_data.g_particle_color_data);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, particle_data.billboard_vertex_buffer);
		glVertexAttribPointer(
			0,		  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,		  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized?
			0,		  // stride
			(void *)0 // array buffer offset
		);

		// 2nd attribute buffer : positions of particles' centers
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, particle_data.particles_position_buffer);
		glVertexAttribPointer(
			1,		  // attribute. No particular reason for 1, but must match the layout in the shader.
			4,		  // size : x + y + z + size => 4
			GL_FLOAT, // type
			GL_FALSE, // normalized?
			0,		  // stride
			(void *)0 // array buffer offset
		);

		// 3rd attribute buffer : particles' colors
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, particle_data.particles_color_buffer);
		glVertexAttribPointer(
			2,				  // attribute. No particular reason for 1, but must match the layout in the shader.
			4,				  // size : r + g + b + a => 4
			GL_UNSIGNED_BYTE, // type
			GL_TRUE,		  // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
			0,				  // stride
			(void *)0		  // array buffer offset
		);

		CHECK_GL_ERRORS;
	}
	m_particleShader.disable();
}

void A5::generateParticles()
{
	if (iter > 0)
		return;
	int newparticles = (int)(delta_time * 10000.0);
	if (newparticles > (int)(0.016f * 10000.0))
		newparticles = (int)(0.016f * 10000.0);

	for (int i = 0; i < newparticles; ++i)
	{
		int particleIndex = particle_data.FindUnusedParticle();
		particle_data.particles_container[particleIndex].life = 5.0f; // This particle will live 5 seconds.
		particle_data.particles_container[particleIndex].pos = vec3(0, 0, -20.0f);

		float spread = 1.5f;
		vec3 maindir = vec3(0.0f, 10.0f, 0.0f);
		// Very bad way to generate a random direction;
		// See for instance http://stackoverflow.com/questions/5408276/python-uniform-spherical-distribution instead,
		// combined with some user-controlled parameters (main direction, spread, etc)
		vec3 randomdir = vec3(
			(rand() % 2000 - 1000.0f) / 1000.0f,
			(rand() % 2000 - 1000.0f) / 1000.0f,
			(rand() % 2000 - 1000.0f) / 1000.0f);

		particle_data.particles_container[particleIndex].speed = maindir + randomdir * spread;

		// Very bad way to generate a random color
		particle_data.particles_container[particleIndex].r = rand() % 256;
		particle_data.particles_container[particleIndex].g = rand() % 256;
		particle_data.particles_container[particleIndex].b = rand() % 256;
		particle_data.particles_container[particleIndex].a = (rand() % 256) / 3;

		particle_data.particles_container[particleIndex].size = (rand() % 1000) / 2000.0f + 0.1f;

		// Simulate all particles
		int ParticlesCount = 0;
		for (int i = 0; i < particle_data.MAX_PARTICLES; ++i)
		{
			Particle &p = particle_data.particles_container[i]; // shortcut

			if (p.life > 0.0f)
			{

				// Decrease life
				p.life -= delta_time;
				if (p.life > 0.0f)
				{
					// Simulate simple physics : gravity only, no collisions
					p.speed += vec3(0.0f, -9.81f, 0.0f) * (float)delta_time * 0.5f;
					p.pos += p.speed * (float)delta_time;
					p.cameradistance = length2(p.pos - camera_position);
					// ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

					// Fill the GPU buffer
					particle_data.g_particle_position_size_data[4 * ParticlesCount + 0] = p.pos.x;
					particle_data.g_particle_position_size_data[4 * ParticlesCount + 1] = p.pos.y;
					particle_data.g_particle_position_size_data[4 * ParticlesCount + 2] = p.pos.z;

					particle_data.g_particle_position_size_data[4 * ParticlesCount + 3] = p.size;

					particle_data.g_particle_color_data[4 * ParticlesCount + 0] = p.r;
					particle_data.g_particle_color_data[4 * ParticlesCount + 1] = p.g;
					particle_data.g_particle_color_data[4 * ParticlesCount + 2] = p.b;
					particle_data.g_particle_color_data[4 * ParticlesCount + 3] = p.a;
				}
				else
				{
					// Particles that just died will be put at the end of the buffer in SortParticles();
					p.cameradistance = -1.0f;
				}

				ParticlesCount++;
			}
		}
		particle_data.ParticlesCount = ParticlesCount;

		particle_data.SortParticles();
	}

	for (int i = 0; i < particle_data.ParticlesCount; ++i)
	{
		cout << "Particle " << i << " pos: " << particle_data.particles_container[i].pos << endl;
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A5::appLogic()
{
	// Place per frame, application logic here ...
	delta_time = glfwGetTime() - lastTime;
	// cout << "Elapsed time: " << delta_time << endl;
	lastTime = glfwGetTime();

	m_view_proj = m_perpsective * m_view; // find a better place to put this
	generateParticles();
	if (iter == 0)
	{
		cout << "Num particles: " << particle_data.ParticlesCount << endl;
	}
	uploadCommonSceneUniforms();
	// cout << "Num particles: " << particle_data.ParticlesCount << endl;
	iter++;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A5::guiLogic()
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

	if (ImGui::Button("Quit Application"))
	{
		glfwSetWindowShouldClose(m_window, GL_TRUE);
	}

	ImGui::Text("Framerate: %.1f FPS", ImGui::GetIO().Framerate);

	ImGui::End();
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
void A5::draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	{
		glEnable(GL_DEPTH_TEST);
	}

	// Bind the VAO once here, and reuse for all GeometryNode rendering below.
	drawParticles();
	// glBindVertexArray(m_vao_meshData);
	// renderSceneGraph(*m_rootNode);
}
void A5::drawParticles()
{
	m_particleShader.enable();

	// Draw the particles !
	// This draws many times a small triangle_strip (which looks like a quad).
	// This is equivalent to :
	// for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4),
	// but faster.
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particle_data.ParticlesCount);

	// glDisableVertexAttribArray(0);
	// glDisableVertexAttribArray(1);
	// glDisableVertexAttribArray(2);

	m_particleShader.disable();
}
//----------------------------------------------------------------------------------------
void A5::renderSceneGraph(const SceneNode &root)
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
	CHECK_GL_ERRORS;
}

void A5::processSceneNode(SceneNode &node, glm::mat4 parentTransform)
{
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

void A5::renderGeometryNode(GeometryNode &node, mat4 parentTransform)
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

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A5::cleanup()
{
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A5::cursorEnterWindowEvent(
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
bool A5::mouseMoveEvent(
	double xPos,
	double yPos)
{
	bool eventHandled(false);

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A5::mouseButtonInputEvent(
	int button,
	int actions,
	int mods)
{
	bool eventHandled(false);

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A5::mouseScrollEvent(
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
bool A5::windowResizeEvent(
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
bool A5::keyInputEvent(
	int key,
	int action,
	int mods)
{
	bool eventHandled(false);
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
	}
	// Fill in with event handling code...

	return eventHandled;
}
