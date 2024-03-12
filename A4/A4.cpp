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
float d = 400.0f;
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
		cout << "assigning index to node: " << node->m_name << endl;
		GeometryNode *geometryNode = static_cast<GeometryNode *>(node);
		geometryNode->index = idx;
		idx++;
		geo_nodes.push_back(geometryNode);
		PhongMaterial *mat = static_cast<PhongMaterial *>(geometryNode->m_material);
		mat_nodes.push_back(mat);
		Primitive *primitive = static_cast<Primitive *>(geometryNode->m_primitive);
		prim_nodes.push_back(primitive);
		cout << "primitive type: " << primitive->type << endl;
		cout << endl;
	}
	for (SceneNode *child : node->children)
	{
		assign_indices(child);
	}
}

void initialize_pixel_transformations(Image &image, double fov, vec3 view, vec3 up)
{
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
	// cout << "S2: " << to_string(S2) << endl;
	// cout << "w: " << to_string(w) << endl;
	// cout << "u: " << to_string(u) << endl;
	// cout << "v: " << to_string(v) << endl;
	// cout << "R3: " << to_string(R3) << endl;
	// cout << "T4: " << to_string(T4) << endl;
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
	cout << "R3" << to_string(R3) << endl;

	for (uint y = 0; y < h; ++y)
	{
		for (uint x = 0; x < w; ++x)
		{
			vec4 p_world = T4 * R3 * S2 * T1 * vec4(float(x) + 0.5f, float(y) + 0.05f, 0.0f, 1.0f);

			vec3 ray = normalize(vec3(p_world) - eye_pos);
			Ray ray_data = {eye_pos, ray, vec3(1.0) / ray};
			vec3 col = ray_color(ray_data, 0);

			image(x, y, 0) = col.x;
			image(x, y, 1) = col.y;
			image(x, y, 2) = col.z;
		}
	}
}

// if (intersection(ray, t, point, normal, mat))
bool intersection(SceneNode *root, mat4 parent_tansform, Ray ray_data, double *t, glm::vec3 *intersection_point, glm::vec3 *normal, PhongMaterial *&mat)
{
	// NH-sphere A value (quad form) -- only compute if spheres are present
	const vec3 ray = ray_data.direction;
	const vec3 ray_origin = ray_data.origin;
	float A = dot(ray, ray);
	float B;
	float C;
	int closest_node = -1;
	int closest_type = -1;
	*t = std::numeric_limits<double>::infinity();
	bool hit = false;
	Primitive *prim;
	// check intersection for current node

	for (int node_index = 0; node_index < NUM_NODES; ++node_index)
	{
		prim = prim_nodes[node_index];
		switch (prim->type)
		{
		case MESH:
		{
			Mesh *mesh = static_cast<Mesh *>(prim);
			double beta, gamma, t_temp;
			double a, b, c, d, e, f, g, h, i, j, k, l;
			double M;
			g = ray.x;
			h = ray.y;
			i = ray.z;
			for (Triangle tri : mesh->m_faces)
			{
				a = mesh->m_vertices[tri.v1].x - mesh->m_vertices[tri.v2].x;
				b = mesh->m_vertices[tri.v1].y - mesh->m_vertices[tri.v2].y;
				c = mesh->m_vertices[tri.v1].z - mesh->m_vertices[tri.v2].z;

				d = mesh->m_vertices[tri.v1].x - mesh->m_vertices[tri.v3].x;
				e = mesh->m_vertices[tri.v1].y - mesh->m_vertices[tri.v3].y;
				f = mesh->m_vertices[tri.v1].z - mesh->m_vertices[tri.v3].z;

				j = mesh->m_vertices[tri.v1].x - ray_origin.x;
				k = mesh->m_vertices[tri.v1].y - ray_origin.y;
				l = mesh->m_vertices[tri.v1].z - ray_origin.z;
				// cout << "eye_pos: " << to_string(eye_pos) << " ray_origin: " << to_string(ray_origin) << " ray: " << to_string(ray) << endl;
				// cout << "S1: " << tri.v1 << " S2: " << tri.v2 << " S3: " << tri.v3 << endl;
				// cout << "S1: " << to_string(mesh->m_vertices[tri.v1]) << " S2: " << to_string(mesh->m_vertices[tri.v2]) << " S3: " << to_string(mesh->m_vertices[tri.v3]) << endl;
				// cout << "a: " << a << " b: " << b << " c: " << c << endl;
				// cout << "d: " << d << " e: " << e << " f: " << f << endl;
				// cout << "g: " << g << " h: " << h << " i: " << i << endl;
				// cout << "j: " << j << " k: " << k << " l: " << l << endl;
				// cout << "M: " << M << endl;

				M = a * (e * i - h * f) + b * (g * f - d * i) + c * (d * h - e * g);
				if (M < EPSILON_T && M > -EPSILON_T)
				{
					continue;
				}

				t_temp = -(f * (a * k - j * b) + e * (j * c - a * l) + d * (b * l - k * c)) / M;
				// cout << "t_temp: " << t_temp << endl;
				if (t_temp < EPSILON_T || t_temp > *t)
				{
					continue;
				}

				gamma = (i * (a * k - j * b) + h * (j * c - a * l) + g * (b * l - k * c)) / M;
				// cout << "gamma: " << gamma << endl;
				if (gamma < -EPSILON_T || gamma > 1 + EPSILON_T)
				{
					continue;
				}

				beta = (j * (e * i - h * f) + k * (g * f - d * i) + l * (d * h - e * g)) / M;
				// cout << "beta: " << beta << " gamma: " << gamma << " t_temp: " << t_temp << endl;
				// exit(0);
				if (beta < -EPSILON_T || beta + gamma > 1 + EPSILON_T)
				{
					continue;
				}
				// exit(0);
				*intersection_point = ray_origin + (t_temp * ray);
				// cout << "ray_origin: " << to_string(ray_origin) << " ray: " << to_string(ray) << " t_temp: " << t_temp << endl;
				// cout << "intersection_point: " << to_string(*intersection_point) << endl;
				if (intersection_visible(*intersection_point))
				{
					*t = t_temp;
					*normal = normalize(cross(mesh->m_vertices[tri.v2] - mesh->m_vertices[tri.v1],
											  mesh->m_vertices[tri.v3] - mesh->m_vertices[tri.v1]));

					mat = mat_nodes[node_index];
					hit = true;
				}
			}
			break;
		}
		case SPHERE:
		{
			break;
		}
		case CUBE:
		{
			break;
		}
		case NONHIER_SPHERE:
		{
			NonhierSphere *nh_sphere = static_cast<NonhierSphere *>(prim);
			B = 2.0f * dot(ray, (ray_origin - nh_sphere->m_pos));
			C = dot(ray_origin - nh_sphere->m_pos, ray_origin - nh_sphere->m_pos) - pow(nh_sphere->m_radius, 2);

			double roots[2] = {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
			double close_root = std::numeric_limits<double>::infinity();
			int ret = quadraticRoots(A, B, C, roots);
			if (ret > 0)
			{
				if (roots[0] > EPSILON && roots[1] > EPSILON)
					close_root = std::min(roots[0], roots[1]);
				else if (roots[0] > EPSILON)
					close_root = roots[0];
				else if (roots[1] > EPSILON)
					close_root = roots[1];

				if (close_root < *t)
				{
					*t = close_root;
					closest_node = node_index;
					closest_type = NONHIER_SPHERE;
				}
			}
			break;
		}
		case NONHIER_BOX:
		{
			// https: // tavianator.com/2022/ray_box_boundary.html#:~:text=The%20slab%20method%20tests%20for,intersects%20the%20box%2C%20if%20any.&text=t%20%3D%20x%20%E2%88%92%20x%200%20x%20d%20.
			NonhierBox *nh_box = static_cast<NonhierBox *>(prim);
			// precompute box min and max
			vec3 box_min = nh_box->m_pos;
			vec3 box_max = nh_box->m_pos + vec3(nh_box->m_size);
			vec3 t0 = (box_min - ray_origin) * ray_data.inv_dir;
			vec3 t1 = (box_max - ray_origin) * ray_data.inv_dir;
			vec3 tmin = min(t0, t1);
			vec3 tmax = max(t0, t1);

			float t_near = max(tmin.x, max(tmin.y, tmin.z));
			float t_far = min(tmax.x, min(tmax.y, tmax.z));

			if (t_near > t_far || (t_far < 0 && t_near < 0))
			{
				continue;
			}
			float t_temp = min(max(t_near, 0.0f), max(t_far, 0.0f));
			if (t_temp > EPSILON && t_temp < *t)
			{
				*t = t_temp;
				closest_node = node_index;
				closest_type = NONHIER_BOX;
			}
			break;
		}
		default:
			continue;
		}
	}
	if (closest_node != -1)
	{
		*intersection_point = ray_origin + (*t * ray);
		switch (closest_type)
		{
		case NONHIER_SPHERE:
		{
			mat = mat_nodes[closest_node];

			NonhierSphere *nh_sphere = static_cast<NonhierSphere *>(prim_nodes[closest_node]);
			*normal = normalize(2.0f * (*intersection_point - nh_sphere->m_pos));

			hit = true;
			break;
		}
		case NONHIER_BOX:
		{
			mat = mat_nodes[closest_node];

			NonhierBox *nh_box = static_cast<NonhierBox *>(prim_nodes[closest_node]);
			*normal = vec3(0.0);
			for (int i = 0; i < 3; ++i)
			{
				if (abs((*intersection_point)[i] - nh_box->m_pos[i]) < EPSILON)
				{
					(*normal)[i] = -1.0;
				}
				else if (abs((*intersection_point)[i] - (nh_box->m_pos[i] + nh_box->m_size)) < EPSILON)
				{
					(*normal)[i] = 1.0;
				}
			}
			hit = true;
			break;
		}
		case MESH:
		{
			hit = true;
			break;
		}
		default:
			break;
		}
	}

	for (SceneNode *child : root->children)
	{
		int cur_t;
		int ret_t = intersection(child, ray_data, t, intersection_point, normal, mat);
		//
	}
	// return hit;
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
		vec3 shadow_ray = normalize(light->position - potential_point);
		Ray shadow_ray_data = {potential_point, shadow_ray, vec3(1.0) / shadow_ray};

		float distance = length(light->position - *point);
		if (!intersection(shadow_ray_data, &t, &potential_point, &norm, mat_tmp) || t > distance || t < 0.0)
		{
			float attenuation = 1 / (light->falloff[0] + light->falloff[1] * distance + light->falloff[2] * pow(distance, 2));
			vec3 l = normalize(light->position - *point);
			vec3 r = glm::reflect(-l, *normal); // Reflect the light vector around the normal
			vec3 diffuse = mat->m_kd * light->colour * max(dot(*normal, l), 0.0f);
			// vec3 specular = mat->m_ks * light->colour * pow(max(dot((eye_pos - *point), r), 0.0f), mat->m_shininess);
			vec3 specular = vec3(0.0);
			light_accumulated += attenuation * (diffuse + specular);
		}
	}
	return min(light_accumulated, vec3(1.0));
}

glm::vec3 ray_color(const Ray ray_data, int hits)
{
	vec3 col;
	double t = std::numeric_limits<double>::infinity();
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
			vec3 reflected_ray = glm::reflect(ray_data.direction, normal);
			Ray reflected_ray_data = {point, reflected_ray, vec3(1.0) / reflected_ray};
			col = col + mat->m_ks * ray_color(reflected_ray_data, hits);
		}
		return col;
	}
	// what about pixels that hit the light source itself? -- wont this be a single point?
	// represent light source as a sphere and material looks like a light source
	return BACKGROUND;
}
