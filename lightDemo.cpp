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

#include <math.h>
#include <iostream>
#include <sstream>
#include <string>

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

#include "avtFreeType.h"

#define ORTHOGONAL 0
#define PERSPECTIVE 1
#define MOVING 2

#define BOAT 7

using namespace std;

#define CAPTION "AVT Demo: Phong Shading and Text rendered with FreeType"
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
GLint lPos_uniformId;
GLint plLoc0, plLoc1, plLoc2, plLoc3, plLoc4, plLoc5;
GLint tex_loc, tex_loc1, tex_loc2;
	
class Camera{
public:
	float camPos[3] = {0.01f, 20.0f, 0.0f};
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
};

Boat boat;

float deltaT = 0.05;
float speed_decay = 0.01;

float buoy_positions[6][2] = {
	{10.0f, 7.0f},
	{-12.0f, 7.0f},
	{16.5f, -4.5f},
	{0.0f, -17.0f},
	{-16.5f, -4.5f },
	{0.0f, 17.0f}
};

// lights
float directionalLightPos[4] = { 1.0f, 1000.0f,1.0f, 0.0f };
float pointLightPos[6][4] = { };




// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = 39.0f, beta = 51.0f;
float r = 20.0f;

// Frame counting and FPS computation
long myTime,timebase = 0,frame = 0;
char s[32];
float lightPos[4] = {4.0f, 6.0f, 2.0f, 1.0f};

void setupPointLightPos() {
	for (int i = 0; i < 6; i++) {
		pointLightPos[i][0] = buoy_positions[i][0];
		pointLightPos[i][1] = buoy_positions[i][1];
	}
}

void timer(int value)
{
	std::ostringstream oss;
	oss << CAPTION << ": " << FrameCount << " FPS @ (" << WinX << "x" << WinY << ")";
	std::string s = oss.str();

	glutSetWindow(WindowHandle);
	glutSetWindowTitle(s.c_str());
    FrameCount = 0;
    glutTimerFunc(1000, timer, 0);
}

void refresh(int value)
{
	float angle_rad = boat.angle * (3.14 / 180.0f);
	boat.position[0] += boat.speed * sin(angle_rad) * deltaT;
	boat.position[2] += boat.speed * cos(angle_rad) * deltaT;

	if (boat.speed > 0) boat.speed -= speed_decay;
	else if (boat.speed < 0) boat.speed += speed_decay;

	if (boat.speed != 0) {
		cams[2].camPos[0] += boat.speed * sin(angle_rad) * deltaT;
		cams[2].camPos[2] += boat.speed * cos(angle_rad) * deltaT;
	}


	cams[2].camTarget[0] = boat.position[0];
	cams[2].camTarget[1] = 0.0f;
	cams[2].camTarget[2] = boat.position[2];
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


// ------------------------------------------------------------
//
// Render stufff
//

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
		multMatrixPoint(VIEW, lightPos,res);   //lightPos definido em World Coord so is converted to eye space
		glUniform4fv(lPos_uniformId, 1, res);

		//send the point light positions
		glUniform4fv(plLoc0, 1, pointLightPos[0]);
		glUniform4fv(plLoc1, 1, pointLightPos[1]);
		glUniform4fv(plLoc2, 1, pointLightPos[2]);
		glUniform4fv(plLoc3, 1, pointLightPos[3]);
		glUniform4fv(plLoc4, 1, pointLightPos[4]);
		glUniform4fv(plLoc5, 1, pointLightPos[5]);
		

	int objId=0; //id of the object mesh - to be used as index of mesh: Mymeshes[objID] means the current mesh

	int buoy = 0;
	for (int i = 0 ; i < 14; ++i) {

		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, myMeshes[objId].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, myMeshes[objId].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, myMeshes[objId].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, myMeshes[objId].mat.shininess);
		pushMatrix(MODEL);

		if (i == 0) translate(MODEL, 0.0f, -0.01f, 0.0f);

		if (i == 1) translate(MODEL, -10.0f, 0.01f, 0.0f);

		// fix house base offset
		if (i == 2) translate(MODEL, -12.5f, 0.0f, -0.5f);

		if (i == 3) {
			translate(MODEL, -12.0f, 1.0f, 0.0f);
			rotate(MODEL, 45, 0, 1, 0);
		}

		if (i == 4) translate(MODEL, -7.0f, 0.3f, 0.5f);

		if (i == 5 || i == 6) translate(MODEL, -9.0f, 0.25f, 2.0f); // tree

		if (i == 7) { // boat
			translate(MODEL, boat.position[0], 0, boat.position[2]);
			rotate(MODEL, boat.angle, 0, 1, 0);
			scale(MODEL, 0.4f, 0.2f, 1.0f);

			//OLA teste test test
		}
		
		if (i > 7) {
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
		glBindVertexArray(myMeshes[objId].vao);
			
		glDrawElements(myMeshes[objId].type, myMeshes[objId].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		popMatrix(MODEL);
		objId++;
		
	}

	//Render text (bitmap fonts) in screen coordinates. So use ortoghonal projection with viewport coordinates.
	glDisable(GL_DEPTH_TEST);
	//the glyph contains transparent background colors and non-transparent for the actual character pixels. So we use the blending
	glEnable(GL_BLEND);  
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	//viewer at origin looking down at  negative z direction
	pushMatrix(MODEL);
	loadIdentity(MODEL);
	pushMatrix(PROJECTION);
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);
	ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	RenderText(shaderText, "This is a sample text", 25.0f, 25.0f, 1.0f, 0.5f, 0.8f, 0.2f);
	RenderText(shaderText, "AVT Light and Text Rendering Demo", 440.0f, 570.0f, 0.5f, 0.3, 0.7f, 0.9f);
	popMatrix(PROJECTION);
	popMatrix(VIEW);
	popMatrix(MODEL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

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

		case 'c': 
			printf("Camera Spherical Coordinates (%f, %f, %f)\n", alpha, beta, r);
			break;
		case 'm': glEnable(GL_MULTISAMPLE); break;
		case 'n': glDisable(GL_MULTISAMPLE); break;

		case '1': active = 0; break;
		case '2': active = 1; break;
		case '3': active = 2; break;

		case 'a':
			if (boat.speed <= 1 && boat.paddle_direction == 1) 
				boat.speed += 0.1 * boat.paddle_strength;
			else if (boat.speed >= -1)
				boat.speed -= 0.1 * boat.paddle_strength;
			boat.angle += 5;
			break;
		case 'd':
			if (boat.speed <= 1 && boat.paddle_direction == 1) 
				boat.speed += 0.1 * boat.paddle_strength;
			else if (boat.speed >= -1)
				boat.speed -= 0.1 * boat.paddle_strength;
			boat.angle -= 5;
			break;
		case 's':
			if (boat.paddle_direction == 1) boat.paddle_direction = 0;
			else boat.paddle_direction = 1;
			break;
		case 'o':
			if (boat.paddle_strength == 1) boat.paddle_strength = 2;
			else boat.paddle_strength = 1;
			break;



		case 'C':
			//toggle point lights
			break;
		case 'N':
			// toggle directional light
			break;
		case 'H': 
			// toggle spot light
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
	shader.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/pointlight_gouraud.vert");
	shader.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/pointlight_gouraud.frag");

	// set semantics for the shader variables
	glBindFragDataLocation(shader.getProgramIndex(), 0,"colorOut");
	glBindAttribLocation(shader.getProgramIndex(), VERTEX_COORD_ATTRIB, "position");
	glBindAttribLocation(shader.getProgramIndex(), NORMAL_ATTRIB, "normal");
	//glBindAttribLocation(shader.getProgramIndex(), TEXTURE_COORD_ATTRIB, "texCoord");

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
	lPos_uniformId = glGetUniformLocation(shader.getProgramIndex(), "l_pos");
	plLoc0 = glGetUniformLocation(shader.getProgramIndex(), "pointLights[0].position");
	plLoc1 = glGetUniformLocation(shader.getProgramIndex(), "pointLights[1].position");
	plLoc2 = glGetUniformLocation(shader.getProgramIndex(), "pointLights[2].position");
	plLoc3 = glGetUniformLocation(shader.getProgramIndex(), "pointLights[3].position");
	plLoc4 = glGetUniformLocation(shader.getProgramIndex(), "pointLights[4].position");
	plLoc5 = glGetUniformLocation(shader.getProgramIndex(), "pointLights[5].position");
	tex_loc = glGetUniformLocation(shader.getProgramIndex(), "texmap");
	tex_loc1 = glGetUniformLocation(shader.getProgramIndex(), "texmap1");
	tex_loc2 = glGetUniformLocation(shader.getProgramIndex(), "texmap2");
	
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

	/// Initialization of freetype library with font_name file
	freeType_init(font_name);

	// set the camera position based on its spherical coordinates

	float angle_rad = boat.angle * (3.14 / 180.0f);

	cams[2].camPos[0] = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	cams[2].camPos[1] = r * sin(beta * 3.14f / 180.0f);
	cams[2].camPos[2] = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);


	// create geometry and VAO of the grass plane
	float amb0[] = { 0.2f, 0.3f, 0.7f, 1.0f };
	float diff0[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	float spec0[] = { 0.5f, 0.5f, 0.7f, 1.0f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 100.0f;
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
	shininess = 40.0f;
	
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
	shininess = 20.0f;

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

	// create geometry and VAO of the sleigh
	amesh = createCube();
	memcpy(amesh.mat.ambient, amb3, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff3, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec3, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
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
	for (int i = 0; i < 6; i++) myMeshes.push_back(amesh);


	//// some GL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
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


//  Callback Registration
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);

	glutTimerFunc(0, timer, 0);
	//glutIdleFunc(renderScene);  // Use it for maximum performance
	glutTimerFunc(0, refresh, 0);    //use it to to get 60 FPS whatever

//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
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


