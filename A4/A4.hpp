// Termm--Fall 2020

#pragma once

#include <glm/glm.hpp>

#include "SceneNode.hpp"
#include "Light.hpp"
#include "Image.hpp"
#include "GeometryNode.hpp"
#include "Primitive.hpp"
#include "Polyroots.hpp"
#include "Material.hpp"
#include "PhongMaterial.hpp"
#include "Light.hpp"
#include "Mesh.hpp"
#include <vector>

struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;
	float dist;
};

void A4_Render(
	// What to render
	SceneNode *root,

	// Image to write to, set to a given width and height
	Image &image,

	// Viewing parameters
	const glm::vec3 &eye,
	const glm::vec3 &view,
	const glm::vec3 &up,
	double fovy,

	// Lighting parameters
	const glm::vec3 &ambient,
	const std::list<Light *> &lights);

void initialize_pixel_transformations(Image &image, double fov, glm::vec3 view, glm::vec3 up);
void assign_indices(SceneNode *node);
void update_transforms(SceneNode *node, SceneNode *parent);
void compute_rays(SceneNode *root, Image &image);
glm::vec4 pixel_to_world(float x, float y);
bool intersection(const Ray ray_data, double *t, glm::vec3 *intersection_point, glm::vec3 *normal, PhongMaterial *&mat);
glm::vec3 cast_shadow_ray(const glm::vec3 *pos, const glm::vec3 *normal, PhongMaterial *&mat);
glm::vec3 ray_color(const Ray ray_data, int hits);