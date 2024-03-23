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

struct RayData
{
	glm::vec3 origin;
	glm::vec3 direction;
};

struct IntersectionData
{
	double t = std::numeric_limits<double>::infinity();
	glm::vec3 normal;
	PhongMaterial *mat;
	glm::vec3 intersection_point;
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
void update_transforms(SceneNode *node, glm::mat4 parent);
void compute_rays(SceneNode *root, Image &image);
glm::vec4 pixel_to_world(float x, float y);
bool intersection(const RayData ray_data, double t0, double t1, IntersectionData &intersect_data);
glm::vec3 ray_color(const RayData ray_data, double t0, double t1, int hits);
// glm::vec3 cast_shadow_ray(const glm::vec3 *pos, const glm::vec3 *normal, PhongMaterial *&mat);
// vec3 cast_shadow_ray(const vec3 *point, const vec3 *normal, PhongMaterial *&mat)
// {
// 	vec3 light_accumulated = vec3(0.0);
// 	vec3 norm = vec3(0.0);
// 	PhongMaterial *mat_tmp;

// 	for (Light *light : lights)
// 	{
// 		double t = std::numeric_limits<double>::infinity();
// 		vec3 potential_point = *point + (EPSILON * *normal);
// 		vec3 shadow_ray_vec = light->position - potential_point;
// 		Ray shadow_ray_data = {potential_point, normalize(shadow_ray_vec), length(shadow_ray_vec)};

// 		float distance = length(light->position - *point);
// 		if (!intersection(shadow_ray_data, &t, &potential_point, &norm, mat_tmp) || t > distance || t < 0.0)
// 		{
// 			float attenuation = 1 / (light->falloff[0] + light->falloff[1] * distance + light->falloff[2] * pow(distance, 2));
// 			vec3 l = normalize(light->position - *point);
// 			vec3 diffuse = mat->m_kd * light->colour * max(dot(*normal, l), 0.0f);
// 			light_accumulated += attenuation * (diffuse);
// 		}
// 	}
// 	return min(light_accumulated, vec3(1.0));
// }
