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

static ParticleAttribs p_sys;
static ParticleAttribs p_clouds;
glm::mat4 island_transform;

int iter = 0;

int node_count = 0;
vector<int> joints;

static vec3 left_hand_pos = vec3(0.0f);
static vec3 center_of_rotation = vec3(0.0f);

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
    // camera_position = vec3(0.0f, 0.0f, 40.0f);
    // camera_position = vec3(-60.0f, 40.0f, 40.0f);
    camera_position = vec3(10, 5, 10);
    // Set the background colour.
    glClearColor(0.32, 0.34, 0.33, 1.0);

    createShaderProgram();
    particle_data.InitParticleSystem(m_particle_shader);
    clouds_data.InitParticleSystem(m_particle_shader);

    glGenVertexArrays(1, &m_vao_meshData);
    enableVertexShaderInputSlots();

    processLuaSceneFile(m_luaSceneFile);

    // Load and decode all .obj files at once here.  You may add additional .obj files to
    // this list in order to support rendering additional mesh types.  All vertex
    // positions, and normals will be extracted and stored within the MeshConsolidator
    // class.
    unique_ptr<MeshConsolidator> meshConsolidator(new MeshConsolidator{
        getAssetFilePath("luffy.obj"),
        getAssetFilePath("onigashima.obj"),
    });

    material_list = meshConsolidator->getMaterialsVector();
    material_ranges = meshConsolidator->getMaterialRangesVector();
    material_map = meshConsolidator->getMaterialMap();

    initMaterialList();

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

    p_sys.velocity = vec3(5.5, 0.1, 0.1);
    p_sys.velocity_variation = vec3(0.1f, 0.1f, 0.1f);
    p_sys.m_color_start = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    p_sys.m_color_end = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    p_sys.size_begin = 0.05f;
    p_sys.size_end = 0.001f;
    p_sys.size_variation = 0.1f;

    p_clouds.velocity = vec3(0.1, 0.1, 0.1);
    p_clouds.velocity_variation = vec3(0.01f, 0.01f, 0.01f);
    p_clouds.m_color_start = vec4(1.0f, 0.8f, 0.8f, 1.0f);
    p_clouds.m_color_end = vec4(1.0f, 0.6f, 0.6f, 1.0f);
    p_clouds.size_begin = 0.01f;
    p_clouds.size_end = 3.0f;
    p_clouds.size_variation = 0.1f;

    clouds_data.is_circle = true;

    particle_system_enabled = false;
    clouds_enabled = true;

    spline_time = 0.0f;
    is_moving_up = true;
}

void A5::initMaterialList()
{
    vector<pair<pair<int, int>, vec3>> temp_mats;
    string objFile = "";
    for (int i = 0; i < material_list->size(); ++i)
    {
        if ((*material_ranges)[i].first == -1)
        {
            if (temp_mats.size() > 0)
            {
                asset_material_map[objFile] = temp_mats;
                cout << "last range: " << temp_mats[temp_mats.size() - 1].first.first << ", " << temp_mats[temp_mats.size() - 1].first.second << endl;
            }
            objFile = (*material_list)[i];
            cout << "objFile: " << objFile << endl;
            temp_mats.clear();
        }
        else
        {
            string mat_name = (*material_list)[i];
            cout << "Mat name: " << mat_name << endl;
            vec3 col = (*material_map)[mat_name];
            pair<int, int> range = (*material_ranges)[i];
            temp_mats.push_back({range, col});
        }
    }
    if (temp_mats.size() > 0 && objFile != "")
    {
        asset_material_map[objFile] = temp_mats;
    }
}

void A5::initializeNodes(SceneNode &node)
{
    int id = node.m_nodeId;
    nodes[id] = &node;
    initial_positions[id] = vec3(node.get_transform()[3]);
    initial_transforms[id] = node.trans;
    if (node.m_name == "Assets/luffy.obj")
    {
        // also rework to add joint node as parent of luffy and onigashima
        // get bounding box for bottom calculation
        vec3 bottom = vec3(node.trans * vec4(0.0f, 0.0f, 0.0f, 1.0f));
        bounce_spline.setUpSpline(6.0f, bottom, 90.0);
    }
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
    m_shader.attachVertexShader(getAssetFilePath("toon.vs").c_str());
    m_shader.attachFragmentShader(getAssetFilePath("toon.fs").c_str());
    m_shader.link();

    m_particle_shader.generateProgramObject();
    m_particle_shader.attachFragmentShader(getAssetFilePath("Particle.fs").c_str());
    m_particle_shader.attachVertexShader(getAssetFilePath("Particle.vs").c_str());
    m_particle_shader.link();
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
    m_view = lookAt(camera_position, vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
}

//----------------------------------------------------------------------------------------
void A5::initLightSources()
{
    // Camera-space position
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

            // --Set background light ambient intensity
            // location = m_shader.getUniformLocation("ambientIntensity");
            // vec3 ambientIntensity = vec3(0.5f, 0.5f, 0.5f);
            // glUniform3fv(location, 1, value_ptr(ambientIntensity));
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

    uploadCommonSceneUniforms();
    if (is_moving_up)
    {
        spline_time += float(delta_time);
        if (spline_time >= 1.0f)
        {
            spline_time = 1.0f;
            is_moving_up = false;
        }
    }
    else
    {
        spline_time -= float(delta_time);
        if (spline_time <= 0.0f)
        {
            spline_time = 0.0f;
            is_moving_up = true;
        }
    }
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
    if (node.m_name == "left_hand")
    {
        left_hand_pos = vec3(trans * vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }
    else if (node.m_name == "onigashima")
    {
        center_of_rotation = vec3(trans * vec4(0.0f, 0.0f, 0.0f, 1.0f));
        island_transform = trans;
    }
    shader.enable();
    {
        //-- Set ModelView matrix:
        {
            GLint location = shader.getUniformLocation("ModelView");
            mat4 modelView = viewMatrix * trans;
            glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));
            CHECK_GL_ERRORS;

            //-- Set NormMatrix:
            location = shader.getUniformLocation("NormalMatrix");
            mat3 normalMatrix = glm::transpose(glm::inverse(mat3(modelView)));
            glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));
            CHECK_GL_ERRORS;

            //-- Set Material values:
            // location = shader.getUniformLocation("material.kd");
            // vec3 kd = node.material.kd;
            // if (parent_joint_selected)
            // {
            //     kd = vec3(1.0f, 1.0f, 0.0f);
            // }
            // glUniform3fv(location, 1, value_ptr(kd));
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
    // set background color
    glClearColor(0.11, 0.21, 0.72, 1.0);
    // Bind the VAO once here, and reuse for all GeometryNode rendering below.
    {
        glBindVertexArray(m_vao_meshData);
        renderSceneGraph(*m_rootNode);
        glBindVertexArray(0);
    }
    {
        m_view_proj = m_perpsective * m_view; // find a better place to put this
        // particle_data.UpdateParticles(float(delta_time), m_view_proj);
        clouds_data.UpdateParticles(float(delta_time), m_view_proj);

        p_sys.m_position = left_hand_pos;
        clouds_data.center_of_rotation = center_of_rotation;
        if (particle_system_enabled)
        {
            for (int i = 0; i < 10; ++i)
            {
                particle_data.EmitParticle(p_sys);
            }
        }
        if (clouds_enabled && iter++ % 200 == 0)
        {
            // generate a set of particles around circle
            int num_points = 64;
            // use similar logic for rotating camera around object by updating its position based on circle
            for (int i = 0; i < num_points; ++i)
            {
                float angle = (2 * glm::pi<float>() / num_points) * i;
                float x = cos(angle);
                float z = sin(angle);
                p_clouds.m_position = vec3(island_transform * vec4(x, -0.1f, z, 1.0f));
                for (int j = 0; j < 1; ++j)
                {
                    clouds_data.EmitParticle(p_clouds);
                }
            }
        }

        // particle_data.DrawParticles();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        clouds_data.DrawParticles();
        glDisable(GL_BLEND);

        CHECK_GL_ERRORS;
    }
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
    // cout << "node name: " << node.m_name << ", ID: " << node.m_nodeId << endl;
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
    mat4 node_trans = node.trans;
    if (node.m_name == "Assets/luffy.obj")
    {
        vec3 pos = bounce_spline.getPos(spline_time);
        float rotation_angle = bounce_spline.getRotation(spline_time);
        // node_trans = glm::translate(node_trans, pos);
        // node_trans = glm::rotate(node_trans, (radians)(rotation_angle), vec3(1.0f, 0.0f, 0.0f));
    }
    SceneNode *parent = node.parent;
    updateShaderUniforms(m_shader,
                         node,
                         m_view,
                         parentTransform * node_trans,
                         (parent->m_nodeType == NodeType::JointNode && selected[parent->m_nodeId]));

    BatchInfo batchInfo = m_batchInfoMap.at(node.meshId);

    //-- Now render the mesh:
    m_shader.enable();
    {
        string key = "./" + node.m_name;
        vector<pair<pair<int, int>, vec3>>
            mats = asset_material_map[key];
        for (auto mat_pair : mats)
        {
            pair<int, int> range = mat_pair.first;
            vec3 col = mat_pair.second;
            GLint location = m_shader.getUniformLocation("color");
            glUniform3fv(location, 1, value_ptr(col));
            glDrawArrays(GL_TRIANGLES, batchInfo.startIndex + range.first, range.second - range.first + 1);
        }
    }
    // glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
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
    if (button == 0)
    {
        // left click
        if (actions == 1)
        {
            particle_system_enabled = true;
        }
        // left release
        if (actions == 0)
        {
            particle_system_enabled = false;
        }
    }
    eventHandled = true;
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
        // manipulate camera with wasd + space + shift
        if (eventHandled)
        {
            return eventHandled;
        }
        float camera_speed = 5.0f;
        if (key == GLFW_KEY_W)
        {
            camera_position += vec3(0.0f, 0.0f, -camera_speed);
            initViewMatrix();
            eventHandled = true;
        }
        if (key == GLFW_KEY_S)
        {
            camera_position += vec3(0.0f, 0.0f, camera_speed);
            initViewMatrix();
            eventHandled = true;
        }
        if (key == GLFW_KEY_A)
        {
            camera_position += vec3(-camera_speed, 0.0f, 0.0f);
            initViewMatrix();
            eventHandled = true;
        }
        if (key == GLFW_KEY_D)
        {
            camera_position += vec3(camera_speed, 0.0f, 0.0f);
            initViewMatrix();
            eventHandled = true;
        }
        if (key == GLFW_KEY_SPACE)
        {
            camera_position += vec3(0.0f, camera_speed, 0.0f);
            initViewMatrix();
            eventHandled = true;
        }
        if (key == GLFW_KEY_LEFT_SHIFT)
        {
            camera_position += vec3(0.0f, -camera_speed, 0.0f);
            initViewMatrix();
            eventHandled = true;
        }
        if (eventHandled)
        {
            cout << "camera_position: " << camera_position << endl;
        }
    }
    return eventHandled;
}
