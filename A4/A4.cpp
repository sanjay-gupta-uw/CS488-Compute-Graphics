// Termm--Fall 2020/

#include <glm/ext.hpp>

#include "A4.hpp"

using namespace std;
using namespace glm;

mat4 T1;
mat4 S2;
mat4 R3;
mat4 T4;
// distance from the eye_pos to the view plane
float nx = 0.0f;
float ny = 0.0f;
unsigned int idx = 0;
vector<GeometryNode *> geo_nodes;
vector<PhongMaterial *> mat_nodes;
vector<Primitive *> prim_nodes;
vec3 eye_pos;
list<Light *> lights;
vec3 ambient;
const int HIT_THRESHOLD = 10;
const vec3 BACKGROUND = vec3(0.2, 0.2, 0.1);
// const vec3 BACKGROUND = vec3(0.0, 0.0, 0.0);
const float EPSILON = 1e-2f;
const float EPSILON_T = 1e-2f;

unsigned int NUM_NODES = 0;
unsigned int NUM_LIGHTS = 0;

const int MESH = 0;
const int SPHERE = 1;
const int CUBE = 2;
const int NONHIER_SPHERE = 3;
const int NONHIER_BOX = 4;
int hit_count = 0;

vec3 N;
vec3 P_PLANE;

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
	eye_pos = eye_;
	lights = lights_;
	ambient = ambient_;

	// Fill in raytracing code here...
	assign_indices(root);
	update_transforms(root, nullptr);
	// exit(0);
	NUM_NODES = geo_nodes.size();
	NUM_LIGHTS = lights.size();
	initialize_pixel_transformations(image, fovy, view, up);
	compute_rays(root, image);

	cout
		<< "F20: Calling A4_Render(\n"
		<< "\t" << *root << "\t"
		<< "Image(width:" << image.width() << ", height:" << image.height() << ")\n"
																			   "\t"
		<< "eye_pos:  " << to_string(eye_pos) << endl
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
		// cout << "\t\t" << *light << endl;
	}
	// cout << "\t}" << endl;
	// cout << ")" << endl;

	cout << "Hit count: " << hit_count << endl;
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

void update_transforms(SceneNode *node, SceneNode *parent)
{
	// check if NH object
	if (node->index != -1)
	{
		int type = prim_nodes[node->index]->type;
		if (type == NONHIER_SPHERE)
		{
			NonhierSphere *nh_sphere = static_cast<NonhierSphere *>(prim_nodes[node->index]);
			node->translation_matrix = translate(mat4(1.0f), vec3(nh_sphere->m_pos));
			node->scale(vec3(nh_sphere->m_radius));
		}
		else if (type == NONHIER_BOX)
		{
			NonhierBox *nh_box = static_cast<NonhierBox *>(prim_nodes[node->index]);
			node->translation_matrix = translate(mat4(1.0f), vec3(nh_box->m_pos));
			node->scale(vec3(nh_box->m_size));
		}

		if (type == NONHIER_BOX || type == CUBE)
		{
			node->translate(vec3(0.5f, 0.5f, 0.5f));
		}
	}
	// updates each node based on all transformation
	bool is_root = parent == nullptr;
	// dont pass scales to children
	// node->scale(is_root ? vec3(1.0f) : parent->scale_amount);
	node->rotation_matrix = is_root ? node->rotation_matrix : parent->rotation_matrix * node->rotation_matrix;
	node->translation_matrix = is_root ? node->translation_matrix : parent->translation_matrix * node->translation_matrix;

	node->set_transform(node->translation_matrix * node->rotation_matrix * node->scale_matrix);
	cout << "node: " << node->m_name << " transform: " << to_string(node->get_transform()) << endl;
	cout << node->m_name << " rotation_matrix: " << to_string(node->rotation_matrix) << endl;

	if (node->index != -1 && prim_nodes[node->index]->type == MESH)
	{
		Mesh *mesh = static_cast<Mesh *>(prim_nodes[node->index]);
		mat4 scaling = node->scale_matrix;
		mat4 rot = node->rotation_matrix;
		mat4 translation = node->translation_matrix;

		vec3 bv_scale = mesh->b_box_max - mesh->b_box_min;
		vec3 bv_translation = mesh->b_box_max - (vec3(0.5) * bv_scale);

		mesh->setTransform(translate(mat4(1.0f), bv_translation) * scale(mat4(1.0f), bv_scale));
		node->set_transform((translation * mat4(translate(mat4(1.0f), bv_translation)) * rot * (scaling * scale(mat4(1.0f), bv_scale))));
	}
	for (SceneNode *child : node->children)
	{
		update_transforms(child, node);
	}
}

void initialize_pixel_transformations(Image &image, double fov, vec3 view, vec3 up)
{
	float d = 0.011f;
	nx = float(image.width());
	ny = float(image.height());

	T1 = translate(mat4(1.0f), vec3(-nx / 2.0f, -ny / 2.0f, d));
	float height = 2.0f * d * tan(glm::radians(fov / 2.0f));
	float width = height * nx / ny;
	S2 = scale(mat4(1.0f), vec3(-height / ny, -width / nx, 1.0f));
	vec3 w = normalize(view - eye_pos);
	vec3 u = normalize(cross(up, w));
	vec3 v = cross(w, u);
	R3 = mat4(vec4(u, 0.0f),
			  vec4(v, 0.0f),
			  vec4(w, 0.0f),
			  vec4(0.0f, 0.0f, 0.0f, 1.0f));
	T4 = translate(mat4(1.0f), eye_pos);
	// // cout << "S2: " << to_string(S2) << endl;
	// // cout << "w: " << to_string(w) << endl;
	// // cout << "u: " << to_string(u) << endl;
	// // cout << "v: " << to_string(v) << endl;
	// // cout << "R3: " << to_string(R3) << endl;
	// // cout << "T4: " << to_string(T4) << endl;
	N = normalize(view);
	P_PLANE = eye_pos + d * N;
}

static float inline min(float a, float b)
{
	return (a < b) ? a : b;
}
static float inline max(float a, float b)
{
	return (a > b) ? a : b;
}

static bool inline intersection_visible(vec3 intersection_point)
{
	float distance = dot(N, intersection_point - P_PLANE);
	return distance > 0;
}

void compute_rays(SceneNode *root, Image &image)
{
	size_t h = image.height();
	size_t w = image.width();

	int c = 0;
	double progress = 0.0;
	for (uint y = 0; y < h; ++y)
	{
		for (uint x = 0; x < w; ++x)
		{
			// if (x != 301 || y != 210)
			// {
			// 	image(x, y, 0) = 0.0;
			// 	image(x, y, 1) = 0.0;
			// 	image(x, y, 2) = 0.0;
			// 	continue;
			// }
			// cout << "x: " << x << " y: " << y << endl;
			vec4 p_world = T4 * R3 * S2 * T1 * vec4(float(x) + 0.5f, float(y) + 0.05f, 0.0f, 1.0f);
			vec3 ray_vec = vec3(p_world) - eye_pos;
			Ray ray_data = {eye_pos, normalize(ray_vec), length(ray_vec)};

			vec3 col = ray_color(ray_data, 0);
			// if (col != BACKGROUND)
			// {
			// 	// self intersection check for when 1 obj is in the scene
			// 	c += 1;
			// 	cout << "x: " << x << " y: " << y << " col: " << to_string(col) << endl;
			// 	cout << "col: " << to_string(col) << endl;
			// }
			// col = vec3(1.0);
			image(x, y, 0) = col.x;
			image(x, y, 1) = col.y;
			image(x, y, 2) = col.z;
			// exit(0);
		}
		progress = ((double)y / (double)h) * 100.0;
		cout << "\rProgress: " << progress << " %" << std::flush;
	}
	// cout << "c: " << c << endl;
}

static double inline sphere_intersection(const vec3 *ray_pos,
										 const vec3 *ray_dir,
										 mat4 *inv,
										 vec3 *normal)
{
	// change this to be more like cube with conversions
	mat4 inv_prime = *inv;
	inv_prime[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	vec3 local_ray_pos = vec3(*inv * vec4(*ray_pos, 1.0f));
	vec3 local_ray_dir = normalize(vec3(inv_prime * vec4(*ray_dir, 0.0f)));

	double A = dot(local_ray_dir, local_ray_dir);
	double B = 2.0f * dot(local_ray_dir, local_ray_pos);
	double C = dot(local_ray_pos, local_ray_pos) - 1;

	double roots[2] = {-1.0, -1.0};
	double close_root = {-1.0};
	int ret = quadraticRoots(A, B, C, roots);
	if (ret > 0)
	{
		if (roots[0] > EPSILON && roots[1] > EPSILON)
			close_root = std::min(roots[0], roots[1]);
		else if (roots[0] > EPSILON)
			close_root = roots[0];
		else if (roots[1] > EPSILON)
			close_root = roots[1];

		vec3 local_normal = normalize(local_ray_pos + (close_root * local_ray_dir));
		*normal = normalize(transpose(mat3(*inv)) * local_normal);
		return dot(*ray_dir, vec3(inverse(*inv) * vec4(close_root * local_ray_dir, 0.0f)));
	}
	return -1.0;
}

static bool inline box_intersection(Ray ray_data, mat4 transformation, double *t, vec3 *normal)
{
	struct face
	{
		vec3 normal;
		vec3 point;
	};

	const mat3 normal_matrix = transpose(inverse(mat3(transformation)));
	vec3 min_b = vec3(transformation * vec4(vec3(-0.5, -0.5, -0.5), 1.0));
	vec3 max_b = vec3(transformation * vec4(vec3(0.5, 0.5, 0.5), 1.0));
	// cout << "min_b: " << to_string(min_b) << " max_b: " << to_string(max_b) << endl;

	face faces[6] = {
		{vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 0.5)},
		{vec3(0.0, 0.0, -1.0), vec3(0.0, 0.0, -0.5)},
		{vec3(0.0, 1.0, 0.0), vec3(0.0, 0.5, 0.0)},
		{vec3(0.0, -1.0, 0.0), vec3(0.0, -0.5, 0.0)},
		{vec3(1.0, 0.0, 0.0), vec3(0.5, 0.0, 0.0)},
		{vec3(-1.0, 0.0, 0.0), vec3(-0.5, 0.0, 0.0)}};
	// vec3 center = rot_scale * (0.0, 0.0, 0.0);
	bool has_intersection = false;

	for (int i = 0; i < 6; ++i)
	{
		faces[i].point = vec3(transformation * vec4(faces[i].point, 1.0f));
		faces[i].normal = normalize(normal_matrix * faces[i].normal);
		double denom = dot(ray_data.direction, faces[i].normal);
		if (abs(denom) > EPSILON)
		{
			double t_temp = dot(faces[i].point - ray_data.origin, faces[i].normal) / denom;
			if (t_temp > 0 && t_temp < *t)
			{
				// Check if the intersection is within the bounds of the face
				vec3 intersection_point = ray_data.origin + (float)t_temp * ray_data.direction;
				vec3 local_intersection = vec3(inverse(transformation) * vec4(intersection_point, 1.0f));

				// Compute bounds in local box space to check if intersection is within the face
				float halfX = 0.5f, halfY = 0.5f, halfZ = 0.5f;
				if (abs(local_intersection.x) <= halfX + EPSILON &&
					abs(local_intersection.y) <= halfY + EPSILON &&
					abs(local_intersection.z) <= halfZ + EPSILON)
				{
					*t = t_temp; // Update closest intersection distance
					*normal = faces[i].normal;
					has_intersection = true;
				}
			}
		}
	}
	return has_intersection;
}

bool intersection(const Ray ray_data, double *t, glm::vec3 *intersection_point, glm::vec3 *normal, PhongMaterial *&mat)
{
	// NH-sphere A value (quad form) -- only compute if spheres are present
	const vec3 ray = ray_data.direction;
	const vec3 ray_origin = ray_data.origin;
	int closest_node = -1;
	int closest_type = -1;
	*t = std::numeric_limits<double>::infinity();
	bool hit = false;
	Primitive *prim;
	mat4 transform;

	for (int node_index = 0; node_index < NUM_NODES; ++node_index)
	{
		transform = geo_nodes[node_index]->get_transform();
		prim = prim_nodes[node_index];
		// if (prim->type != MESH)
		// 	continue;
		switch (prim->type)
		{
		case MESH:
		{
			Mesh *mesh = static_cast<Mesh *>(prim);
			double potential_t = *t;
			vec3 potential_normal;
			// cout << "mesh bounding box: " << to_string(mesh->b_box_min) << " " << to_string(mesh->b_box_max) << endl;
			// cout << "mesh transform: " << to_string(mesh->transform) << endl;
			if (!box_intersection(ray_data, transform, &potential_t, &potential_normal))
			{
				continue;
			}
#ifdef RENDER_BOUNDING_VOLUMES
			*t = potential_t;
			*normal = potential_normal;
			closest_node = node_index;
			hit = true;

#else
			double beta, gamma, t_temp;
			double a, b, c, d, e, f, g, h, i, j, k, l;
			double M;
			g = ray.x;
			h = ray.y;
			i = ray.z;
			vec3 t_1, t_2, t_3;
			for (Triangle tri : mesh->m_faces)
			{
				t_1 = vec3(transform * inverse(mesh->transform) * vec4(mesh->m_vertices[tri.v1], 1.0f));
				t_2 = vec3(transform * inverse(mesh->transform) * vec4(mesh->m_vertices[tri.v2], 1.0f));
				t_3 = vec3(transform * inverse(mesh->transform) * vec4(mesh->m_vertices[tri.v3], 1.0f));

				a = t_1.x - t_2.x;
				b = t_1.y - t_2.y;
				c = t_1.z - t_2.z;

				d = t_1.x - t_3.x;
				e = t_1.y - t_3.y;
				f = t_1.z - t_3.z;

				j = t_1.x - ray_origin.x;
				k = t_1.y - ray_origin.y;
				l = t_1.z - ray_origin.z;
				// // cout << "eye_pos: " << to_string(eye_pos) << " ray_origin: " << to_string(ray_origin) << " ray: " << to_string(ray) << endl;
				// // cout << "S1: " << tri.v1 << " S2: " << tri.v2 << " S3: " << tri.v3 << endl;
				// // cout << "S1: " << to_string(mesh->m_vertices[tri.v1]) << " S2: " << to_string(mesh->m_vertices[tri.v2]) << " S3: " << to_string(mesh->m_vertices[tri.v3]) << endl;
				// // cout << "a: " << a << " b: " << b << " c: " << c << endl;
				// // cout << "d: " << d << " e: " << e << " f: " << f << endl;
				// // cout << "g: " << g << " h: " << h << " i: " << i << endl;
				// // cout << "j: " << j << " k: " << k << " l: " << l << endl;
				// // cout << "M: " << M << endl;

				M = a * (e * i - h * f) + b * (g * f - d * i) + c * (d * h - e * g);
				if (M < EPSILON_T && M > -EPSILON_T)
				{
					continue;
				}

				t_temp = -(f * (a * k - j * b) + e * (j * c - a * l) + d * (b * l - k * c)) / M;
				// // cout << "t_temp: " << t_temp << endl;
				if (t_temp < EPSILON_T || t_temp > *t)
				{
					continue;
				}

				gamma = (i * (a * k - j * b) + h * (j * c - a * l) + g * (b * l - k * c)) / M;
				// // cout << "gamma: " << gamma << endl;
				if (gamma < -EPSILON_T || gamma > 1 + EPSILON_T)
				{
					continue;
				}

				beta = (j * (e * i - h * f) + k * (g * f - d * i) + l * (d * h - e * g)) / M;
				// // cout << "beta: " << beta << " gamma: " << gamma << " t_temp: " << t_temp << endl;
				if (beta < -EPSILON_T || beta + gamma > 1 + EPSILON_T)
				{
					continue;
				}

				if (t_temp > EPSILON && t_temp < *t && intersection_visible(ray_origin + (t_temp * ray)))
				{
					*t = t_temp;
					*normal = normalize(cross(t_2 - t_1, t_3 - t_1));
					closest_node = node_index;
					hit = true;
				}
			}
#endif
			break;
		}
		case SPHERE:
		case NONHIER_SPHERE:
		{
			mat4 inv_transform = inverse(transform);
			vec3 potential_normal;

			double t_temp = sphere_intersection(&ray_origin, &ray, &inv_transform, &potential_normal);

			if ((t_temp > EPSILON && t_temp < *t) && intersection_visible(ray_origin + (t_temp * ray)))
			{
				*t = t_temp;
				closest_node = node_index;
				closest_type = NONHIER_SPHERE;
				*normal = potential_normal;

				hit = true;
			}
			break;
		}
		case CUBE:
		case NONHIER_BOX:
		{
			double t_temp = *t;
			vec3 potential_normal;

			if (box_intersection(ray_data, geo_nodes[node_index]->get_transform(), &t_temp, &potential_normal))
			{
				if (t_temp < *t && intersection_visible(ray_origin + (t_temp * ray)))
				{
					*t = t_temp;
					closest_node = node_index;
					closest_type = NONHIER_BOX;
					*normal = potential_normal;

					hit = true;
				}
			}
			break;
		}
		default:
			continue;
		}
	}
	if (closest_node != -1)
	{
		mat = mat_nodes[closest_node];
		*intersection_point = ray_origin + (*t * ray);
	}
	return hit;
}
vec3 cast_shadow_ray(const vec3 *point, const vec3 *normal, PhongMaterial *&mat)
{
	vec3 light_accumulated = vec3(0.0);
	vec3 norm = vec3(0.0);
	PhongMaterial *mat_tmp;

	for (Light *light : lights)
	{
		double t = std::numeric_limits<double>::infinity();
		vec3 potential_point = *point + (EPSILON * *normal);
		vec3 shadow_ray_vec = light->position - potential_point;
		Ray shadow_ray_data = {potential_point, normalize(shadow_ray_vec), length(shadow_ray_vec)};

		float distance = length(light->position - *point);
		if (!intersection(shadow_ray_data, &t, &potential_point, &norm, mat_tmp) || t > distance || t < 0.0)
		{
			float attenuation = 1 / (light->falloff[0] + light->falloff[1] * distance + light->falloff[2] * pow(distance, 2));
			vec3 l = normalize(light->position - *point);
			vec3 diffuse = mat->m_kd * light->colour * max(dot(*normal, l), 0.0f);
			light_accumulated += attenuation * (diffuse);
		}
	}
	return min(light_accumulated, vec3(1.0));
}

glm::vec3 ray_color(const Ray ray_data, int hits)
{
	vec3 col;
	double t = ray_data.dist;
	vec3 normal;
	PhongMaterial *mat = nullptr;
	vec3 point;
	// mat, point and normal are updated
	if (intersection(ray_data, &t, &point, &normal, mat))
	{
		col = mat->m_kd * ambient;
		if (mat->m_kd != vec3(0.0)) // diffuse material
		{
			col = col + cast_shadow_ray(&point, &normal, mat);
		}
		// need to update this if first obj is actually light
		if ((mat->m_ks != vec3(0.0)) && (hits < HIT_THRESHOLD)) // specular material
		{
			hits = hits + 1;
			vec3 r = glm::reflect(-l, *normal); // Reflect the light vector around the normal
												// vec3 specular = mat->m_ks * light->colour * pow(max(dot((eye_pos - *point), r), 0.0f), mat->m_shininess);

			vec3 reflected_ray = glm::reflect(ray_data.direction, normal);
			Ray reflected_ray_data = {point, reflected_ray, std::numeric_limits<float>::infinity()};
			col = col + mat->m_ks * ray_color(reflected_ray_data, hits);
			// remove self inter-reflection
		}
		return col;
	}
	// what about pixels that hit the light source itself? -- wont this be a single point?
	// represent light source as a sphere and material looks like a light source
	return BACKGROUND;
}
