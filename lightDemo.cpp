//
// AVT: Phong Shading and Text rendered with FreeType library
// The text rendering was based on https://learnopengl.com/In-Practice/Text-Rendering
// This demo was built for learning purposes only.
// Some code could be severely optimised, but I tried to
// keep as simple and clear as possible.
//
// The code comes with no warranties, use it at your own risk.
// You may use it, or parts of it, wherever you want.
// 
// Author: João Madeiras Pereira
//
#include <cmath> // For cos and sin
#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <vector>
#include <cstdlib>  // for random numbers

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>


// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

#include <IL/il.h>


// Use Very Simple Libs
#include "VSShaderlib.h"
#include "AVTmathLib.h"
#include "VertexAttrDef.h"
#include "geometry.h"
#include "Texture_Loader.h"
#include "flare.h"
#include "avtFreeType.h"

#define ORTHOGONAL 0
#define PERSPECTIVE 1
#define MOVING 2

#define BOAT 7

using namespace std;

#define CAPTION "AVT 2024/25 project"
int WindowHandle = 0;
int WinX = 1024, WinY = 768;

unsigned int FrameCount = 0;

//shaders
VSShaderLib shader;  //geometry
VSShaderLib shaderText;  //render bitmap text

//File with the font
const string font_name = "fonts/arial.ttf";

//Vector with meshes
vector<struct MyMesh> myMeshes;
vector<struct MyMesh> flareMeshes;
vector<struct MyMesh> fishMeshes;

//active camera variable
int active = 0;

//External array storage defined in AVTmathLib.cpp

/// The storage for matrices
extern float mMatrix[COUNT_MATRICES][16];
extern float mCompMatrix[COUNT_COMPUTED_MATRICES][16];

/// The normal matrix
extern float mNormal3x3[9];

GLint pvm_uniformId;
GLint vm_uniformId;
GLint normal_uniformId;
GLint lPos_uniformId[8];
GLint lightEnabledId;
GLuint TextureArray[2];
GLint texMode_uniformId;
GLint flareEffectOnId;

GLint tex_loc, tex_loc1, tex_loc2, tex_flare;


// CALCULATIONS 

inline double clamp(const double x, const double min, const double max) {
	return (x < min ? min : (x > max ? max : x));
}

inline int clampi(const int x, const int min, const int max) {
	return (x < min ? min : (x > max ? max : x));
}
// ------------------------------------------------------------


class AABB {
public:
	std::vector<float> min = { 0.0f, 0.0f, 0.0f };
	std::vector<float> max = { 0.0f, 0.0f, 0.0f };
};

class OBB {
public:
	std::vector<float> center = { 0.0f, 0.0f, 0.0f };
	std::vector<float> halfSize = { 0.0f, 0.0f, 0.0f };
	std::vector<std::vector<float>> orientation = { {1.0f, 0.0f, 0.0f},
												   {0.0f, 1.0f, 0.0f},
												   {0.0f, 0.0f, 1.0f} };
};

class Camera {
public:
	float camPos[3] = { 0.01f, 20.0f, 0.0f };
	float camTarget[3] = { 0.0f, 0.0f, 0.0f };
	int type = 0;
};

Camera cams[3];

class Boat {
public:
	float speed = 0;
	float angle = 0;
	int paddle_strength = 1;
	int paddle_direction = 1;
	float direction[3] = { 0.0f, 0.0f, 0.0f };
	float position[3] = { 0.0f, 0.0f, 0.0f };
	bool left_paddle_working = false;
	bool right_paddle_working = false;
	int paddle_angle = 0;
	int lives = 5;
	OBB boatOBB;
};

Boat boat;
int play_time = 0;

const int maxFish = 10; //Numero Maximo de Peixes
const float maxDistance = 20.0f; //Distancia a que podem tar do barco

float deltaT = 0.05;
float speed_decay = 0.01;

class Fish {
public:
	float speed = 0;
	float direction[3] = { 0.0f, 0.0f, 0.0f };
	float position[3] = { 0.0f, 0.0f, 0.0f };
	OBB fishOBB;
};


vector<class Fish> fishList;

float buoy_positions[6][2] = {
	{10.0f, 7.0f},
	{-12.0f, 7.0f},
	{16.5f, -4.5f},
	{0.0f, -17.0f},
	{-16.5f, -4.5f },
	{0.0f, 17.0f}
};

// lights
float directionalLightDir[4] = { -1.0f, -1.0f, -1.0f, 0.0f };
float pointLightPos[6][4];
float spotLightPos[2][4] = {
	{-0.1f, 0.2f, 0.8f, 1.0f },
	{ 0.1f, 0.2f, 0.8f, 1.0f }
};
float spotLightAngle = 0.1;

struct DirectionalLight {
	float direction[3] = { -0.2f, -1.0f, -0.3f };
	float ambient[3] = { 0.3f, 0.3f, 0.3f };
	float diffuse[3] = { 0.8f, 0.8f, 0.8f };
	float specular[3] = { 1.0f, 1.0f, 1.0f };
};

DirectionalLight dirLight;

bool isDay = true;
bool pointLightsOn = false;
bool spotLightsOn = false;
float coneDir[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
bool fogEffectOn = false;
bool flareEffectOn = false;

//Flare effect 
FLARE_DEF AVTflare;
float lightScreenPos[3];
GLuint FlareTextureArray[5];

bool isPaused = false;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = 39.0f, beta = 51.0f;
float r = 20.0f;

// Frame counting and FPS computation
long myTime, timebase = 0, frame = 0;
char s[32];
float lightPos[4] = { -10.0f, -10.0f, -10.0f, 1.0f };
	
//////////////////////////////////////////////////////////////////////////
//collision variables, matrix calcs, OBB into AABB and collision detection.
//variables


//matrix calcs
std::vector<float> subtract(std::vector<float> v1, std::vector<float> v2) {
	return { v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2] };
}

float dotProduct(std::vector<float> v1, std::vector<float> v2) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

std::vector<float> matrixVectorMultiply(std::vector<std::vector<float>> matrix, std::vector<float> vec) {
	return {
		matrix[0][0] * vec[0] + matrix[0][1] * vec[1] + matrix[0][2] * vec[2],
		matrix[1][0] * vec[0] + matrix[1][1] * vec[1] + matrix[1][2] * vec[2],
		matrix[2][0] * vec[0] + matrix[2][1] * vec[1] + matrix[2][2] * vec[2]
	};
}

void normalize(std::vector<float>& vec) {
	float length = std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
	if (length > 0.0f) {
		vec[0] /= length;
		vec[1] /= length;
		vec[2] /= length;
	}
}

float projectAABB(AABB aabb, std::vector<float> axis) {
	std::vector<float> extent = subtract(aabb.max, aabb.min);
	extent[0] *= 0.5f; extent[1] *= 0.5f; extent[2] *= 0.5f;
	return std::fabs(extent[0] * axis[0]) +
		std::fabs(extent[1] * axis[1]) +
		std::fabs(extent[2] * axis[2]);
}

float projectOBB(OBB obb, std::vector<float> axis) {
	std::vector<float> extent = obb.halfSize;
	return std::fabs(extent[0] * dotProduct(obb.orientation[0], axis)) +
		std::fabs(extent[1] * dotProduct(obb.orientation[1], axis)) +
		std::fabs(extent[2] * dotProduct(obb.orientation[2], axis));
}

// calculate the AABB from OBB
AABB calculateAABBFromOBB(OBB obb) {
	AABB aabb;

	aabb.min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	aabb.max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

	std::vector<std::vector<float>> corners(8);
	for (int x = -1; x <= 1; x += 2) {
		for (int y = -1; y <= 1; y += 2) {
			for (int z = -1; z <= 1; z += 2) {
				corners[(x + 1) / 2 * 4 + (y + 1) / 2 * 2 + (z + 1) / 2] = {
					obb.center[0] + x * obb.halfSize[0] * obb.orientation[0][0] + y * obb.halfSize[1] * obb.orientation[1][0] + z * obb.halfSize[2] * obb.orientation[2][0],
					obb.center[1] + x * obb.halfSize[0] * obb.orientation[0][1] + y * obb.halfSize[1] * obb.orientation[1][1] + z * obb.halfSize[2] * obb.orientation[2][1],
					obb.center[2] + x * obb.halfSize[0] * obb.orientation[0][2] + y * obb.halfSize[1] * obb.orientation[1][2] + z * obb.halfSize[2] * obb.orientation[2][2]
				};
			}
		}
	}

	for (const auto& corner : corners) {
		aabb.min[0] = std::min(aabb.min[0], corner[0]);
		aabb.min[1] = std::min(aabb.min[1], corner[1]);
		aabb.min[2] = std::min(aabb.min[2], corner[2]);

		aabb.max[0] = std::max(aabb.max[0], corner[0]);
		aabb.max[1] = std::max(aabb.max[1], corner[1]);
		aabb.max[2] = std::max(aabb.max[2], corner[2]);
	}

	return aabb;
}

//COLLISION
bool isColliding(const AABB& a, const AABB& b) {
	return (a.min[0] <= b.max[0] && a.max[0] >= b.min[0]) &&
		(a.min[1] <= b.max[1] && a.max[1] >= b.min[1]) &&
		(a.min[2] <= b.max[2] && a.max[2] >= b.min[2]);
}

bool isCollidingWithBuoy(const AABB& a, int buoy) {
	for (int i = 0; i < 6; i++) {
		return(a.min[0] <= buoy_positions[buoy][0] + 0.15f && a.max[0] >= buoy_positions[buoy][0] - 0.15f &&
			a.min[2] <= buoy_positions[buoy][1] + 0.15f && a.max[2] >= buoy_positions[buoy][1] - 0.15f);
	}
}

bool isCollidingWithIsland(const AABB& a) {
	return (a.min[0] <= -10.0f + 5.0f && a.max[0] >= -10.0f - 5.0f &&
		a.min[2] <= 5.0f && a.max[2] >= -5.0f);
}

void resetBoat() {
	boat.position[0] = 0.0;
	boat.position[2] = 0.0;
	boat.speed = 0.0;
	cams[2].camPos[0] = 0;
	cams[2].camPos[1] = 20 * sin(beta * 3.14f / 180.0f);
	cams[2].camPos[2] = -20;
	cams[2].camTarget[0] = 0.0;
	cams[2].camTarget[1] = 0.0;
}
//////////////////////////////////////////////////////////////////////////



//create OBB for the fish
OBB createOBB(float position[3], float halfSize[3]) {
	OBB OBB;

	std::copy(position, position + 3, OBB.center.begin());

	OBB.halfSize = { 0.5f, 0.25f, 0.15f }; // Unsure how to calculate

	OBB.orientation = {
		{1.0f, 0.0f, 0.0f},  // X-axis
		{0.0f, 1.0f, 0.0f},  // Y-axis
		{0.0f, 0.0f, 1.0f}   // Z-axis
	};

	return OBB;
}

void resetGame() {
	resetBoat();
	play_time = 0;
	boat.lives = 5;
}


void setupPointLightPos() {
	for (int i = 0; i < 6; i++) {
		pointLightPos[i][0] = buoy_positions[i][0];
		pointLightPos[i][1] = 0.71f;
		pointLightPos[i][2] = buoy_positions[i][1];
		pointLightPos[i][3] = 1.0f;
	}
}

void timer(int value)
{
	std::ostringstream oss;
	oss << CAPTION << ": " << FrameCount << " FPS @ (" << WinX << "x" << WinY << ")";
	std::string s = oss.str();

	play_time++;

	glutSetWindow(WindowHandle);
	glutSetWindowTitle(s.c_str());
    FrameCount = 0;
    glutTimerFunc(1000, timer, 0);
}

void updateFishSpeed(int value) {
	for (auto& fish : fishList) {
		fish.speed = fish.speed * 2;
	}
	glutTimerFunc(30000, updateFishSpeed, 0);
}

// ------------------------------------------------------------
//
// Despawn fish if away from boat
//
void despawnFish(float boatPos[3]) {
	for (int i = fishList.size() - 1; i >= 0; i--) {
		float dx = fishList[i].position[0] - boatPos[0];
		float dz = fishList[i].position[2] - boatPos[2];
		float distance = sqrt(dx * dx + dz * dz);

		if (distance > maxDistance) {
			fishList.erase(fishList.begin() + i);//«remove it
		}
	}
}

// ------------------------------------------------------------
//
// Spawn fish if there are less then maxFish
//
void spawnFish(float boatPos[3]) {

	if (fishList.size() < maxFish) {
		Fish newFish;

		std::random_device rd;  // Obtain a random number from hardware
		std::mt19937 gen(rd()); // Seed the generator

		std::uniform_real_distribution<double> dis(0.0, 360.0); // Distribution in the range [0, 360]

		// Generate a random angle
		double random_angle = dis(gen);

		newFish.position[0] = boat.position[0] + maxDistance * cos(random_angle);
		newFish.position[2] = boat.position[2] + maxDistance * sin(random_angle);
		newFish.position[1] = 0.0f; // doesnt move on the third axis

		newFish.speed = 0.01f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX)) * 0.05f; // Random speed

		newFish.direction[0] = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
		newFish.direction[2] = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
		newFish.direction[1] = 0.0f;  // doesnt move on the third axis

		newFish.fishOBB = createOBB(newFish.position, newFish.position);
		normalize(newFish.direction);
		fishList.push_back(newFish);
	}
}

void fishCollision(AABB boatAABB, AABB fishAABB) {
	if (isColliding(boatAABB, fishAABB)) {
		resetBoat();
		boat.lives -= 1;
		if (boat.lives == 0) 
			resetGame();
	}
}

// ------------------------------------------------------------
//
// Update function
//
void updateFish(float boatPos[3]) {

	AABB boatAABB = calculateAABBFromOBB(boat.boatOBB);
	// Spawn fish if necessary
	while (fishList.size() < maxFish) {
		spawnFish(boatPos);
	}

	// Move the fish and despawn if too far from the boat
	despawnFish(boatPos);

	// Update fish positions (this can be more complex if you want to simulate swimming)
	for (auto& fish : fishList) {
		// Example of simple fish movement
		AABB fishAABB = calculateAABBFromOBB(fish.fishOBB);
		fishCollision(boatAABB, fishAABB);
		fish.position[0] += fish.direction[0]*fish.speed;
		fish.fishOBB.center[0] = fish.position[0];
		fish.position[2] += fish.direction[1]*fish.speed;
		fish.fishOBB.center[2] = fish.position[2];


	}
}

void refresh(int value)
{

	if (isPaused) {
		glutPostRedisplay();
		glutTimerFunc(1000 / 60, refresh, 0);
		return;
	}

	if (boat.left_paddle_working || boat.right_paddle_working) {
		if (boat.speed <= 1 && boat.paddle_direction == 1)
			boat.speed += 0.1 * boat.paddle_strength;
		else if (boat.speed >= -1)
			boat.speed -= 0.1 * boat.paddle_strength;

		if (boat.left_paddle_working && !boat.right_paddle_working)
			boat.angle += 2;
		else if (!boat.left_paddle_working && boat.right_paddle_working)
			boat.angle -= 2;
		boat.paddle_angle += 2 * boat.paddle_strength;
	}

	float angle_rad = boat.angle * (3.14 / 180.0f);
	boat.position[0] += boat.speed * sin(angle_rad) * deltaT;
	boat.position[2] += boat.speed * cos(angle_rad) * deltaT;
	boat.boatOBB = createOBB(boat.position, boat.position);

	updateFish(boat.position);

	if (boat.speed > 0) boat.speed -= speed_decay;
	else if (boat.speed < 0) boat.speed += speed_decay;

	if (boat.speed != 0) {
		AABB boatAABB = calculateAABBFromOBB(boat.boatOBB);
		spotLightPos[0][0] = boat.position[0] + 0.8f * sin(angle_rad - spotLightAngle);
		spotLightPos[0][2] = boat.position[2] + 0.8f * cos(angle_rad);
		spotLightPos[1][0] = boat.position[0] + 0.8f * sin(angle_rad + spotLightAngle);
		spotLightPos[1][2] = boat.position[2] + 0.8f * cos(angle_rad);

		//calculate cone direction
		coneDir[0] = sin(angle_rad);
		coneDir[2] = cos(angle_rad);

		cams[2].camPos[0] = boat.position[0] - r * sin(angle_rad);
		cams[2].camPos[2] = boat.position[2] - r * cos(angle_rad) ;

		cams[2].camTarget[0] = boat.position[0];
		cams[2].camTarget[1] = 0.0f;
		cams[2].camTarget[2] = boat.position[2];

	
		if (isCollidingWithIsland(boatAABB)) {
			boat.speed = 0.0;

		}
		else {	
			for (int i = 0; i < 6; i++) {
				if (isCollidingWithBuoy(boatAABB, i)) {
					boat.speed = 0.0;
				}
			}
		}
	}


	glutPostRedisplay();
	glutTimerFunc(1000 / 60, refresh, 0);
}

// ------------------------------------------------------------
//
// Reshape Callback Function
//

void changeSize(int w, int h) {

	float ratio;
	// Prevent a divide by zero, when window is too short
	if(h == 0)
		h = 1;
	// set the viewport to be the entire window
	glViewport(0, 0, w, h);
	// set the projection matrix
	ratio = (1.0f * w) / h;
	loadIdentity(PROJECTION);
	perspective(53.13f, ratio, 0.1f, 1000.0f);
}


// Render the fish
void renderFish() {
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLint loc;
	int randomFish = rand() % 3;

	while (fishList.size() < maxFish) {
		spawnFish(boat.position);
	}

	for (int i = 0; i < fishList.size(); i++) {
		// Send the material of the fish
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, fishMeshes[randomFish].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, fishMeshes[randomFish].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, fishMeshes[randomFish].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, fishMeshes[randomFish].mat.shininess);

		// Set the fish position
		pushMatrix(MODEL);
		translate(MODEL, fishList[i].position[0], fishList[i].position[1], fishList[i].position[2]);
		scale(MODEL, 0.2f, 0.2f, 0.2f); // Adjust size of fish if needed

		// Send matrices to OpenGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// Render the fish mesh
		glBindVertexArray(fishMeshes[randomFish].vao);
		glDrawElements(fishMeshes[randomFish].type, fishMeshes[randomFish].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

// ------------------------------------------------------------
//
// Render stufff
//

void renderFlare(FLARE_DEF* flare, int lx, int ly, int* m_viewport) {  //lx, ly represent the projected position of light on viewport

	int     dx, dy;          // Screen coordinates of "destination"
	int     px, py;          // Screen coordinates of flare element
	int		cx, cy;
	float    maxflaredist, flaredist, flaremaxsize, flarescale, scaleDistance;
	int     width, height, alpha;    // Piece parameters;
	int     i;
	float	diffuse[4];

	GLint loc;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int screenMaxCoordX = m_viewport[0] + m_viewport[2] - 1;
	int screenMaxCoordY = m_viewport[1] + m_viewport[3] - 1;

	//viewport center
	cx = m_viewport[0] + (int)(0.5f * (float)m_viewport[2]) - 1;
	cy = m_viewport[1] + (int)(0.5f * (float)m_viewport[3]) - 1;

	// Compute how far off-center the flare source is.
	maxflaredist = sqrt(cx * cx + cy * cy);
	flaredist = sqrt((lx - cx) * (lx - cx) + (ly - cy) * (ly - cy));
	scaleDistance = (maxflaredist - flaredist) / maxflaredist;
	flaremaxsize = (int)(m_viewport[2] * flare->fMaxSize);
	flarescale = (int)(m_viewport[2] * flare->fScale);

	// Destination is opposite side of centre from source
	dx = clampi(cx + (cx - lx), m_viewport[0], screenMaxCoordX);
	dy = clampi(cy + (cy - ly), m_viewport[1], screenMaxCoordY);

	// Render each element. To be used Texture Unit 0

	glUniform1i(texMode_uniformId, 2); // draw modulated textured particles 
	glUniform1i(tex_loc, 0);  //use TU 0

	for (i = 0; i < flare->nPieces; ++i)
	{
		// Position is interpolated along line between start and destination.
		px = (int)((1.0f - flare->element[i].fDistance) * lx + flare->element[i].fDistance * dx);
		py = (int)((1.0f - flare->element[i].fDistance) * ly + flare->element[i].fDistance * dy);
		px = clampi(px, m_viewport[0], screenMaxCoordX);
		py = clampi(py, m_viewport[1], screenMaxCoordY);

		// Piece size are 0 to 1; flare size is proportion of screen width; scale by flaredist/maxflaredist.
		width = (int)(scaleDistance * flarescale * flare->element[i].fSize);

		// Width gets clamped, to allows the off-axis flaresto keep a good size without letting the elements get big when centered.
		if (width > flaremaxsize)  width = flaremaxsize;

		height = (int)((float)m_viewport[3] / (float)m_viewport[2] * (float)width);
		memcpy(diffuse, flare->element[i].matDiffuse, 4 * sizeof(float));
		diffuse[3] *= scaleDistance;   //scale the alpha channel

		if (width > 1)
		{
			// send the material - diffuse color modulated with texture
			loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
			glUniform4fv(loc, 1, diffuse);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, FlareTextureArray[flare->element[i].textureId]);

			pushMatrix(MODEL);
			translate(MODEL, (float)(px - width * 0.0f), (float)(py - height * 0.0f), 0.0f);
			scale(MODEL, (float)width, (float)height, 1);
			computeDerivedMatrix(PROJ_VIEW_MODEL);
			glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
			glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
			computeNormalMatrix3x3();
			glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

			glBindVertexArray(myMeshes[19].vao);
			glDrawElements(myMeshes[19].type, myMeshes[19].numIndexes, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
			popMatrix(MODEL);
		}
	}
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}


void renderHUD() {

	//Render text (bitmap fonts) in screen coordinates. So use ortoghonal projection with viewport coordinates.
	glDisable(GL_DEPTH_TEST);
	//the glyph contains transparent background colors and non-transparent for the actual character pixels. So we use the blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
	int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
	float char_width = 0.0f;
	float char_height = 0.0f;
	// Initialization of freetype library with font_name file
	freeType_init("fonts/PixelGame-R9AZe.otf", 0, 84, char_width, char_height);
	pushMatrix(MODEL);
	loadIdentity(MODEL);
	pushMatrix(PROJECTION);
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	// switch to orthogonal projection
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);
	ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	RenderText(shaderText, "TIME: " + std::to_string(play_time), 0.0f, windowHeight - char_height / 2.0f, 0.5f, 1.0f, 1.0f, 1.0f);
	float xPos = windowWidth - TextWidth("LIVES: ", 0.5f, char_width);
	RenderText(shaderText, "LIVES: " + std::to_string(boat.lives), xPos, windowHeight - char_height / 2.0f, 0.5f, 1.0f, 1.0f, 1.0f);
	if (isPaused) {
		xPos = windowWidth / 2.0f - (TextWidth("PAUSED", 0.5f, char_width) / 2.0f);
		float yPos = windowHeight / 2.0f;
		RenderText(shaderText, "PAUSED", xPos, yPos, 1.0f, 1.0f, 0.0f, 0.0f);
	}
	popMatrix(PROJECTION);
	popMatrix(VIEW);
	popMatrix(MODEL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}


void renderScene(void) {

	GLint loc;

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// load identity matrices
	loadIdentity(VIEW);
	loadIdentity(MODEL);

	// set the cameras
	GLint m_view[4];

	lookAt(cams[active].camPos[0], cams[active].camPos[1], cams[active].camPos[2],
		cams[active].camTarget[0], cams[active].camTarget[1], cams[active].camTarget[2],
		0.0f, 1.0f, 0.0f);
	glGetIntegerv(GL_VIEWPORT, m_view);
	float ratio = (float)(m_view[2] - m_view[0]) / (float)(m_view[3] - m_view[1]);

	loadIdentity(PROJECTION);

	if (cams[active].type == PERSPECTIVE) {
		perspective(53.13f, ratio, 0.1f, 1000.0f);
	}
	else if (cams[active].type == ORTHOGONAL) {
		ortho(ratio*(-25), ratio*25, -25, 25, 0.1f, 1000.0f);
	}

	// use our shader
	
	glUseProgram(shader.getProgramIndex());

	//send the light position in eye coordinates
	//glUniform4fv(lPos_uniformId, 1, lightPos); //efeito capacete do mineiro, ou seja lighPos foi definido em eye coord 

	float res[4];
	//multMatrixPoint(VIEW, lightPos,res);   //lightPos definido em World Coord so is converted to eye space
	//glUniform4fv(lPos_uniformId, 1, res);
	loc = glGetUniformLocation(shader.getProgramIndex(), "isDay");
	if (isDay == true)
		glUniform1i(loc, 1);
	else
		glUniform1i(loc, 0);

	loc = glGetUniformLocation(shader.getProgramIndex(), "pointLightsOn");
	if (pointLightsOn == true)
		glUniform1i(loc, 1);
	else
		glUniform1i(loc, 0);

	loc = glGetUniformLocation(shader.getProgramIndex(), "spotLightsOn");
	if (spotLightsOn == true)
		glUniform1i(loc, 1);
	else
		glUniform1i(loc, 0);


	loc = glGetUniformLocation(shader.getProgramIndex(), "fogEffectOn");
	if (fogEffectOn == true)
		glUniform1i(loc, 1);
	else
		glUniform1i(loc, 0);

	loc = glGetUniformLocation(shader.getProgramIndex(), "spotCosCutOff");
	glUniform1f(loc, 0.93f);

	//Send the directional light position
	GLint ldirpos = glGetUniformLocation(shader.getProgramIndex(), "dir_pos");
	multMatrixPoint(VIEW, directionalLightDir, res);
	glUniform4fv(ldirpos, 1, res);

	//send the point light positions
	for (int i = 0; i < 6; i++) {
		multMatrixPoint(VIEW, pointLightPos[i], res);
		glUniform4fv(lPos_uniformId[i], 1, res);
	}

	for (int i = 0; i < 2; i++) {
		multMatrixPoint(VIEW, spotLightPos[i], res);
		glUniform4fv(lPos_uniformId[6 + i], 1, res);
	}

	loc = glGetUniformLocation(shader.getProgramIndex(), "coneDir");
	multMatrixPoint(VIEW, coneDir, res);
	glUniform4fv(loc, 1, res);

	int buoy = 0;

	// Associar os Texture Units aos Objects Texture

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, TextureArray[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, TextureArray[1]);


	//Indicar aos tres samplers do GLSL quais os Texture Units a serem usados
	glUniform1i(tex_loc, 0);
	glUniform1i(tex_loc1, 1);

	for (int i = 0; i < 19; ++i) {

		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, myMeshes[i].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, myMeshes[i].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, myMeshes[i].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, myMeshes[i].mat.shininess);
		pushMatrix(MODEL);

		//if (i == 0) translate(MODEL, 0.0f, -0.01f, 0.0f);

		if (i == 1) translate(MODEL, -10.0f, 0.01f, 0.0f); //island

		// fix house base offset
		if (i == 2) translate(MODEL, -12.5f, 0.5f, -0.5f);

		if (i == 3) { //house roof
			translate(MODEL, -12.5f, 1.0f, -0.5f);
			rotate(MODEL, 45, 0, 1, 0);
		}

		if (i == 4) translate(MODEL, -7.0f, 0.3f, 0.5f);

		if (i == 5 || i == 6) translate(MODEL, -9.0f, 0.25f, 2.0f); // tree

		if (i == 7) { // boat base
			translate(MODEL, boat.position[0], 0.1, boat.position[2]);
			rotate(MODEL, boat.angle, 0, 1, 0);
			scale(MODEL, 0.4f, 0.2f, 0.7f);
		}
		if (i == 8) { // boat front
			translate(MODEL, boat.position[0], 0.1, boat.position[2]);
			rotate(MODEL, boat.angle, 0, 1, 0);
			translate(MODEL, 0.0f, 0.0f, 0.35f);
			rotate(MODEL, 90, 1, 0, 0);
			scale(MODEL, 1, 1, 0.5);
			rotate(MODEL, 45, 0, 1, 0);
		}
		if (i == 10) { // left row handle
			translate(MODEL, boat.position[0], 0.15f, boat.position[2]);
			rotate(MODEL, boat.angle, 0, 1, 0);
			if (boat.left_paddle_working && boat.paddle_direction == 1)
				rotate(MODEL, boat.paddle_angle, 1, 0, 0);
			else if (boat.left_paddle_working && boat.paddle_direction == 0)
				rotate(MODEL, -boat.paddle_angle, 1, 0, 0);
			translate(MODEL, -0.3f, 0.0f, 0.0f);
			rotate(MODEL, -45, 0, 0, 1);
		}
		if (i == 9) { // right row handle
			translate(MODEL, boat.position[0], 0.15f, boat.position[2]);
			rotate(MODEL, boat.angle, 0, 1, 0);
			if (boat.right_paddle_working && boat.paddle_direction == 1)
				rotate(MODEL, boat.paddle_angle, 1, 0, 0);
			else if (boat.right_paddle_working && boat.paddle_direction == 0)
				rotate(MODEL, -boat.paddle_angle, 1, 0, 0);
			translate(MODEL, 0.3f, 0.0f, 0.0f);
			rotate(MODEL, 45, 0, 0, 1);
		}
		if (i == 11) { //left row paddle
			translate(MODEL, boat.position[0], 0.0f, boat.position[2]);
			rotate(MODEL, boat.angle, 0, 1, 0);
			translate(MODEL, 0.0f, 0.15f, 0.0f);
			if (boat.left_paddle_working && boat.paddle_direction == 1)
				rotate(MODEL, boat.paddle_angle, 1, 0, 0);
			else if (boat.left_paddle_working && boat.paddle_direction == 0)
				rotate(MODEL, -boat.paddle_angle, 1, 0, 0);
			rotate(MODEL, 180, 1, 0, 0);
			translate(MODEL, -0.4f, 0.15f, 0.0f);
			rotate(MODEL, 45, 0, 0, 1);
			scale(MODEL, 0.1f, 0.15f, 0.05f);
		}
		if (i == 12) { //right3 row paddle
			translate(MODEL, boat.position[0], 0.0f, boat.position[2]);
			rotate(MODEL, boat.angle, 0, 1, 0);
			translate(MODEL, 0.0f, 0.15f, 0.0f);
			if (boat.right_paddle_working && boat.paddle_direction == 1)
				rotate(MODEL, boat.paddle_angle, 1, 0, 0);
			else if (boat.right_paddle_working && boat.paddle_direction == 0)
				rotate(MODEL, -boat.paddle_angle, 1, 0, 0);
			rotate(MODEL, 180, 1, 0, 0);
			translate(MODEL, 0.4f, 0.15f, 0.0f);
			rotate(MODEL, -45, 0, 0, 1);
			scale(MODEL, 0.1f, 0.15f, 0.05f);
		}

		if (i > 12) {
			translate(MODEL, buoy_positions[buoy][0], 0.0f, buoy_positions[buoy][1]);
			buoy++;
		}


		// send matrices to OGL
		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

		// Render mesh
		glBindVertexArray(myMeshes[i].vao);
		if (i == 0) {
			glUniform1i(texMode_uniformId, 1);
		}
		else glUniform1i(texMode_uniformId, 0);

		glDrawElements(myMeshes[i].type, myMeshes[i].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
	}
	renderFish();

	if (flareEffectOn) {
		int flarePos[2];
		int m_viewport[4];
		glGetIntegerv(GL_VIEWPORT, m_viewport);

		pushMatrix(MODEL);
		loadIdentity(MODEL);
		computeDerivedMatrix(PROJ_VIEW_MODEL);  //pvm to be applied to lightPost. pvm is used in project function

		if (!project(lightPos, lightScreenPos, m_viewport))
			printf("Error in getting projected light in screen\n");  //Calculate the window Coordinates of the light position: the projected position of light on viewport
		flarePos[0] = clampi((int)lightScreenPos[0], m_viewport[0], m_viewport[0] + m_viewport[2] - 1);
		flarePos[1] = clampi((int)lightScreenPos[1], m_viewport[1], m_viewport[1] + m_viewport[3] - 1);
		popMatrix(MODEL);

		//viewer looking down at  negative z direction
		pushMatrix(PROJECTION);
		loadIdentity(PROJECTION);
		pushMatrix(VIEW);
		loadIdentity(VIEW);
		ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
		renderFlare(&AVTflare, flarePos[0], flarePos[1], m_viewport);
		popMatrix(PROJECTION);
		popMatrix(VIEW);
	}

	renderHUD();

	glBindTexture(GL_TEXTURE_2D, 0);
	glutSwapBuffers();
}

// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeys(unsigned char key, int xx, int yy)
{
	switch(key) {

		case 27:
			glutLeaveMainLoop();
			break;

		case '0': 
			printf("Camera Spherical Coordinates (%f, %f, %f)\n", alpha, beta, r);
			break;
		case 'm': glEnable(GL_MULTISAMPLE); break;
		case '�': glDisable(GL_MULTISAMPLE); break;

		case '1': active = 0; break;
		case '2': active = 1; break;
		case '3': active = 2; break;

		case 'a':
			if (isPaused) break;
			boat.right_paddle_working = true;
			break;
		case 'd':
			if (isPaused) break;
			boat.left_paddle_working = true;
			break;
		case 's':
			if (isPaused) break;
			if (boat.paddle_direction == 1) boat.paddle_direction = 0;
			else boat.paddle_direction = 1;
			break;
		case 'o':
			if (isPaused) break;
			if (boat.paddle_strength == 1) boat.paddle_strength = 2;
			else boat.paddle_strength = 1;
			break; 
		case 'f': 
			if (fogEffectOn == false) {
				fogEffectOn = true;
				printf("Fog effect enabled.\n");
			}
			else {
				fogEffectOn = false;
				printf("Fog effect disabled.\n");
			}
			break;
		case 'g':
			if (flareEffectOn == false && isDay == true) {
				flareEffectOn = true;
				printf("Flare effect enabled.\n");
			}
			else {
				flareEffectOn = false;
				printf("Flare effect disabled.\n");
			}
			break;
		case 'c': 
			if (pointLightsOn == false) {
				pointLightsOn = true;
				printf("Point lights enabled.\n");
			}
			else {
				pointLightsOn = false;
				flareEffectOn = false;
				printf("Point lights disabled.\n");
			}
			break;
		case 'h':
			if (spotLightsOn == false) {
				spotLightsOn = true;
				printf("Spot lights enabled.\n");
			}
			else {
				spotLightsOn = false;
				printf("Spot lights disabled.\n");
			}
			break;
		case 'n':
			if (isDay == false) {
				isDay = true;
				printf("Day lights enabled.\n");
			}
			else {
				isDay = false;
				printf("Day lights disabled.\n");
			}
			break;

		case 'p':
			isPaused = !isPaused;
			break;

		case 'r':
			resetGame();
			break;
	}
}

void processKeysUp(unsigned char key, int xx, int yy) {
	switch (key) {
		case 'a':
			boat.right_paddle_working = false;
			break;
		case 'd':
			boat.left_paddle_working = false;
			break;
	}
}

// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	// start tracking the mouse
	if (state == GLUT_DOWN)  {
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else if (button == GLUT_RIGHT_BUTTON)
			tracking = 2;
	}

	//stop tracking the mouse
	else if (state == GLUT_UP) {
		if (tracking == 1) {
			alpha -= (xx - startX);
			beta += (yy - startY);
		}
		else if (tracking == 2) {
			r += (yy - startY) * 0.01f;
			if (r < 0.1f)
				r = 0.1f;
		}
		tracking = 0;
	}
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{

	int deltaX, deltaY;
	float alphaAux, betaAux;
	float rAux;

	deltaX =  - xx + startX;
	deltaY =    yy - startY;
	alphaAux = 0;

	// left mouse button: move camera
	if (tracking == 1) {


		alphaAux = alpha + deltaX;
		betaAux = beta + deltaY;

		if (betaAux > 85.0f)
			betaAux = 85.0f;
		else if (betaAux < -85.0f)
			betaAux = -85.0f;
		rAux = r;
	}
	// right mouse button: zoom
	else if (tracking == 2) {

		alphaAux = alpha;
		betaAux = beta;
		rAux = r + (deltaY * 0.01f);
		if (rAux < 0.1f)
			rAux = 0.1f;

	}

	cams[2].camPos[0] = boat.position[0] + rAux * sin(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	cams[2].camPos[2] = boat.position[2] + rAux * cos(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	cams[2].camPos[1] = rAux * sin(betaAux * 3.14f / 180.0f);

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}


void mouseWheel(int wheel, int direction, int x, int y) {

	r += direction * 0.1f;
	if (r < 0.1f)
		r = 0.1f;

	cams[active].camPos[0] = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	cams[active].camPos[2] = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	cams[active].camPos[1] = r *   						     sin(beta * 3.14f / 180.0f);

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}

// --------------------------------------------------------
//
// Shader Stuff
//

GLuint setupShaders() {

	// Shader for models
	shader.init();
	shader.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/pointlight_phong.vert");
	shader.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/pointlight_phong.frag");

	// set semantics for the shader variables
	glBindFragDataLocation(shader.getProgramIndex(), 0,"colorOut");
	glBindAttribLocation(shader.getProgramIndex(), VERTEX_COORD_ATTRIB, "position");
	glBindAttribLocation(shader.getProgramIndex(), NORMAL_ATTRIB, "normal");
	glBindAttribLocation(shader.getProgramIndex(), TEXTURE_COORD_ATTRIB, "texCoord");

	glLinkProgram(shader.getProgramIndex());
	printf("InfoLog for Model Rendering Shader\n%s\n\n", shaderText.getAllInfoLogs().c_str());

	if (!shader.isProgramValid()) {
		printf("GLSL Model Program Not Valid!\n");
		printf("InfoLog for Per Fragment Phong Lightning Shader\n%s\n\n", shader.getAllInfoLogs().c_str());
		exit(1);
	}

	pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
	vm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
	normal_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_normal");
	glUniform1d(lightEnabledId, 1);
	

	GLint LightsUniformLoc = glGetUniformLocation(shader.getProgramIndex(), "point_pos");
	for (int i = 0; i < 6; i++) {
		std::string result = "point_pos[" + std::to_string(i) + "]";
		const GLchar* glString = result.c_str();
		lPos_uniformId[i] = glGetUniformLocation(shader.getProgramIndex(), glString);
	}

	LightsUniformLoc = glGetUniformLocation(shader.getProgramIndex(), "spot_pos");
	for (int i = 0; i < 2; i++) {
		std::string result = "spot_pos[" + std::to_string(i) + "]";
		const GLchar* glString = result.c_str();
		lPos_uniformId[6 + i] = glGetUniformLocation(shader.getProgramIndex(), glString);
	}
  
	texMode_uniformId = glGetUniformLocation(shader.getProgramIndex(), "texMode");
	tex_loc = glGetUniformLocation(shader.getProgramIndex(), "texmap");
	tex_loc1 = glGetUniformLocation(shader.getProgramIndex(), "texmap1");
	tex_loc2 = glGetUniformLocation(shader.getProgramIndex(), "texmap2");
	tex_flare = glGetUniformLocation(shader.getProgramIndex(), "tex_flare");
	
	printf("InfoLog for Per Fragment Phong Lightning Shader\n%s\n\n", shader.getAllInfoLogs().c_str());

	// Shader for bitmap Text
	shaderText.init();
	shaderText.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/text.vert");
	shaderText.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/text.frag");

	glLinkProgram(shaderText.getProgramIndex());
	printf("InfoLog for Text Rendering Shader\n%s\n\n", shaderText.getAllInfoLogs().c_str());

	if (!shaderText.isProgramValid()) {
		printf("GLSL Text Program Not Valid!\n");
		exit(1);
	}
	
	return(shader.isProgramLinked() && shaderText.isProgramLinked());
}

// ------------------------------------------------------------
//
// Model loading and OpenGL setup
//

void initCams() {
	//orth
	cams[0].type = ORTHOGONAL;
	cams[1].type = PERSPECTIVE;
	cams[2].type = PERSPECTIVE;
	return;
}


void init()
{
	// set the lights
	setupPointLightPos();

	MyMesh amesh;

	/* Initialization of DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		exit(0);
	}
	ilInit();

	// set the camera position based on its spherical coordinates

	float angle_rad = boat.angle * (3.14 / 180.0f);

	cams[2].camPos[0] = 0;
	cams[2].camPos[1] = r * sin(beta * 3.14f / 180.0f);
	cams[2].camPos[2] = -r;

	glGenTextures(2, TextureArray);
	Texture2D_Loader(TextureArray, "img/azure-blue-paint-diffusing-with-water.jpg", 0);
	Texture2D_Loader(TextureArray, "img/clear-ocean-water-texture.jpg", 1);

	//Flare elements textures
	glGenTextures(5, FlareTextureArray);
	Texture2D_Loader(FlareTextureArray, "crcl.tga", 0);
	Texture2D_Loader(FlareTextureArray, "flar.tga", 1);
	Texture2D_Loader(FlareTextureArray, "hxgn.tga", 2);
	Texture2D_Loader(FlareTextureArray, "ring.tga", 3);
	Texture2D_Loader(FlareTextureArray, "sun.tga", 4);

	// create geometry and VAO of the water plane
	float amb0[] = { 0.2f, 0.3f, 0.7f, 1.0f };
	float diff0[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	float spec0[] = { 0.5f, 0.5f, 0.7f, 1.0f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 50.0f;
	int texcount = 0;

	amesh = createQuad(100.0f, 100.0f);
	memcpy(amesh.mat.ambient, amb0, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff0, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec0, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);

	// create a geometry for a grassy island
	float amb6[] = { 0.1f, 0.4f, 0.1f, 1.0f };
	float diff6[] = { 0.2f, 0.8f, 0.2f, 1.0f };
	float spec6[] = { 0.1f, 0.3f, 0.1f, 1.0f };
	shininess = 10.0f;
	
	amesh = createQuad(10.0f, 10.0f);
	memcpy(amesh.mat.ambient, amb6, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff6, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec6, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);

	// create geometry and VAO of the house base
	float amb1[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	float diff1[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	float spec1[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	float emissive1[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	shininess = 10.0f;
	
	amesh = createCube();
	memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive1, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);

	
	// create geometry and VAO of the house roof
	float amb2[] = { 0.3f, 0.2f, 0.2f, 1.0f };
	float diff2[] = { 0.6f, 0.3f, 0.3f, 1.0f };
	float spec2[] = { 0.4f, 0.2f, 0.2f, 1.0f };
	shininess = 10.0f;

	amesh = createCone(0.7f, 1.0f, 4);
	memcpy(amesh.mat.ambient, amb2, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff2, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec2, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive1, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);

	
	// create geometry and VAO of the snowball
	shininess = 500.0f;
	amesh = createSphere(0.3f, 20);
	memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);


	// create geometry and VAO of the cylinder tree trunk
	float amb3[] = { 0.3f, 0.1f, 0.05f, 1.0f };
	float diff3[] = { 0.5f, 0.2f, 0.1f, 1.0f };
	float spec3[] = { 0.2f, 0.1f, 0.05f, 1.0f };
	shininess = 10.0f;

	amesh = createCylinder(0.5f, 0.2f, 20);
	memcpy(amesh.mat.ambient, amb3, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff3, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec3, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);

	// create geometry and VAO of the cone foliage
	float amb4[] = { 0.05f, 0.2f, 0.05f, 1.0f };
	float diff4[] = { 0.1f, 0.4f, 0.1f, 1.0f };
	float spec4[] = { 0.05f, 0.2f, 0.05f, 1.0f };
	shininess = 5.0f;

	amesh = createCone(1.0f, 0.5f, 20);
	memcpy(amesh.mat.ambient, amb4, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff4, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec4, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);

	// create geometry and VAO for fish blue
	float ambFishB[] = { 0.1f, 0.1f, 0.2f, 1.0f }; 
	float diffFishB[] = { 0.2f, 0.2f, 0.5f, 1.0f };  
	float specFishB[] = { 0.3f, 0.3f, 0.6f, 1.0f };  
	float emissiveFishB[] = { 0.0f, 0.0f, 0.0f, 1.0f }; 
	////////////////////////////////////////////////////
	float ambFishG[] = { 0.1f, 0.2f, 0.1f, 1.0f };  
	float diffFishG[] = { 0.2f, 0.4f, 0.2f, 1.0f };  
	float specFishG[] = { 0.3f, 0.6f, 0.3f, 1.0f };  
	float emissiveFishG[] = { 0.0f, 0.0f, 0.0f, 1.0f };  
	////////////////////////////////////////////////////	
	float ambFishR[] = { 0.2f, 0.1f, 0.1f, 1.0f };
	float diffFishR[] = { 0.5f, 0.2f, 0.2f, 1.0f };  
	float specFishR[] = { 0.6f, 0.3f, 0.3f, 1.0f }; 
	float emissiveFishR[] = { 0.0f, 0.0f, 0.0f, 1.0f };  
	shininess = 10.0f;

	
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambFishB, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffFishB, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specFishB, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissiveFishB, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	fishMeshes.push_back(amesh);

	amesh = createCube();
	memcpy(amesh.mat.ambient, ambFishG, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffFishG, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specFishG, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissiveFishG, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	fishMeshes.push_back(amesh);

	amesh = createCube();
	memcpy(amesh.mat.ambient, ambFishR, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffFishR, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specFishR, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissiveFishR, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	fishMeshes.push_back(amesh);


	

	// create geometry and VAO of the sleigh
	// create geometry and VAO of the boat
	amesh = createCube();
	memcpy(amesh.mat.ambient, amb3, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff3, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec3, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);


	amesh = createCone(0.2, 0.3, 4); // front of the boat
	memcpy(amesh.mat.ambient, amb3, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff3, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec3, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);

	// create rows
	amesh = createCylinder(0.4, 0.03, 4);
	memcpy(amesh.mat.ambient, amb3, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff3, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec3, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);
	myMeshes.push_back(amesh);

	amesh = createCube();
	memcpy(amesh.mat.ambient, amb3, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff3, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec3, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	myMeshes.push_back(amesh);
	myMeshes.push_back(amesh);

	// create buoys
	float amb5[] = { 0.2f, 0.05f, 0.05f, 1.0f };
	float diff5[] = { 0.6f, 0.1f, 0.1f, 1.0f };
	float spec5[] = { 0.8f, 0.2f, 0.2f, 1.0f };

	amesh = createCone(0.7f, 0.3f, 10);
	memcpy(amesh.mat.ambient, amb5, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff5, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec5, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	// for 6 buoys
	for (int i = 0; i < 6; i++) 
		myMeshes.push_back(amesh);

	// create geometry and VAO of the quad for flare elements
	amesh = createCube(); // created a cube because quad was not rendering a flare
	myMeshes.push_back(amesh);

	//Load flare from file
	loadFlareFile(&AVTflare, "flare.txt");

	// some GL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	//glDeleteTextures(2, TextureArray);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	initCams();

}

// ------------------------------------------------------------
//
// Main function
//


int main(int argc, char **argv) {

//  GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE);

	glutInitContextVersion (4, 3);
	glutInitContextProfile (GLUT_CORE_PROFILE );
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

	glutInitWindowPosition(100,100);
	glutInitWindowSize(WinX, WinY);
	WindowHandle = glutCreateWindow(CAPTION);

	//glEnable(GL_BLEND); //TRANS
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  //TRANS



//  Callback Registration
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);

	glutTimerFunc(0, timer, 0);
	glutTimerFunc(0, updateFishSpeed, 0);
	//glutIdleFunc(renderScene);  // Use it for maximum performance
	glutTimerFunc(0, refresh, 0);    //use it to to get 60 FPS whatever

//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	glutKeyboardUpFunc(processKeysUp);
	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc ( mouseWheel ) ;
	

//	return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

//	Init GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	printf ("Vendor: %s\n", glGetString (GL_VENDOR));
	printf ("Renderer: %s\n", glGetString (GL_RENDERER));
	printf ("Version: %s\n", glGetString (GL_VERSION));
	printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));

	if (!setupShaders())
		return(1);

	init();

	//  GLUT main loop
	glutMainLoop();

	return(0);
}

unsigned int getTextureId(char* name) {
	int i;

	for (i = 0; i < NTEXTURES; ++i)
	{
		if (strncmp(name, flareTextureNames[i], strlen(name)) == 0)
			return i;
	}
	return -1;
}

void    loadFlareFile(FLARE_DEF* flare, char* filename)
{
	int     n = 0;
	FILE* f;
	char    buf[256];
	int fields;

	memset(flare, 0, sizeof(FLARE_DEF));

	f = fopen(filename, "r");
	if (f)
	{
		fgets(buf, sizeof(buf), f);
		sscanf(buf, "%f %f", &flare->fScale, &flare->fMaxSize);

		while (!feof(f))
		{
			char            name[8] = { '\0', };
			double          dDist = 0.0, dSize = 0.0;
			float			color[4];
			int				id;

			fgets(buf, sizeof(buf), f);
			fields = sscanf(buf, "%4s %lf %lf ( %f %f %f %f )", name, &dDist, &dSize, &color[3], &color[0], &color[1], &color[2]);
			if (fields == 7)
			{
				for (int i = 0; i < 4; ++i) {
					color[i] = clamp(color[i] / 255.0f, 0.0f, 1.0f);
				}
				id = getTextureId(name);
				if (id < 0) printf("Texture name not recognized\n");
				else
					flare->element[n].textureId = id;
				flare->element[n].fDistance = (float)dDist;
				flare->element[n].fSize = (float)dSize;
				memcpy(flare->element[n].matDiffuse, color, 4 * sizeof(float));
				++n;
			}
		}

		flare->nPieces = n;
		fclose(f);
	}
	else printf("Flare file opening error\n");
}

