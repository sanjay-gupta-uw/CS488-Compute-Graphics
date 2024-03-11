// Termm--Fall 2020/

#include <glm/ext.hpp>

#include "A4.hpp"

using namespace std;
using namespace glm;

mat4 T1;
mat4 S2;
mat4 R3;
mat4 T4;
// distance from the eye to the view plane
float d = 10.0;
int nx = 0;
int ny = 0;
unsigned int idx = 0;
vector<GeometryNode *> geo_nodes;
vector<PhongMaterial *> mat_nodes;
vector<Primitive *> prim_nodes;
vec3 eye;
list<Light *> lights;
vec3 ambient;
const int HIT_THRESHOLD = 10;
const vec3 BACKGROUND = vec3(0.2, 0.2, 0.1);
// const vec3 BACKGROUND = vec3(0.0, 0.0, 0.0);
const float EPSILON = 1e-4f;

unsigned int NUM_NODES = 0;
unsigned int NUM_LIGHTS = 0;

const int SPHERE = 0;
const int CUBE = 1;
const int NONHIER_SPHERE = 2;
const int NONHIER_BOX = 3;

void A4_Render(
	// What to render
	SceneNode *root,

	// Image to write to, set to a given width and height
	Image &image,

	// Viewing parameters
	const vec3 &eye_,
	const vec3 &view,
	const vec3 &up,
	double fovy,

	// Lighting parameters
	const vec3 &ambient_,
	const list<Light *> &lights_)
{
	eye = eye_;
	lights = lights_;
	ambient = ambient_;

	// Fill in raytracing code here...
	assign_indices(root);
	NUM_NODES = geo_nodes.size();
	NUM_LIGHTS = lights.size();
	initialize_pixel_transformations(image, fovy, eye, view, up);
	compute_rays(root, eye, image);

	cout
		<< "F20: Calling A4_Render(\n"
		<< "\t" << *root << "\t"
		<< "Image(width:" << image.width() << ", height:" << image.height() << ")\n"
																			   "\t"
		<< "eye:  " << to_string(eye) << endl
		<< "\t"
		<< "view: " << to_string(view) << endl
		<< "\t"
		<< "up:   " << to_string(up) << endl
		<< "\t"
		<< "fovy: " << fovy << endl
		<< "\t"
		<< "ambient: " << to_string(ambient) << endl
		<< "\t"
		<< "lights{" << endl;

	for (const Light *light : lights)
	{
		cout << "\t\t" << *light << endl;
	}
	cout << "\t}" << endl;
	cout << ")" << endl;
	size_t h = image.height();
	size_t w = image.width();
}

void assign_indices(SceneNode *node)
{
	if (node->m_nodeType == NodeType::GeometryNode)
	{
		// cout << "assigning index to node: " << node->m_name << endl;
		GeometryNode *geometryNode = static_cast<GeometryNode *>(node);
		geometryNode->index = idx;
		idx++;
		geo_nodes.push_back(geometryNode);
		PhongMaterial *mat = static_cast<PhongMaterial *>(geometryNode->m_material);
		mat_nodes.push_back(mat);
		Primitive *primitive = static_cast<Primitive *>(geometryNode->m_primitive);
		prim_nodes.push_back(primitive);
		// cout << "primitive type: " << primitive->type << endl;
		// cout << endl;
	}
	for (SceneNode *child : node->children)
	{
		assign_indices(child);
	}
}

void initialize_pixel_transformations(Image &image, double fov, vec3 eye, vec3 view, vec3 up)
{
	nx = image.width();
	ny = image.height();

	T1 = translate(mat4(1.0f), vec3(-nx / 2, -ny / 2, d));
	float height = 2 * d * tan(fov / 2);
	float width = height * nx / ny;
	S2 = scale(mat4(1.0f), vec3(-height / ny, width / nx, 1));
	vec3 w = normalize(view - eye);
	vec3 u = normalize(cross(up, w));
	vec3 v = cross(w, u);
	R3 = mat4(
		vec4(u, 0),
		vec4(v, 0),
		vec4(w, 0),
		vec4(0, 0, 0, 1));
	T4 = translate(mat4(1.0f), eye);
}

vec4 pixel_to_world(uint x, uint y)
{
	vec4 p_world = T4 * R3 * S2 * T1 * vec4(x, y, 0, 1);
	return p_world;
}

void compute_rays(SceneNode *root, const vec3 &eye, Image &image)
{
	size_t h = image.height();
	size_t w = image.width();
	for (uint y = 0; y < h; ++y)
	{
		for (uint x = 0; x < w; ++x)
		{
			vec4 p_world = pixel_to_world(x, y);
			vec3 ray = normalize(vec3(p_world) - eye);
			Ray r = {eye, ray, 1.0f / ray};
			vec3 col = ray_color(r, 0);

			float correction = 1.0f;
			image(x, y, 0) = col.x / correction;
			image(x, y, 1) = col.y / correction;
			image(x, y, 2) = col.z / correction;

			// image(x, y, 0) = pow(col.x, 1 / 2.2); // Gamma correction
			// image(x, y, 1) = pow(col.y, 1 / 2.2); // Assuming a gamma value of 2.2
			// image(x, y, 2) = pow(col.z, 1 / 2.2);
		}
	}
}

static float inline min(float a, float b)
{
	return (a < b) ? a : b;
}
static float inline max(float a, float b)
{
	return (a > b) ? a : b;
}

bool intersection(const Ray ray_obj, double *t, vec3 *point, vec3 *normal, PhongMaterial *&mat)
{
	const vec3 ray = ray_obj.direction;
	cout << "ray: " << to_string(ray) << endl;
	// NH-sphere A value (quad form) -- only compute if spheres are present
	float A = dot(ray, ray);
	float B;
	float C;
	int closest_node = -1;
	int closest_type = -1;
	Primitive *prim;

	for (int i = 0; i < NUM_NODES; ++i)
	{
		prim = prim_nodes[i];
		switch (prim->type)
		{
		case SPHERE:
			// code for Sphere
			break;
		case CUBE:
			// code for Cube
			break;
		case NONHIER_SPHERE:
		{
			NonhierSphere *nh_sphere = static_cast<NonhierSphere *>(prim);
			B = 2.0f * dot(ray, (*point - nh_sphere->m_pos));
			C = dot(*point - nh_sphere->m_pos, *point - nh_sphere->m_pos) - pow(nh_sphere->m_radius, 2);

			double roots[2] = {-1.0, -1.0};
			int ret = quadraticRoots(A, B, C, roots);
			if (ret > 0)
			{

				if (roots[0] < 0 && roots[1] < 0)
				{
					continue;
				}
				double close_root;
				if (roots[0] < 0)
				{
					close_root = roots[1];
				}
				else if (roots[1] < 0)
				{
					close_root = roots[0];
				}
				else
				{
					close_root = (roots[0] < roots[1]) ? roots[0] : roots[1];
				}

				if (*t == -1.0 || close_root < *t)
				{
					*t = close_root;
					closest_node = i;
					closest_type = NONHIER_SPHERE;
					// update normal
				}
			}
			break;
		}
		break;
		case NONHIER_BOX:
		{
			// https: // tavianator.com/2022/ray_box_boundary.html#:~:text=The%20slab%20method%20tests%20for,intersects%20the%20box%2C%20if%20any.&text=t%20%3D%20x%20%E2%88%92%20x%200%20x%20d%20.
			NonhierBox *nh_box = static_cast<NonhierBox *>(prim);
			// precompute box min and max
			vec3 box_min = nh_box->m_pos;
			vec3 box_max = nh_box->m_pos + vec3(nh_box->m_size);
			vec3 t0 = (box_min - *point) * ray_obj.inv_dir;
			vec3 t1 = (box_max - *point) * ray_obj.inv_dir;
			vec3 tmin = min(t0, t1);
			vec3 tmax = max(t0, t1);

			float t_near = max(tmin.x, max(tmin.y, tmin.z));
			float t_far = min(tmax.x, min(tmax.y, tmax.z));

			if (t_near > t_far || (t_far < 0 && t_near < 0))
			{
				continue;
			}
			float t_temp = min(max(t_near, 0.0f), max(t_far, 0.0f));
			if (*t == -1.0 || t_temp < *t)
			{
				*t = t_temp;
				closest_node = i;
				closest_type = NONHIER_BOX;
			}
		}
		default:
			// code for unknown type (maybe mesh or other types of primitives)
			break;
		}
	}
	bool intersect = false;
	if (closest_node != -1)
	{
		switch (closest_type)
		{
		case NONHIER_SPHERE:
		{
			NonhierSphere *nh_sphere = static_cast<NonhierSphere *>(prim_nodes[closest_node]);
			*normal = normalize(2.0f * (*point - nh_sphere->m_pos));
			*point = *point + (*t * ray);
			mat = mat_nodes[closest_node];
			intersect = true;
			break;
		}
		case NONHIER_BOX:
		{
			NonhierBox *nh_box = static_cast<NonhierBox *>(prim_nodes[closest_node]);
			*normal = vec3(0.0);
			for (int i = 0; i < 3; ++i)
			{
				if (abs((*point)[i] - nh_box->m_pos[i]) < EPSILON)
				{
					(*normal)[i] = -1.0;
				}
				else if (abs((*point)[i] - (nh_box->m_pos[i] + nh_box->m_size)) < EPSILON)
				{
					(*normal)[i] = 1.0;
				}
			}
			*point = *point + (*t * ray);
			mat = mat_nodes[closest_node];
			intersect = true;
			break;
		}
		default:
			break;
		}
	}
	return intersect;
}
vec3 cast_shadow_ray(const vec3 point, const vec3 normal)
{
	vec3 light_accumulated = vec3(0.0);
	vec3 norm;
	PhongMaterial *mat;
	double t = -1.0;
	Light *light;
	vec3 potential_point;
	float distance;
	vec3 shadow_ray;
	for (list<Light *>::iterator it = lights.begin(); it != lights.end(); ++it)
	{
		light = *it;
		distance = length(light->position - point);
		potential_point = point + (EPSILON * normal);
		shadow_ray = normalize(light->position - potential_point);
		Ray shadow_ray_obj = {potential_point, shadow_ray, 1.0f / shadow_ray};

		if (intersection(shadow_ray_obj, &t, &potential_point, &norm, mat) && (t > 0 && t < distance))
		{
			continue;
		}
		float attenuation = 1 / (light->falloff[0] + light->falloff[1] * distance + light->falloff[2] * pow(distance, 2));
		light_accumulated = light_accumulated + (attenuation * light->colour * glm::max(dot(normal, shadow_ray), 0.0f));
	}
	return min(light_accumulated, vec3(1.0));
}

vec3 get_reflected_ray(vec3 ray, vec3 normal)
{
	// lesson 12
	return normalize(ray - 2 * normal * dot(ray, normal));
}

vec3 ray_color(Ray ray_obj, int hits)
{
	vec3 col;
	double t = -1.0;
	vec3 normal;
	PhongMaterial *mat;
	// mat, point and normal are updated
	vec3 point;
	if (intersection(ray_obj, &t, &point, &normal, mat))
	{
		col = mat->m_kd * ambient;
		if (mat->m_kd != vec3(0.0)) // diffuse material
		{
			col = col + mat->m_kd * cast_shadow_ray(point, normal);
		}
		// need to update this if first obj is actually light
		if ((mat->m_ks != vec3(0.0)) && (hits < HIT_THRESHOLD)) // specular material
		{
			hits = hits + 1;
			vec3 reflected_ray = get_reflected_ray(ray_obj.direction, normal);
			// if (ray_is_lit(point, reflected_ray))
			{
				// this should include shininess
				Ray reflected_ray_obj = {point, reflected_ray, 1.0f / reflected_ray};
				col = col + mat->m_ks * ray_color(reflected_ray_obj, hits);
			}
		}
		return col;
	}
	// what about pixels that hit the light source itself? -- wont this be a single point?
	// represent light source as a sphere and material looks like a light source
	return BACKGROUND;
}
