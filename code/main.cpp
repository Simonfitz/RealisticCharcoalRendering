#define STB_IMAGE_IMPLEMENTATION
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "stb_image.h"
#include "maths_funcs.h"
#include "model.h"
#include <string> 
#include <fstream>
#include <iostream>
#include <vector> 
#include <assimp/cimport.h>
#include <assimp/scene.h> 
#include <assimp/postprocess.h> 
using namespace std;

// DEFINES
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#define TOTAL_MESH 1
#define TOTAL_MODEL 3
#define TOTAL_IMAGES 9

//INTERGERS
int lastX, lastY;
int width = 600;
int height = 800;
unsigned int vao = 0;
int model_no = 1;

//GLUINTS
GLuint control_textures[TOTAL_IMAGES];
GLuint control_textures2[TOTAL_IMAGES];
GLuint control_textures3[TOTAL_IMAGES];
GLuint loc1;
GLuint loc2;
GLuint depth_buffer = 0;
GLuint depth_texture;

//FLOATS
float x_pos = 0.0, y_pos = 0.15, z_pos = -2.0;
float view_x_rotation = 0.0, view_y_rotation = 0.0, view_z_rotation = 0.0;
float modifier = 0.1;
float specular_exponent = 5.0f;
float light_rotation = 0.0;

//BOOLS
bool firstMouse = true;

//VECTORS
vec2 imageSize = vec2(1024, 1024);
vec4 light_position = vec4(1.0f, 1.0f, 1.0f, 0.0f);
vec3 diffuse_coef = vec3(1.0f, 1.0f, 1.0f);
vec3 ambient_coef = vec3(0.1, 0.1, 0.05);
vec3 specular_coef = vec3(0.8, 0.8, 0.8);

///old
const char * control_images[TOTAL_IMAGES] =
{
	"../images/Charcoal0.png", "../images/Charcoal1.png", "../images/Charcoal2.png",
	"../images/Charcoal3.png", "../images/Charcoal4.png", "../images/Charcoal5.png",
	"../images/Charcoal6.png", "../images/Charcoal7.png", "../images/Charcoal8.png"
};

///NEW
const char * control_images2[TOTAL_IMAGES] = 
{
	"../images/new0.jpg", "../images/new1.jpg", "../images/new2.jpg",
	"../images/new3.jpg", "../images/new4.jpg", "../images/new5.jpg",
	"../images/new6.jpg", "../images/new7.jpg", "../images/new8.jpg"
};

///Black Outline Textures
const char * control_images3[TOTAL_IMAGES] = 
{
	"../images/control1.png", "../images/control1.png", "../images/control1.png",
	"../images/control1.png", "../images/control1.png", "../images/control1.png",
	"../images/control1.png", "../images/control1.png", "../images/control1.png"
};

typedef struct model model;
Mesh * meshes[TOTAL_MESH];
Model * models[TOTAL_MODEL];
struct model 
{
	int num_vertices;
	GLuint vao;
	GLuint vbo[5];
	GLuint shader;
	GLuint texture;
	vector<float> verts;
	vector<float> normals;
	vector<float> tangents;
	vector<float> bitangents;
	vector<float> tex_coords;
};

void renderBitmapString(float x, float y, float z, void *font, const char *string)
{

	int j = strlen(string);

	glRasterPos3f(x, y, z);
	for (int i = 0; i < j; i++)
	{
		glutBitmapCharacter(font, string[i]);
	}

}

bool tex_load(const char* file_name, GLuint* tex) 
{
	int x, y, n;
	int force_channels = 4;
	unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);
	if (!image_data) {
		fprintf(stderr, "ERROR: could not load %s\n", file_name);
		return false;
	}
	// NPOT check
	if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
		fprintf(
			stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name
		);
	}
	int width_in_bytes = x * 4;
	unsigned char *top = NULL;
	unsigned char *bottom = NULL;
	unsigned char temp = 0;
	int half_height = y / 2;

	for (int row = 0; row < half_height; row++) {
		top = image_data + row * width_in_bytes;
		bottom = image_data + (y - row - 1) * width_in_bytes;
		for (int col = 0; col < width_in_bytes; col++) {
			temp = *top;
			*top = *bottom;
			*bottom = temp;
			top++;
			bottom++;
		}
	}
	glGenTextures(1, tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		x,
		y,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		image_data
	);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
	return true;
}

model * load_mesh(const char* file_name) {
	model * m = new model();
	m->verts = *new vector<float>();
	m->normals = *new vector<float>();
	m->tangents = *new vector<float>();
	m->bitangents = *new vector<float>();
	m->tex_coords = *new vector<float>();
	m->num_vertices = 0;
	const aiScene* scene = aiImportFile(file_name, aiProcess_Triangulate | aiProcess_CalcTangentSpace); // TRIANGLES!
	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return false;
	}
	printf("  %i animations\n", scene->mNumAnimations);
	printf("  %i cameras\n", scene->mNumCameras);
	printf("  %i lights\n", scene->mNumLights);
	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		m->num_vertices += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				//printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);

				m->verts.push_back(vp->x);
				m->verts.push_back(vp->y);
				m->verts.push_back(vp->z);
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				//printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
				m->normals.push_back(vn->x);
				m->normals.push_back(vn->y);
				m->normals.push_back(vn->z);
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				//printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
				m->tex_coords.push_back(vt->x);
				m->tex_coords.push_back(vt->y);
			}
			if (mesh->HasTangentsAndBitangents()) {
				const aiVector3D* vtan = &(mesh->mTangents[v_i]);
				const aiVector3D* vbitan = &(mesh->mBitangents[v_i]);
				m->tangents.push_back(vtan->x);
				m->tangents.push_back(vtan->y);
				m->tangents.push_back(vtan->z);
				m->bitangents.push_back(vbitan->x);
				m->bitangents.push_back(vbitan->y);
				m->bitangents.push_back(vbitan->z);
			}
		}
	}
	aiReleaseImport(scene);
	return m;
}


Mesh * createModelBuffer(model * m) {
	unsigned int pos_loc, norm_loc, tan_loc, bitan_loc, tex_loc;
	pos_loc = 0;
	norm_loc = 1;
	tex_loc = 2;
	tan_loc = 3;
	bitan_loc = 4;

	glGenVertexArrays(1, &(m->vao));
	glBindVertexArray(m->vao);
	glGenBuffers(5, m->vbo);

	glBindBuffer(GL_ARRAY_BUFFER, m->vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m->verts.size(), &m->verts[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(pos_loc);
	glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, m->vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m->normals.size(), &m->normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(norm_loc);
	glVertexAttribPointer(norm_loc, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	if (m->tex_coords.size() > 0 && tex_loc != -1) {
		glBindBuffer(GL_ARRAY_BUFFER, m->vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m->tex_coords.size(), &m->tex_coords[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(tex_loc);
		glVertexAttribPointer(tex_loc, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	if (m->tangents.size() > 0 && tan_loc != -1) {

		glBindBuffer(GL_ARRAY_BUFFER, m->vbo[3]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m->tangents.size(), &m->tangents[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(tan_loc);
		glVertexAttribPointer(tan_loc, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, m->vbo[4]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m->bitangents.size(), &m->bitangents[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(bitan_loc);
		glVertexAttribPointer(bitan_loc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	return new Mesh(m->vao, m->num_vertices);
}


void display() {

	glEnable(GL_DEPTH_TEST); 
	glDepthFunc(GL_LESS); 
	glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Transform Camera
	mat4 orthograph = orthographic(-10, 10, -10, 10, -10, 20);
	mat4 persp_proj = perspective(45.0, (float)width / (float)height, 0.1, 100.0);
	mat4 view = identity_mat4();
	view = rotate_z_deg(view, view_z_rotation);
	view = rotate_x_deg(view, view_x_rotation);
	view = rotate_y_deg(view, view_y_rotation);	
	view = translate(view, vec3(x_pos, y_pos, z_pos));
	
	//Lighting
	vec4 light_pos = rotate_y_deg(identity_mat4(), light_rotation) * light_position;
	mat4 light_view = look_at(light_pos, vec3(0, 0, 0), vec3(0, 1, 0));
	mat4 depthMatrix = orthograph * light_view;
	float * v = light_pos.v;
	vec4 light_dir = vec4(-v[0], -v[1], -v[2], -v[3]);

	glCullFace(GL_FRONT);


	//Create DEPTHMAP
	glViewport(0, 0, 1024, 1024);
	glBindFramebuffer(GL_FRAMEBUFFER, depth_buffer);
	glClear(GL_DEPTH_BUFFER_BIT);
	useShader(*shaders[SHADOW], light_view, orthograph, light_pos, light_dir, ambient_coef, diffuse_coef, specular_coef, imageSize, depth_texture, depthMatrix);

	Model::draw(*models[0], *shaders[SHADOW]);
	glCullFace(GL_BACK);

	//Use BARYCENTRIC shader
	Shader s = *shaders[BARYCENTRIC];
	useShader(*shaders[BARYCENTRIC], view, persp_proj, light_pos, light_pos, ambient_coef, diffuse_coef, specular_coef, imageSize, depth_texture, depthMatrix);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	
	switch (model_no)
	{
	case 1:
		glUseProgram(NULL);
		// TEXT
		renderBitmapString(-0.9, 0.75, 0.0, GLUT_BITMAP_HELVETICA_18, "Control Set: 1");
		renderBitmapString(-0.9, 0.7, 0.0, GLUT_BITMAP_HELVETICA_18, "Draw Outline: OFF");
		// Draw
		Model::draw(*models[0], s);
		break;

	case 2:
		glUseProgram(NULL);
		// TEXT
		renderBitmapString(-0.9, 0.75, 0.0, GLUT_BITMAP_HELVETICA_18, "Control Set: 2");
		renderBitmapString(-0.9, 0.7, 0.0, GLUT_BITMAP_HELVETICA_18, "Draw Outline: OFF");
		// Draw
		Model::draw(*models[1], s);
		break;

	case 3:
		glUseProgram(NULL);
		// TEXT
		renderBitmapString(-0.9, 0.75, 0.0, GLUT_BITMAP_HELVETICA_18, "Control Set: 1");
		renderBitmapString(-0.9, 0.7, 0.0, GLUT_BITMAP_HELVETICA_18, "Draw Outline: ON");
		// Draw
		Model::draw(*models[0], s);
		Model::draw(*models[2], s);
		break;

	case 4:
		glUseProgram(NULL);
		// TEXT
		renderBitmapString(-0.9, 0.75, 0.0, GLUT_BITMAP_HELVETICA_18, "Control Set: 2");
		renderBitmapString(-0.9, 0.7, 0.0, GLUT_BITMAP_HELVETICA_18, "Draw Outline: ON");
		// Draw
		Model::draw(*models[1], s);
		Model::draw(*models[2], s);
		break;
	}

	
	glutSwapBuffers();
}

void keypress(unsigned char key, int x, int y)
{
	float deltaX = 0;
	float deltaZ = 0;
	switch (key)
	{

	case 'w':

		deltaX -= modifier*sin(degreeToRad(view_y_rotation));
		deltaZ += modifier*cos(degreeToRad(view_y_rotation));
		break;

	case 's':

		deltaX += modifier*sin(degreeToRad(view_y_rotation));
		deltaZ -= modifier*cos(degreeToRad(view_y_rotation));
		break;

	case 'd':

		deltaX -= modifier*cos(degreeToRad(view_y_rotation));
		deltaZ -= modifier*sin(degreeToRad(view_y_rotation));
		break;

	case 'a':

		deltaX += modifier*cos(degreeToRad(view_y_rotation));
		deltaZ += modifier*sin(degreeToRad(view_y_rotation));
		break;

	case 'q':

		view_y_rotation -= 0.8;
		break;

	case 'e':

		view_y_rotation += 0.8;
		break;

	case 'm':

		light_rotation++; 
		break;

	case 'n':

		light_rotation--;
		break;

	case 'x':

		y_pos += 0.25;
		break;

	case 'z':

		y_pos -= 0.25;
		break;

	case '1':
		model_no = 1;
		break;

	case '2':
		model_no = 2;
		break;

	case '3':
		model_no = 3;
		break;

	case '4':
		model_no = 4;
		break;

	case '5':
		model_no = 5;
		break;

	}
	x_pos += deltaX;
	z_pos += deltaZ;
	glutPostRedisplay();
}

void processSpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:

		break;

	case GLUT_KEY_DOWN:

		break;

	case GLUT_KEY_LEFT:

		break;

	case GLUT_KEY_RIGHT:

		break;
	}
	glutPostRedisplay();
}

void processMouse(int x, int y)
{
	if (firstMouse)
	{
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	lastX = x;
	lastY = y;
	glutPostRedisplay();
}

void updateScene() {
	
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// Draw the next frame
	glutPostRedisplay();
}

void doShadow() {
	glGenFramebuffers(1, &depth_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, depth_buffer);

	glGenTextures(1, &depth_texture);

	glBindTexture(GL_TEXTURE_2D, depth_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depth_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void init()
{
	// Set up the shaders and meshes
	doShaders();
	doShadow();
	model m = *(load_mesh("../models/torso.obj"));
	meshes[0] = createModelBuffer(&m);

	//load in the control textures
	vector<char *> control_name =
	{
		"control0", "control1", "control2",
		"control3", "control4,", "control5",
		"control6", "control7", "control8"
	};

	vector<char *> allTextures(control_name);
	for (int i = 0; i < TOTAL_IMAGES; i++) 
	{
		tex_load(control_images[i], &control_textures[i]);
	}
	for (int i = 0; i < TOTAL_IMAGES; i++)
	{
		tex_load(control_images2[i], &control_textures2[i]);
	}
	for (int i = 0; i < TOTAL_IMAGES; i++)
	{
		tex_load(control_images3[i], &control_textures3[i]);
	}

	//load model
	models[0] = new Model(
		meshes[0], 
		*shaders[BARYCENTRIC],
		{ control_textures[0], control_textures[1], control_textures[2], control_textures[3], control_textures[4], control_textures[5], control_textures[6], control_textures[7], control_textures[8] }, 
		control_name,
		false, //use texture
		false, //use normal map
		false, // use refelection
		vec3(0.5, 0.5, 0.5), //scale
		vec3(0.0, 135.0, 0.0), //rotation
		vec3(0.0, 0.0, 0.0), //position
		vec3(0.1, 0.1, 0.1), //specular
		vec3(0.2, 0.2, 0.5), //diffuse
		vec3(0.1, 0.1, 0.1), //ambient
		specular_exponent // spec_coef
	);

	//load model 2
	models[1] = new Model(
		meshes[0],
		*shaders[BARYCENTRIC],
		{ control_textures2[0], control_textures2[1], control_textures2[2], control_textures2[3], control_textures2[4], control_textures2[5], control_textures2[6], control_textures2[7], control_textures2[8] },
		control_name,
		false, //use texture
		false, //use normal map
		false, // use refelection
		vec3(0.5, 0.5, 0.5), //scale
		vec3(0.0, 135.0, 0.0), //rotation
		vec3(0.0, 0.0, 0.0), //position
		vec3(0.1, 0.1, 0.1), //specular
		vec3(0.2, 0.2, 0.5), //diffuse
		vec3(0.1, 0.1, 0.1), //ambient
		specular_exponent // spec_coef
	);

	//outline model
	models[2] = new Model(
		meshes[0],
		*shaders[BARYCENTRIC],
		{ control_textures3[0], control_textures3[1], control_textures3[2], control_textures3[3], control_textures3[4], control_textures3[5], control_textures3[6], control_textures3[7], control_textures3[8] },
		control_name,
		false, //use texture
		false, //use normal map
		false, // use refelection
		vec3(0.56, 0.5555, 0.56), //scale
		vec3(0.0, 135.0, 0.0), //rotation
		vec3(0.0, 0.015, -0.2), //position
		vec3(0.1, 0.1, 0.1), //specular
		vec3(0.2, 0.2, 0.5), //diffuse
		vec3(0.1, 0.1, 0.1), //ambient
		specular_exponent // spec_coef
	);

}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Real Time Rendering");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	glutSpecialFunc(processSpecialKeys);

	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();

	return 0;
}
