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
#include <vector>

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

void initialize_pixel_transformations(Image &image, double fov, glm::vec3 eye, glm::vec3 view, glm::vec3 up);
void assign_indices(SceneNode *node);
void compute_rays(SceneNode *root, const glm::vec3 &eye, Image &image);
glm::vec4 pixel_to_world(uint x, uint y);
bool intersection(glm::vec3 ray, double *t, glm::vec3 *point, glm::vec3 *normal, PhongMaterial *&mat);
glm::vec3 cast_shadow_ray(const glm::vec3 pos, const glm::vec3 normal);
glm::vec3 get_reflected_ray(glm::vec3 ray, glm::vec3 normal);
glm::vec3 ray_color(glm::vec3 ray, glm::vec3 point, int MAX_HITS);
bool ray_is_lit(const glm::vec3 point, glm::vec3 ray);
// vec3 ray_color(SceneNode *node, glm::vec3 eye, glm::vec3 ray, const std::list<Light *> &lights, const vec3 &ambient, int depth);