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
	update_transforms(root, mat4(1.0));
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

	// cout << "Hit count: " << hit_count << endl;
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

void update_transforms(SceneNode *node, mat4 parent)
{
	// check if NH object
	if (node->index != -1)
	{
		int type = prim_nodes[node->index]->type;
		if (type == NONHIER_SPHERE)
		{
			NonhierSphere *nh_sphere = static_cast<NonhierSphere *>(prim_nodes[node->index]);
			node->scale(vec3(nh_sphere->m_radius));
			node->translate(nh_sphere->m_pos);
		}
		else if (type == NONHIER_BOX)
		{
			NonhierBox *nh_box = static_cast<NonhierBox *>(prim_nodes[node->index]);
			node->scale(vec3(nh_box->m_size));
			node->translate(nh_box->m_pos);
		}

		if (type == NONHIER_BOX || type == CUBE)
		{
			node->translate(vec3(0.5f, 0.5f, 0.5f));
		}
		if (type == MESH)
		{
			Mesh *mesh = static_cast<Mesh *>(prim_nodes[node->index]);
			// mesh->b_box_min -= vec3(1.0);
			// mesh->b_box_max += vec3(1.0);
			// cout << "min: " << to_string(mesh->b_box_min) << " max: " << to_string(mesh->b_box_max) << endl;

			vec3 bv_scale = mesh->b_box_max - mesh->b_box_min;
			vec3 bv_translation = (mesh->b_box_max) - (vec3(0.5) * bv_scale);
			if (mesh->b_box_max.x == mesh->b_box_min.x)
			{
				bv_scale.x = 1.0f;
			}
			if (mesh->b_box_max.y == mesh->b_box_min.y)
			{
				bv_scale.y = 1.0f;
			}
			if (mesh->b_box_max.z == mesh->b_box_min.z)
			{
				bv_scale.z = 1.0f;
			}

			node->scale(bv_scale);
			node->translate(bv_translation);

			mesh->setTransform(translate(mat4(1.0f), bv_translation) * scale(mat4(1.0f), bv_scale));
		}
	}
	node->trans = (parent * node->trans);
	node->unscaled = parent * node->unscaled;

	// cout << "node: " << node->m_name << " transform: " << to_string(node->trans) << endl;
	for (SceneNode *child : node->children)
	{
		update_transforms(child, node->unscaled);
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

	double progress = 0.0;
	for (uint y = 0; y < h; ++y)
	{
		for (uint x = 0; x < w; ++x)
		{
			vec4 p_world = T4 * R3 * S2 * T1 * vec4(float(x) + 0.5f, float(y) + 0.05f, 0.0f, 1.0f);
			vec3 ray_vec = vec3(p_world) - eye_pos;
			RayData ray_data = {eye_pos, normalize(ray_vec)};

			vec3 col = ray_color(ray_data, length(ray_vec) + EPSILON, std::numeric_limits<double>::infinity(), 0);
			image(x, y, 0) = col.x;
			image(x, y, 1) = col.y;
			image(x, y, 2) = col.z;
		}
		progress = ((double)y / (double)h) * 100.0;
		cout << "\rProgress: " << progress << " %" << std::flush;
	}
}

static double inline sphere_intersection(const vec3 *ray_pos,
										 const vec3 *ray_dir,
										 mat4 *inv,
										 vec3 *normal,
										 vec3 *intersection_point)
{
	// change this to be more like cube with conversions

	vec3 local_ray_pos = vec3(*inv * vec4(*ray_pos, 1.0f));
	vec3 local_ray_dir = normalize(vec3(*inv * vec4(*ray_dir, 0.0f)));

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
		*intersection_point = vec3(inverse(*inv) * vec4(local_normal, 1.0f));
		return dot(normalize(*ray_dir), vec3(inverse(*inv) * vec4(close_root * local_ray_dir, 0.0f)));
	}
	return -1.0;
}

static bool inline box_intersection(RayData ray_data, mat4 transformation, double *t, vec3 *normal)
{
	struct face
	{
		vec3 normal;
		vec3 point;
	};

	mat4 inv = inverse(transformation);

	vec3 local_ray_pos = vec3(inv * vec4(ray_data.origin, 1.0f));
	vec3 local_ray_dir = normalize(vec3(inv * vec4(ray_data.direction, 0.0f)));

	const mat3 normal_matrix = transpose(inverse(mat3(transformation)));
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

	// cout << "Transformation: " << to_string(transformation) << endl;
	for (int i = 0; i < 6; ++i)
	{
		// cout << "WS VTX: " << to_string(transformation * vec4(faces[i].point, 1.0f)) << endl;
		double denom = dot(local_ray_dir, faces[i].normal);
		if (abs(denom) > EPSILON)
		{
			double t_temp = dot(faces[i].point - local_ray_pos, faces[i].normal) / denom;
			vec3 local_intersection = local_ray_pos + (t_temp * local_ray_dir);
			if (t_temp > EPSILON &&
				abs(local_intersection.x) <= 0.5 + EPSILON &&
				abs(local_intersection.y) <= 0.5 + EPSILON &&
				abs(local_intersection.z) <= 0.5 + EPSILON)
			{
				vec3 norm = normalize(normal_matrix * faces[i].normal);
				vec3 potential_intersection = vec3(transformation * vec4(local_intersection, 1.0f));
				double potential_t = length(potential_intersection - ray_data.origin);
				if (potential_t > EPSILON && potential_t < *t)
				{
					*t = potential_t;
					*normal = norm;
					has_intersection = true;
				}
			}
		}
	}
	return has_intersection;
}

bool intersection(const RayData ray_data, double t0, double t1, IntersectionData &intersect_data)
{
	// NH-sphere A value (quad form) -- only compute if spheres are present
	const vec3 ray_dir = ray_data.direction;
	const vec3 ray_origin = ray_data.origin;
	int closest_node = -1;
	int closest_type = -1;
	bool hit = false;
	Primitive *prim;
	mat4 transform;

	for (int node_index = 0; node_index < NUM_NODES; ++node_index)
	{
		transform = geo_nodes[node_index]->get_transform();
		prim = prim_nodes[node_index];
		switch (prim->type)
		{
		case MESH:
		{
			// continue;
			double potential_t = t1;
			vec3 potential_normal;

			Mesh *mesh = static_cast<Mesh *>(prim);
			if (!box_intersection(ray_data, transform, &potential_t, &potential_normal))
			{
				continue;
			}
#ifdef RENDER_BOUNDING_VOLUMES
			intersect_data.t = potential_t;
			intersect_data.normal = potential_normal;
			intersect_data.mat = mat_nodes[node_index];
			hit = true;
#else
			double beta, gamma, t_temp;
			double a, b, c, d, e, f, g, h, i, j, k, l;
			double M;
			vec3 t_1, t_2, t_3;
			mat4 mesh_inv_trans = inverse(mesh->transform);

			vec3 trans_ray_origin = vec3(inverse(transform) * vec4(ray_origin, 1.0f));
			vec3 trans_ray_dir = vec3(inverse(transform) * vec4(ray_dir, 0.0f));

			g = trans_ray_dir.x;
			h = trans_ray_dir.y;
			i = trans_ray_dir.z;

			for (Triangle tri : mesh->m_faces)
			{
				// cout << "t1:" << to_string(mesh->m_vertices[tri.v1]) << " t2:" << to_string(mesh->m_vertices[tri.v2]) << " t3:" << to_string(mesh->m_vertices[tri.v3]) << endl;
				// cout << "mesh_inv_trans: " << to_string(mesh_inv_trans) << endl;
				t_1 = vec3(mesh_inv_trans * vec4(mesh->m_vertices[tri.v1], 1.0f));
				t_2 = vec3(mesh_inv_trans * vec4(mesh->m_vertices[tri.v2], 1.0f));
				t_3 = vec3(mesh_inv_trans * vec4(mesh->m_vertices[tri.v3], 1.0f));
				// cout << "t_1: " << to_string(t_1) << " t_2: " << to_string(t_2) << " t_3: " << to_string(t_3) << endl;

				a = t_1.x - t_2.x;
				b = t_1.y - t_2.y;
				c = t_1.z - t_2.z;

				d = t_1.x - t_3.x;
				e = t_1.y - t_3.y;
				f = t_1.z - t_3.z;

				j = t_1.x - trans_ray_origin.x;
				k = t_1.y - trans_ray_origin.y;
				l = t_1.z - trans_ray_origin.z;

				M = a * (e * i - h * f) + b * (g * f - d * i) + c * (d * h - e * g);
				if (abs(M) < 0.00001f)
				{
					continue;
				}

				t_temp = -(f * (a * k - j * b) + e * (j * c - a * l) + d * (b * l - k * c)) / M;
				if (t_temp < t0 || t_temp > t1)
				{
					continue;
				}

				gamma = (i * (a * k - j * b) + h * (j * c - a * l) + g * (b * l - k * c)) / M;
				if (gamma < -EPSILON_T || gamma > 1 + EPSILON_T)
				{
					continue;
				}

				beta = (j * (e * i - h * f) + k * (g * f - d * i) + l * (d * h - e * g)) / M;
				// cout << "beta: " << beta << " gamma: " << gamma << " t_temp: " << t_temp << endl;
				if (beta < -EPSILON_T || beta + gamma > 1 + EPSILON_T)
				{
					continue;
				}

				vec3 local_intersection = trans_ray_origin + (t_temp * trans_ray_dir);
				vec3 intersection_p = vec3(transform * vec4(local_intersection, 1.0f));
				if (intersection_visible(intersection_p))
				{

					vec3 local_norm = normalize(cross(t_2 - t_1, t_3 - t_1));
					vec3 normal_tmp = normalize(transpose(inverse(mat3(transform))) * local_norm);
					vec3 intersection_point_tmp = intersection_p + (EPSILON * normal_tmp);
					double t_temp = length(intersection_point_tmp - ray_origin);
					vec3 dif = abs((ray_origin + ray_dir * t_temp) - intersection_point_tmp);
					if (dif.x > EPSILON_T || dif.y > EPSILON_T || dif.z > EPSILON_T || t_temp < t0 || t_temp > t1 || (t_temp > intersect_data.t))
						continue;

					// cout << "here" << endl;
					intersect_data.normal = normal_tmp;
					intersect_data.intersection_point = intersection_point_tmp;
					intersect_data.t = t_temp;
					intersect_data.mat = mat_nodes[node_index];

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
			vec3 potential_intersection;

			double t_temp = sphere_intersection(&ray_origin, &ray_dir, &inv_transform, &potential_normal, &potential_intersection);

			if ((t_temp > t0 && t_temp < t1) && intersection_visible(potential_intersection))
			{
				intersect_data.t = t_temp;
				intersect_data.normal = potential_normal;
				intersect_data.intersection_point = potential_intersection + (EPSILON * potential_normal);
				intersect_data.mat = mat_nodes[node_index];

				hit = true;
			}
			break;
		}
		case CUBE:
		case NONHIER_BOX:
		{
			double t_temp = t1;
			vec3 potential_normal;

			if (box_intersection(ray_data, geo_nodes[node_index]->get_transform(), &t_temp, &potential_normal))
			{
				vec3 potential_intersection = ray_origin + (t_temp * ray_dir);
				if (t_temp > t0 && intersection_visible(potential_intersection))
				{
					intersect_data.t = t_temp;
					intersect_data.normal = potential_normal;
					intersect_data.mat = mat_nodes[node_index];
					intersect_data.intersection_point = potential_intersection + (EPSILON * potential_normal);

					hit = true;
				}
			}
			break;
		}
		default:
			continue;
		}
	}
	return hit;
}

glm::vec3 ray_color(const RayData ray_data, double t0, double t1, int hits)
{
	if (hits > HIT_THRESHOLD)
	{
		return BACKGROUND;
	}
	IntersectionData intersect_data, intersect_shadow_data;
	if (intersection(ray_data, t0, t1, intersect_data))
	{
		vec3 intersection_point = intersect_data.intersection_point;
		vec3 col = intersect_data.mat->m_kd * ambient;
		if (intersect_data.mat->m_ks != vec3(0.0))
		{
			for (Light *light : lights)
			{
				vec3 l_vec = (light->position - intersection_point);
				RayData shadow_ray_data = {
					intersection_point,
					normalize(l_vec),
				};

				if (!intersection(shadow_ray_data, EPSILON, length(l_vec) + EPSILON, intersect_shadow_data))
				{
					col = col + intersect_data.mat->m_kd * light->colour * max(dot(intersect_data.normal, shadow_ray_data.direction), 0.0f);
					// add specular
					vec3 view_dir = -ray_data.direction;
					vec3 reflect_dir = normalize(reflect(-l_vec, intersect_data.normal));
					if (intersect_data.mat->m_ks != vec3(0.0))
					{
						col += intersect_data.mat->m_ks * light->colour * pow(max(dot(view_dir, reflect_dir), 0.0f), intersect_data.mat->m_shininess);
					}
				}
			}
		}
		if (intersect_data.mat->m_ks != vec3(0.0))
		{
			// mirror reflection
			RayData reflected_ray_data = {
				intersection_point,
				normalize(reflect(ray_data.direction, intersect_data.normal)),
			};
			vec3 reflect_ = ray_data.direction - 2 * dot(ray_data.direction, intersect_data.normal) * intersect_data.normal;
			col += intersect_data.mat->m_ks * ray_color(reflected_ray_data, EPSILON, std::numeric_limits<double>::infinity(), hits + 1);
		}
		return min(col, vec3(1.0));
	}
	// what about pixels that hit the light source itself? -- wont this be a single point?
	// represent light source as a sphere and material looks like a light source
	float t = 0.5 * (normalize(ray_data.direction).y + 1.0);
	vec3 lower_color = vec3(0.5, 0.0, 0.8);
	vec3 upper_color = vec3(0.2, 0.4, 0.3);
	return mix(upper_color, lower_color, t);
}
