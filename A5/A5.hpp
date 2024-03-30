// Termm-Fall 2020

#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"

#include "SceneNode.hpp"
#include "GeometryNode.hpp"
#include "JointNode.hpp"

#include <glm/glm.hpp>
#include <memory>
#include "ParticleSystem.hpp"
#include "HermiteSpline.hpp"
#include <unordered_map>

struct LightSource
{
    glm::vec3 position;
    glm::vec3 rgbIntensity;
};

class A5 : public CS488Window
{
public:
    A5(const std::string &luaSceneFile);
    virtual ~A5();

protected:
    virtual void init() override;
    virtual void appLogic() override;
    virtual void guiLogic() override;
    virtual void draw() override;
    virtual void cleanup() override;

    //-- Virtual callback methods
    virtual bool cursorEnterWindowEvent(int entered) override;
    virtual bool mouseMoveEvent(double xPos, double yPos) override;
    virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
    virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
    virtual bool windowResizeEvent(int width, int height) override;
    virtual bool keyInputEvent(int key, int action, int mods) override;

    //-- One time initialization methods:
    void processLuaSceneFile(const std::string &filename);
    void createShaderProgram();
    void enableVertexShaderInputSlots();
    void uploadVertexDataToVbos(const MeshConsolidator &meshConsolidator);
    void mapVboDataToVertexShaderInputLocations();
    void initViewMatrix();
    void initLightSources();

    void initPerspectiveMatrix();
    void uploadCommonSceneUniforms();

    void renderSceneGraph(const SceneNode &node);
    void processSceneNode(SceneNode &node, glm::mat4 parentTransform);
    void renderGeometryNode(GeometryNode &node, glm::mat4 parentTransform);

    glm::mat4 m_perpsective;
    glm::mat4 m_view;
    glm::vec3 camera_position;

    glm::mat4 m_view_proj;

    LightSource m_light;

    //-- GL resources for mesh geometry data:
    GLuint m_vao_meshData;
    GLuint m_vbo_vertexPositions;
    GLuint m_vbo_vertexNormals;
    GLint m_positionAttribLocation;
    GLint m_normalAttribLocation;
    ShaderProgram m_shader;
    ShaderProgram m_particle_shader;

    // BatchInfoMap is an associative container that maps a unique MeshId to a BatchInfo
    // object. Each BatchInfo object contains an index offset and the number of indices
    // required to render the mesh with identifier MeshId.
    BatchInfoMap m_batchInfoMap;

    std::string m_luaSceneFile;

    std::shared_ptr<SceneNode> m_rootNode;
    std::vector<bool> selected;
    void initializeNodes(SceneNode &node);
    std::vector<SceneNode *> nodes;
    std::vector<glm::vec3> initial_positions;
    std::vector<float> initial_angles_x;
    std::vector<float> initial_angles_y;
    std::vector<glm::mat4> initial_transforms;
    std::vector<glm::mat4> initial_transforms_unscaled;

    std::vector<std::string> *material_list;
    std::vector<std::pair<int, int>> *material_ranges;
    // mapping for each material file
    std::unordered_map<std::string, glm::vec3> *material_map;

    void initMaterialList();

    // asset to material mapping, vector contains the colouring for each face
    std::unordered_map<std::string, std::vector<std::pair<std::pair<int, int>, glm::vec3>>> asset_material_map;

    // particle system
    ParticleSystem particle_data;
    bool particle_system_enabled;

    ParticleSystem clouds_data;
    bool clouds_enabled;

    double delta_time;
    glm::vec3 light_cspace;

    // spline
    HermiteSpline bounce_spline;
    float spline_time;
    bool is_moving_up;
};
