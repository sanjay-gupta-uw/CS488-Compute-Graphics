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
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A5::appLogic()
{
 // Place per frame, application logic here ...
 delta_time = glfwGetTime() - lastTime;
 lastTime = glfwGetTime();

 m_view_proj = m_perpsective * m_view; // find a better place to put this
 for (int i = 0; i < 10; ++i)
 {
  particle_data.EmitParticle();
 }
 particle_data.UpdateParticles(delta_time, m_view_proj);
 uploadCommonSceneUniforms();
 // cout << "Num particles: " << particle_data.ParticlesCount << endl;
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
 particle_data.DrawParticles();
 // glBindVertexArray(m_vao_meshData);
 // renderSceneGraph(*m_rootNode);
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
