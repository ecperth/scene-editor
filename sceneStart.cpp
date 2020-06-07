// sceneStart.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "SceneHeaders.h"							// for now all possible headers are here 

// these needed early for gnatiread.h
glm::mat4 RotateX(float);							// create a matrix to rotate around X axis
glm::mat4 RotateY(float);							// create a matrix to rotate around Y axis
glm::mat4 RotateZ(float);							// create a matrix to rotate around Z axis
GLint windowHeight = 640, windowWidth = 960;
#include "gnatidread.h"								// contains definitions and c code for compilation
#include "gnatidread2.h"							// contains definitions and c code for compilation

// IDs for the GLSL program and GLSL variables.
GLuint shaderProgram; // The number identifying the GLSL shader program
GLint vPosition, vNormal, vTexCoord; // IDs for shader input vars (from glGetAttribLocation)
GLint uProjection, uModelView; // IDs for uniform variables (from glGetUniformLocation)
								//GLuint modelU;  //*BC* aded this to do lighting in world coords

GLint vBoneIDs;			// BC-Part 2
GLint vBoneWeights;		// BC-Part 2
GLint uBoneTransforms;		// BC-Part 2
static float deltaTime;		// BC-Part 2 in milli-secs - added for animation control

static float viewDist = -1.7; // *BC* was 1.5 - Distance from the camera to the centre of the scene
static float camRotSidewaysDeg = 0; // rotates the camera around the axis normal to the ground plane
static float camRotUpAndOverDeg = 20; // rotates the camera up and over the centre.

glm::mat4 projection; 	// Projection matrix - set in reshape()
glm::mat4 view; 		// View matrix - set in display()

// These are used to set the window title
char lab[] = "Project1";
char *programName = NULL; // Set in main 
int numDisplayCalls = 0; // Used to calculate the number of frames per second

//------Meshes----------------------------------------------------------------
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h
// (numMeshes is defined in gnatidread.h)
aiMesh* meshes[numMeshes]; 			// For each mesh we have a pointer to the mesh to draw
const aiScene* scenes[numMeshes];  	// BC-Part 2
GLuint vaoIDs[numMeshes]; 			// and a corresponding VAO ID from glGenVertexArrays
									// keep arrays of animation details (one per scene object)
float DurationInTicks[numMeshes];    // BC Part 2  (animation duration in total ticks)
float TicksPerSec[numMeshes];  		// BC Part 2  (animation ticks per seconds)
float poseTime[numMeshes];			// BC Part 2 (current pose time)

// -----Textures--------------------------------------------------------------
//                           (numTextures is defined in gnatidread.h)
GLuint textureIDs[numTextures]; // Stores the IDs returned by glGenTextures
#define NOT_LOADED 99999		// an impossible textureID ( hopefully!)

//------Scene Objects---------------------------------------------------------
//
// For each object in a scene we store the following
// Note: the following is exactly what the sample solution uses, you can do things differently if you want.
glm::vec3 StartWalkDir = glm::vec3(0.0, 0.0, 1.0);  // initial walk is in z direction
typedef struct {
	glm::vec4 	loc;
	float 	scale;
	// animation stuff
	int		numAnimations;		// no of animations for this object (initialised by loadMeshIfNotAlreadyLoaded)
	float 	poseTime;	        // updated by display() to control the animation phase (initialised by addObject()
	int 	AnimationDirection; // direction of animation 	(initialised by addObject())
	float 	AnimationSpeed;     // speed of animation		(initialised by addObject())
	float	DurationInTicks;	// duration of animation in ticks (initialised by loadMeshIfNotAlreadyLoaded)
	float	TicksPerSec;		// animation ticks per second (initialised by loadMeshIfNotAlreadyLoaded)
								// BC Walking
	float	walkSpeed; 			// Speed in units/sec
	float	walkLength; 	 	// Total walk length in units
	float	walkDistance;	 	// distance traveled per sec
	glm::vec3   walkDir;		// current walk direction (for animated objects)
	glm::vec4	walkVec;		// object position = loc + walkVec
	float	walkOrientation;	// Distance from loc either increasing (value 1) or decreasing (value -1)
								// int 	walkPause;			// to pause while direction is being changed
								// end animation stuff

	float 	angles[3];			// rotations around X, Y and Z axes.
	float 	diffuse; 			// material factor 0-1 (Ld) for light scattered in all directions equally
								// The light is from a light source at a specific location
								// Diffuse light at surface = LightStrength*Ld*(Ldir.surfaceN) 
	float 	specular;			// material factor 0-1 for light reflected from the surface towards a viewer
								// The light from a light source at a specific location
								// Its called specular highlighting
								// This is based on angle of reflection = angle of incident
								// The reflected light is max when the camera is aligned with the reflection angle
								// The reflected light decreases with cosine of angle between camera and reflection
								// This is further raised to the power of the shininess.
								// At maximum shininess reflection highlight is very local
	float 	ambient; 			// material factor 0-1 (La)for ambient light(equal from all directions)  
	float 	shine;				// Degree of scatter of reflected (specular light)
	glm::vec3 	rgb;
	float 	brightness;			// Multiplies all colours
	int 	meshId;
	int 	texId;
	float 	texScale;
} SceneObject;

const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely

SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene.
int nObjects = 0;    // How many objects are currently in the scene.
int currObject = -1; // The current object
int toolObj = -1;    // The object currently being modified
					 //----------------------------------------------------------------------------
					 // locations where the models and textures may be found
char dirDefault1[] = "C:\\MSProjects\\models-textures";
//----------------------------------------------------------------------------
// function prototypes
// --------------------------
GLuint Load_Texture(const char *);					// from LoadTextures.cpp

void init(void);									// intialisiation called once by main()
static void doRotate(void);							// set the tool to camera rotate
glm::mat2 camRotZ(void);							// helper function
													// Tool callbacks (set by menu selection)
static void adjustCamrotsideViewdist(glm::vec2);	// for camera rotation
static void adjustcamSideUp(glm::vec2);				// for camera rotation
static void adjustLocXZ(glm::vec2);					// used by several below
static void adjustScaleY(glm::vec2);				// for scale adjustment
static void adjustBrightnessY(glm::vec2);			// brightness adjustment						
static void adjustRedGreen(glm::vec2);				// red/green colour adjustment
static void adjustBlueBrightness(glm::vec2);		// blue brightness adjustment
static void adjustAngleYX(glm::vec2);
static void adjustAngleZTexscale(glm::vec2);
static void adjustDiffuseAmbient(glm::vec2);		// BC-3
static void adjustSpecShine(glm::vec2);				// BC-3

// scene building and drawing
void drawMesh(SceneObject, int);
static void addObject(int);							// add an object to the scene
void loadMeshIfNotAlreadyLoaded(int);				// read in a mesh
void loadTextureIfNotAlreadyLoaded(int);			// read in a texture
													// GLUT callbacks
void display(void);									// draw the scene
static void mouseClickOrScroll(int, int, int, int);	// called when a mouse button is pressed/released or wheel turned
static void mousePassiveMotion(int, int);			// called every time mouse is moved
void keyboard(unsigned char, int, int);				// called when key is pressed ( only ESC is used)
void idle(void);									// called when no other callback is pending
void reshape(int width, int height);				// called when user resizes the screen
void timer(int);									// called when timer expires
void fileErr(char* fileName);						// helper function

// menu functions
static void makeMenu(void);							// create the final menu system
static int createArrayMenu(int, const char[][128], void(*)(int)); // create an array of blank submenus
static void mainmenu(int);							// construct main menu
static void objectMenu(int);						// object submenu
static void texMenu(int);							// texture submenu
static void groundMenu(int);						// ground submenu
static void lightMenu(int);							// light submenu
static void materialMenu(int);						// material submenu


// *BC* ADDED for debug and test
static int DEBUGdisplayed = 0;
static GLfloat nearDist = 0.2;						// BC-4 was defined inside the reshape() below - made global to enable changes
static GLfloat nearFactor = 1.0;					// to adjust size of near plane

static GLfloat testAmbient = 0.1;
static GLfloat testDiffuse = 0.1;
static GLfloat testSpecular = 0.1;
static GLfloat testShininess = 0.1;

static GLfloat lightModel = 2;						// Start with light model = Binn Phong
static glm::vec3 SpotConeDir = glm::vec3(0.0, 1.0, 0.0); // Initial direction of spotlight
static GLfloat SpotBeamWidthDegrees = 5;

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{

	// Get the program name, excluding the directory, for the window title
	programName = argv[0];
	for (char *cpointer = argv[0]; *cpointer != 0; cpointer++)
		if (*cpointer == '/' || *cpointer == '\\') programName = cpointer + 1;
	printf("program name is %s\n", programName);

	// Set the models-textures directory
	strcpy_s(dataDir, dirDefault1);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);

	glutInitContextVersion(3, 3);
	//glutInitContextProfile(GLUT_CORE_PROFILE);       // Blocks deprectaed functions (no good for SOIL)
	// glGetString(GL_EXTENSIONS); is deprecated - this part of SOIL.c needs to be revised!!
	glutInitContextProfile( GLUT_COMPATIBILITY_PROFILE ); // should still use only OpenGL 3.2 Core

	glutCreateWindow("Initialising...");
	glewExperimental = true; // **BC** Just a setup to make sure the latest version of OpenGL is used
	glewInit(); // With some old hardware yields GL_INVALID_ENUM, if so use glewExperimental.
	CheckError(); // This bug is explained at: http://www.opengl.org/wiki/OpenGL_Loading_Library

	//*BC* register the callback functions ( all called by GLUT)
	glutDisplayFunc(display);                	// *BC* called to refresh the display
	glutKeyboardFunc(keyboard);					// *BC* called if keyboard key is pressed (ESC only)
	glutIdleFunc(idle);							// *BC* called when nothing else is pending
	glutMouseFunc(mouseClickOrScroll);			// *BC* called if mouse keys or scroll are used (starts/stops tool)
	glutPassiveMotionFunc(mousePassiveMotion);  // *BC* called if mouse moved with no buttons (updates mouseX and mouseY) 
	glutMotionFunc(doToolUpdateXY);				// *BC* called if mouse moved with a key pressed (updated coords for tool)
	glutReshapeFunc(reshape);					// *BC* called if window size is adjusted by the user
	glutTimerFunc(1000, timer, 1);				// *BC* this is called each time the timer expires
	CheckError();

	aiInit();									// initialise the assimp model loading package

	init();										// load the models/meshes to display
	CheckError();

	makeMenu();									// construct the menu system
	CheckError();

	glutMainLoop();								// pass permanent control to glut
	return 0;
} // end main()

  //----------------------------------------------------------------------------
  //The init() function
  //----------------------------------------------------------------------------
void init(void)
{
	// BC check the max number of attributes supported by openGL
	// This is only for curiosity and not used in program
	int max_attribs;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);
	printf("max attributes supported = %d\n", max_attribs); // GL_MAX_VERTEX_ATTRIBS is 16

	srand(time(NULL));		/* initialize random seed - so the starting scene varies */

	// Initialise the array of meshes (meshs are loaded later as needed by assimp) 
	for (int i = 0; i < numMeshes; i++)    meshes[i] = NULL; // maybe over cautious!

	glGenVertexArrays(numMeshes, vaoIDs); CheckError();   // Allocate vertex array objects for meshes

	// Load impossible textureIDS for now. SOIL will provide valid textureIDs and load the requested texture file. 
	for (int n = 0; n < numTextures; n++)
		textureIDs[n] = NOT_LOADED;

	// Load shaders and use the resulting shader program
	shaderProgram = InitShader("vStart.glsl", "fStart.glsl");

	glUseProgram(shaderProgram); CheckError();

	// BC-Part 2 initialise part 2 Vertex shader variables
	uBoneTransforms		= glGetUniformLocation(shaderProgram, "boneTransforms"); 
	uProjection			= glGetUniformLocation(shaderProgram, "Projection");
	uModelView			= glGetUniformLocation(shaderProgram, "ModelView");

	// Initialize the vertex position attribute from the vertex shader        
	vPosition			= glGetAttribLocation(shaderProgram, "vPosition1"); CheckError();
	vNormal				= glGetAttribLocation(shaderProgram, "vNormal"); 	CheckError();

	// Likewise, initialize the vertex texture coordinates attribute.    
	vTexCoord			= glGetAttribLocation(shaderProgram, "vTexCoord"); CheckError();
	vBoneIDs			= glGetAttribLocation(shaderProgram, "vBoneIDs"); CheckError();			// BC-Part 2
	vBoneWeights		= glGetAttribLocation(shaderProgram, "vBoneWeights"); CheckError();		// BC-Part 2

	// ************* BC Test Code - check that all Ids have good values
	printf("\nShader program_id   = %d\n", shaderProgram);

	printf("\nAttribute locations\n");
	printf("vPosition        = %d\n", vPosition);
	printf("vNormal          = %d\n", vNormal);
	printf("vTexCoord        = %d\n", vTexCoord);
	printf("vBoneIDs         = %d\n", vBoneIDs);
	printf("vBoneWeights     = %d\n", vBoneWeights);

	printf("\nUniform locations\n");
	printf("uBoneTransforms  = %d\n", uBoneTransforms);
	printf("uProjection      = %d\n", uProjection);
	printf("uModelView       = %d\n", uModelView);
	// ************** BC test code end

	// Objects 0, and 1 are the ground and the first light.
	addObject(0); // Object 0 is Square for the ground
	// Note - these override the values set in addObject()
	sceneObjs[0].loc = glm::vec4(0.0, 0.0, 0.0, 1.0);
	sceneObjs[0].scale = 10.0;
	sceneObjs[0].angles[0] = -90; //*BC* ground plane is xz ie y is perpendicular
	sceneObjs[0].texScale = 5.0; 	// Repeat the texture.
	sceneObjs[0].texId = 14; 		// *BC* force a denim(14) pattern


	addObject(55); // Sphere for the first light (1)
	// Note - these override the values set in addObject()
	sceneObjs[1].loc = glm::vec4(2.0, 1.0, 1.0, 1.0);
	sceneObjs[1].scale = 0.1;
	sceneObjs[1].texId = 0; // Plain texture
	sceneObjs[1].brightness = 0.5; // *BC* was 0.2

	// BC-8 start - sphere for directional light (2) -  object 2 as per instructions
	addObject(55); // Sphere for the second light
	// Note - these override the values set in addObject()
	sceneObjs[2].loc = glm::vec4(0.0, 0.0, 0.0, 1.0); // change its location
	sceneObjs[2].scale = 0.15; // make it a bit bigger
	sceneObjs[2].texId = 0; // Plain texture
	sceneObjs[2].brightness = 0.5; // *BC* was 0.2 then 0.5
	// BC-8 end - see other BC-8 

	// BC-8 start - sphere for spotlight light (3) -  object 3
	addObject(55); // Sphere for the second light
	// Note - these override the values set in addObject()
	sceneObjs[3].loc = glm::vec4(3.0, 1.0, 1.0, 1.0); // change its location
	sceneObjs[3].scale = 0.2; // make it a bit bigger
	sceneObjs[3].texId = 0; // Plain texture
	sceneObjs[3].brightness = 0.9; // *BC* was 0.2
	// BC-8 end - see other BC-8 

	//addObject(rand() % numMeshes);	// A test mesh
	addObject(2);  						// *BC* make object Winnie(31) or shark(32) or big dog(2) or dinosaur (3)
	sceneObjs[4].texId = 5; 			// *BC* force beach sand (5)

	// Enable the depth test to discard fragments behind previously drawn fragments for the same pixel.
	glEnable(GL_DEPTH_TEST);

	// setup stencil test for object outlining (ref learnopengl.com)
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	// end stencil test setup 

	doRotate(); // Start with tool set to rotate the camera.

	glClearColor(0.0, 0.0, 0.0, 1.0); /* black background */

} // end init

  //----------------------------------------------------------------------------
  // Set the mouse buttons to rotate the camera around the centre of the scene.
  //----------------------------------------------------------------------------
static void doRotate()
{
	// setToolCallbacks(adjustCamrotsideViewdist, mat2(400,0,0,-2), adjustcamSideUp, mat2(400, 0, 0,-90) );
	setToolCallbacks(adjustCamrotsideViewdist, glm::mat2(360, 0, 0, -10), adjustcamSideUp, glm::mat2(360, 0, 0, +360));	//BC-1
} // end doRotate()

  //----------------------------------------------------------------------------

glm::mat2 camRotZ()
{
	return rotZ(-camRotSidewaysDeg) * glm::mat2(10.0, 0, 0, -10.0);
}

// --------------------------------
//Tool callback functions
//---------------------------------
static void adjustCamrotsideViewdist(glm::vec2 cv)				// for camera rotation
{
	std::cout << "adjustCamrotsideViewdist() " << cv.x << " " << cv.y;
	camRotSidewaysDeg += cv[0];
	viewDist += cv[1];
	std::cout << " sideDeg=" << camRotSidewaysDeg << " viewDist=" << viewDist << std::endl;

	DEBUGdisplayed = 1;
}

static void adjustcamSideUp(glm::vec2 su)						// for camera rotation
{
	std::cout << "BC adjustcamSideUp() " << su.x << " " << su.y;
	camRotSidewaysDeg += su[0];
	camRotUpAndOverDeg += su[1];
	std::cout << "BC sideDeg=" << camRotSidewaysDeg << " UpOverDeg=" << camRotUpAndOverDeg << std::endl;

	DEBUGdisplayed = 1;
}
//--------------------------------------  
static void adjustLocXZ(glm::vec2 xz)							// for location X and Z ( all objects)
{
	sceneObjs[toolObj].loc[0] += xz[0];
	sceneObjs[toolObj].loc[2] += xz[1];

	DEBUGdisplayed = 1;
}
//---------------------------------------
static void adjustScaleY(glm::vec2 sy)							// for location Y and scale for non-light objects
{
	sceneObjs[toolObj].scale += sy[0];
	if (sceneObjs[toolObj].scale < 0.0001) sceneObjs[toolObj].scale = 0.0001;  // don't let scale go negative
	sceneObjs[toolObj].loc[1] += sy[1];

	DEBUGdisplayed = 1;
}

static void adjustBrightnessY(glm::vec2 by)			           // for Y location and brightness for lights 1 and 2			
//------------------------------------------
{
	sceneObjs[toolObj].brightness += by[0];
	sceneObjs[toolObj].loc.y += by[1];

	DEBUGdisplayed = 1;
}
//------------------------------------------
static void adjustRedGreen(glm::vec2 rg)
{
	sceneObjs[toolObj].rgb[0] += rg[0];
	if (sceneObjs[toolObj].rgb[0]>1) sceneObjs[toolObj].rgb[0] = 1;
	if (sceneObjs[toolObj].rgb[0]<0) sceneObjs[toolObj].rgb[0] = 0;
	sceneObjs[toolObj].rgb[1] += rg[1];
	if (sceneObjs[toolObj].rgb[1]>1) sceneObjs[toolObj].rgb[1] = 1;
	if (sceneObjs[toolObj].rgb[1]<0) sceneObjs[toolObj].rgb[1] = 0;

	DEBUGdisplayed = 1;
}
//------------------------------------------
static void adjustBlueBrightness(glm::vec2 bl_br)
{
	sceneObjs[toolObj].rgb[2] += bl_br[0];
	if (sceneObjs[toolObj].rgb[2]>1) sceneObjs[toolObj].rgb[2] = 1;
	if (sceneObjs[toolObj].rgb[2]<0) sceneObjs[toolObj].rgb[2] = 0;
	sceneObjs[toolObj].brightness += bl_br[1];
	if (sceneObjs[toolObj].brightness < 0) sceneObjs[toolObj].brightness = 0;

	DEBUGdisplayed = 1;
}
//------------------------------------------
// *BC* These 2 are active when menu "Rotation/Texture Scale" is selected
// ie rotate object and change the texture scale
static void adjustAngleYX(glm::vec2 angle_yx)
{
	sceneObjs[currObject].angles[0] += angle_yx[1];
	if (sceneObjs[currObject].angles[0] > 360) sceneObjs[currObject].angles[0] -= 360;
	if (sceneObjs[currObject].angles[0] < -360) sceneObjs[currObject].angles[0] += 360;
	sceneObjs[currObject].angles[1] += angle_yx[0];
	if (sceneObjs[currObject].angles[1] > 360) sceneObjs[currObject].angles[1] -= 360;
	if (sceneObjs[currObject].angles[1] < -360) sceneObjs[currObject].angles[1] += 360;

	//sceneObjs[currObject].walkPause = 100;  // pause the walk for 100mSec while direction changes

	glm::mat4 RotWalk = RotateZ(sceneObjs[currObject].angles[2])*
		RotateY(-sceneObjs[currObject].angles[1])*
		RotateX(sceneObjs[currObject].angles[0]);
	glm::vec4 walkNew = RotWalk*glm::vec4(StartWalkDir, 0);
	sceneObjs[currObject].walkDir = glm::normalize(glm::vec3(walkNew.x, walkNew.y, walkNew.z));

	printf("walk dir: %f  %f  %f \n", walkNew.x, walkNew.y, walkNew.z);
	printf("angles: %f  %f  %f \n", sceneObjs[currObject].angles[0], sceneObjs[currObject].angles[1], sceneObjs[currObject].angles[2]);

	DEBUGdisplayed = 1;
}

static void adjustAngleZTexscale(glm::vec2 az_ts)
{
	sceneObjs[currObject].angles[2] += az_ts[0];
	if (sceneObjs[currObject].angles[2] > 360) sceneObjs[currObject].angles[2] -= 360;
	if (sceneObjs[currObject].angles[2] < -360) sceneObjs[currObject].angles[2] += 360;
	sceneObjs[currObject].texScale += az_ts[1];

	glm::mat4 RotWalk = RotateZ(sceneObjs[currObject].angles[2])*
		RotateY(-sceneObjs[currObject].angles[1])*
		RotateX(sceneObjs[currObject].angles[0]);
	glm::vec4 walkNew = RotWalk*glm::vec4(StartWalkDir, 0);
	sceneObjs[currObject].walkDir = glm::normalize(glm::vec3(walkNew.x, walkNew.y, walkNew.z));

	printf("angles: %f  %f  %f \n", sceneObjs[currObject].angles[0], sceneObjs[currObject].angles[1], sceneObjs[currObject].angles[2]);
	printf("TexScale %f\n", sceneObjs[currObject].texScale);
	DEBUGdisplayed = 1;
}
//---------------------------------------------
// BC-3 Start
static void adjustDiffuseAmbient(glm::vec2 da)
{
	sceneObjs[currObject].ambient += da[0];
	sceneObjs[currObject].diffuse += da[1];

	// Force these to have range 0-1
	if (sceneObjs[currObject].ambient < 0) sceneObjs[currObject].ambient = 0;
	if (sceneObjs[currObject].ambient > 1) sceneObjs[currObject].ambient = 1;
	if (sceneObjs[currObject].diffuse < 0) sceneObjs[currObject].diffuse = 0;
	if (sceneObjs[currObject].diffuse > 1) sceneObjs[currObject].diffuse = 1;

	printf("Ambient: %f  Diffuse: %f\n", sceneObjs[currObject].ambient, sceneObjs[currObject].diffuse);

	DEBUGdisplayed = 1;
}

static void adjustSpecShine(glm::vec2 ss)
{
	sceneObjs[currObject].specular += ss[0];
	// Force specular to have range 0-1
	if (sceneObjs[currObject].specular < 0) sceneObjs[currObject].specular = 0;
	if (sceneObjs[currObject].specular > 1) sceneObjs[currObject].specular = 1;

	// force shine to have range 1-255
	sceneObjs[currObject].shine += ss[1];
	if (sceneObjs[currObject].shine < 1) sceneObjs[currObject].shine = 1;
	if (sceneObjs[currObject].shine >100) sceneObjs[currObject].shine = 100;

	printf("Specular: %f  Shine %f\n", sceneObjs[currObject].specular, sceneObjs[currObject].shine);
	DEBUGdisplayed = 1;
}
// BC-3 end
//---------------------------------------------
//end of tool functions

// ----------------------------------		   
// Add an object to the scene
// -----------------------------------
static void addObject(int id)
{
	DEBUGdisplayed = 1;  //*BC*


	glm::vec2 currPos = currMouseXYworld(camRotSidewaysDeg);
	printf("addObject %f,%f\n", currPos.x, currPos.y);
	sceneObjs[nObjects].loc[0] = currPos[0];
	sceneObjs[nObjects].loc[1] = 0.0;
	sceneObjs[nObjects].loc[2] = currPos[1];
	sceneObjs[nObjects].loc[3] = 1.0;

	sceneObjs[nObjects].angles[0] = 0.0;
	sceneObjs[nObjects].angles[1] = 180.0;    //Presumably because the Blender export had objects facing away
	sceneObjs[nObjects].angles[2] = 0.0;
	
	// *BC* this conditional is redundant (overridden in init()) 
	if (id != 0 && id != 55)
		sceneObjs[nObjects].scale = 0.005;

	sceneObjs[nObjects].rgb[0] = 0.7;
	sceneObjs[nObjects].rgb[1] = 0.7;
	sceneObjs[nObjects].rgb[2] = 0.7;

	sceneObjs[nObjects].brightness = 1.0;

	sceneObjs[nObjects].diffuse = 1.0;
	sceneObjs[nObjects].specular = 0.5;
	sceneObjs[nObjects].ambient = 0.7;
	sceneObjs[nObjects].shine = 10.0;

	sceneObjs[nObjects].meshId = id;

	sceneObjs[nObjects].texId = rand() % numTextures;

	sceneObjs[nObjects].texScale = 2.0;


	if (scenes[id] == NULL) {
		printf("Loading the scene and mesh data for model %d\n", id);
		loadMeshIfNotAlreadyLoaded(id); // load the mesh early
	}

	if (scenes[id]->HasAnimations()) { // this assumes only one animation per scene object

		sceneObjs[nObjects].DurationInTicks = scenes[id]->mAnimations[0]->mDuration; // in ticks
		sceneObjs[nObjects].TicksPerSec = scenes[id]->mAnimations[0]->mTicksPerSecond; // ticks sec
		sceneObjs[nObjects].numAnimations = scenes[id]->mNumAnimations;

		printf("num animations %d, duration %f, ticks %f\n", sceneObjs[nObjects].numAnimations,
			sceneObjs[nObjects].DurationInTicks, sceneObjs[nObjects].TicksPerSec);

		// BC controls for animation speed and direction
		sceneObjs[nObjects].poseTime = 0.0;          // initialize pose increment
		sceneObjs[nObjects].AnimationDirection = 0;  // set initial direction for forward
		sceneObjs[nObjects].AnimationSpeed = 0.4;   // was 1 -set speed of animation to 1 grid unit per sec

		// BC Walking controls
		sceneObjs[nObjects].walkLength = 3.0; 	 	// was 1.5 Total walk length in axis units

		// adjust animation speed to match the walk length (to make 1 walk length = 1 animation cycle)
		float bestWalkSpeed = (sceneObjs[nObjects].AnimationSpeed * sceneObjs[nObjects].TicksPerSec * sceneObjs[nObjects].walkLength)
			/ (sceneObjs[nObjects].DurationInTicks);
		printf("best walk speed = %f\n", bestWalkSpeed);
		sceneObjs[nObjects].walkSpeed = bestWalkSpeed;

		sceneObjs[nObjects].walkDistance = 0;	 	// Distance traveled total
		sceneObjs[nObjects].walkDir = StartWalkDir;	// current walk direction (for animated objects)
		sceneObjs[nObjects].walkVec = glm::vec4(0);	// object position = loc + walkVec
		sceneObjs[nObjects].walkOrientation = 1.0;	// Distance from loc either increasing (value 1) or decreasing (value -1)

	}	// end if animated

	toolObj = currObject = nObjects++;
	setToolCallbacks(adjustLocXZ, camRotZ(),
		adjustScaleY, glm::mat2(0.05, 0, 0, 10.0));

} // end addObject()

  //----------------------------------------------------------------------------
  // *BC* draw one model from the array of SceneObjects ... called by display()
  //-----------------------------------------------------------------------------
void drawMesh(SceneObject sceneObj, int outline)
// BC - added i as index to sceneObjects to enable changing the pose time
{
	// Activate a texture, loading if needed.
	loadTextureIfNotAlreadyLoaded(sceneObj.texId);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureIDs[sceneObj.texId]);

	// Texture 0 is the only texture type in this program, and is for the rgb
	glUniform1i(glGetUniformLocation(shaderProgram, "texture"), 0);

	// Set the texture scale for the shaders
	GLint texScale = glGetUniformLocation(shaderProgram, "texScale");
	if (texScale == -1) printf("Uniform variable \"texScale\" is not found");
	else glUniform1f(texScale, sceneObj.texScale);

	// Set the projection matrix for the shaders
	glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(projection));

	// Prepare the model transforms 
	glm::mat4 transModel	= glm::translate(glm::vec3(sceneObj.loc+sceneObj.walkVec));
	glm::mat4 rotateModel	= RotateZ(sceneObj.angles[2])*RotateY(-sceneObj.angles[1])*RotateX(sceneObj.angles[0]); //BC-2
	glm::mat4 scaleModel	= glm::scale(glm::vec3(sceneObj.scale, sceneObj.scale, sceneObj.scale));				//BC-2
	glm::mat4 model			= transModel*rotateModel*scaleModel;

	//Note "view" matrix is prepared in display()
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, glm::value_ptr(view * model));    CheckError();

	// Activate the objects VAO
	glBindVertexArray(vaoIDs[sceneObj.meshId]);    CheckError();

	//--------------------------------------------------------------------------
	// This part calculates the bone transform matrices
	// poseTime was updated by display() prior to calling drawMesh()

	int nBones = meshes[sceneObj.meshId]->mNumBones;
	if (nBones == 0)nBones = 1;  // If no bones, just a single identity matrix is used

	//Create an array of transforms - one for each bone  
	glm::mat4 * boneTransforms = new glm::mat4[nBones];  //can't allocate array dynamically in c++ (yes in C99)
	// std::vector<mat4> boneTransforms(nBones);   // vector version does not work yet

	// This function returns an array of transforms (one per bone) that is either a key frame or an interpolation between 2 key frames
	calculateAnimPose(meshes[sceneObj.meshId], scenes[sceneObj.meshId], 0, sceneObj.poseTime, boneTransforms);

	// now send the boneTransform matrix array to the shader
	glUniformMatrix4fv(uBoneTransforms, nBones, GL_FALSE, (const GLfloat *)boneTransforms); CheckError();

	// free the transform matrix array
	delete boneTransforms;

	// End of section to calculate bone transform array
	//---------------------------------------------------

	if (outline == 0) { // single draw with no outline
		glStencilMask(0x00);				// no stencil mask for floor or 3 lights ( all bits set to zero)
	}
	else {
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
	}

	// now do normal draw for everything
	glUniform1i(glGetUniformLocation(shaderProgram, "Outline"), 0); // let shader know to use normal colour
	glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3, GL_UNSIGNED_INT, NULL);

	// now redraw if highlighted
	if (outline == 1) {  // we have to redraw object slightly scaled up
		float outlineFactor = 1.01;
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilMask(0x00);
		//glDisable(GL_DEPTH_TEST);
		glm::mat4 outlineScale = glm::scale(glm::vec3(outlineFactor));

		model = transModel*rotateModel*scaleModel*outlineScale;   // now should be marginally bigger
		glUniformMatrix4fv(uModelView, 1, GL_FALSE, glm::value_ptr( view* model));

		glUniform1i(glGetUniformLocation(shaderProgram, "Outline"), 1); // let shader know to use highlight
		glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3, GL_UNSIGNED_INT, NULL);
		// repair the stencil mask and depth test for next object
		glStencilMask(0xFF);
		//glEnable(GL_DEPTH_TEST);
	}

	glBindVertexArray(0);  // unbind the vao

}

//------------------------------------------
// GLUT callbacks
//------------------------------------------

//------------------------------------------
//*BC* this GLUT callback redraws the scene
//------------------------------------------
void display(void)
{
	numDisplayCalls++;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	CheckError(); // May report a harmless GL_INVALID_OPERATION with GLEW on the first frame

	//*BC* here we add the calculate the camera rotation matrix (view is a mat4)
	glm::mat4 rotateCam = RotateX(camRotUpAndOverDeg)*RotateY(-camRotSidewaysDeg);  	//BC-1
	view = glm::translate(glm::vec3(0.0, 0.0,viewDist))*rotateCam;
	
	SceneObject lightObj1 = sceneObjs[1];  // BC - This is a point light
	SceneObject lightObj2 = sceneObjs[2];  // BC-8 - This is a directional light
	SceneObject lightObj3 = sceneObjs[3];  // BC-9 - This is a spotlight (point light with limited beam)

	glm::vec4 light1Position = view * lightObj1.loc;  	// *BC* light-1 position in eye coords
	glm::vec4 light2Direction = lightObj2.loc; 			// BC-8 light-2 direction in eye coords
	light2Direction.w = 0.0;  							// remove the w component for a direction vector
	light2Direction = view*light2Direction;				// direction in eye coordinates
	glm::vec4 light3Position = view * lightObj3.loc;  	// BC-9 light-3 position in eye coords
	// make position of light 2 the direction of the beam
	//vec4 SpotConeDirection = view*normalize(vec4(lightObj2.loc.x,lightObj2.loc.y,lightObj2.loc.z,0));
	glm::vec4 SpotConeDirection = view*glm::vec4(normalize(SpotConeDir), 0.0);

	float SpotBeamWidth = DegreesToRadians*SpotBeamWidthDegrees;  					// BC-9 beam-width of spotlight in degrees

	glUniform4fv(glGetUniformLocation(shaderProgram, "Light1Position"),    1, glm::value_ptr(light1Position));
	glUniform4fv(glGetUniformLocation(shaderProgram, "Light2Direction"),   1, glm::value_ptr(light2Direction));		//BC-8
	glUniform4fv(glGetUniformLocation(shaderProgram, "Light3Position"),    1, glm::value_ptr(light3Position));		//BC-9
	glUniform1f(glGetUniformLocation(shaderProgram, "SpotBeamWidth"), SpotBeamWidth); 								//BC-9
	glUniform4fv(glGetUniformLocation(shaderProgram, "SpotConeDirection"), 1, glm::value_ptr(SpotConeDirection)); 	//BC-9
																										// 
	glUniform3fv(glGetUniformLocation(shaderProgram, "Light1col"), 1, glm::value_ptr(lightObj1.rgb * lightObj1.brightness));  // BC-8
	glUniform3fv(glGetUniformLocation(shaderProgram, "Light2col"), 1, glm::value_ptr(lightObj2.rgb * lightObj2.brightness));  // BC-8
	glUniform3fv(glGetUniformLocation(shaderProgram, "Light3col"), 1, glm::value_ptr(lightObj3.rgb * lightObj3.brightness));  // BC-9

	glUniform1f(glGetUniformLocation(shaderProgram, "lightModel"), lightModel); // BC-10 - select lighting model
	CheckError();

	for (int i = 0; i < nObjects; i++) {  // draw each object in object array
		SceneObject so = sceneObjs[i];

		// *BC* This line changes the colour of the object according to the light colour
		// ie a red light would shade the object red.
		// However, this calculation should be moved to the shader to account for attenuation with distance 
		// was vec3 rgb = so.rgb * so.brightness * lightObj1.rgb * lightObj1.brightness * 2.0;
		glm::vec3 Obj_rgb = (so.rgb * so.brightness);

		// This conditional is test code to set the objects material parameters using keyboard	
		if (i == 4) {  // 0 = ground plane, 1= first light, 2 = 2nd light, 3 = first object
			glUniform3fv(glGetUniformLocation(shaderProgram, "AmbientProduct"),  1, glm::value_ptr(testAmbient * Obj_rgb));
			glUniform3fv(glGetUniformLocation(shaderProgram, "DiffuseProduct"),  1, glm::value_ptr(testDiffuse * Obj_rgb));
			glUniform3fv(glGetUniformLocation(shaderProgram, "SpecularProduct"), 1, glm::value_ptr(testSpecular * Obj_rgb));
			glUniform1f(glGetUniformLocation(shaderProgram, "Shininess"), testShininess);

		}
		else

		{ // *BC* block used to allow else clause if conditional used
			glUniform3fv(glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, glm::value_ptr(so.ambient * Obj_rgb));
			CheckError();
			glUniform3fv(glGetUniformLocation(shaderProgram, "DiffuseProduct"), 1, glm::value_ptr(so.diffuse * Obj_rgb));
			glUniform3fv(glGetUniformLocation(shaderProgram, "SpecularProduct"), 1, glm::value_ptr(so.specular * Obj_rgb));
			glUniform1f(glGetUniformLocation(shaderProgram, "Shininess"), so.shine);
			CheckError();
		}
		// *BC* debug
#ifdef DEBUG1	
		if (DEBUGdisplayed > 0) {
			printf("Object Loc x=%f ,y= %f,z= %f  Scale =%f\n", so.loc.x, so.loc.y, so.loc.z, so.scale);
			printf("Angles x=%f y=%f z=%f\n\n", so.angles[0], so.angles[1], so.angles[2]);
		}
#endif
		//------------------------------------------------------------------------------
		// Update the animation pose time and the walk position for this object
		if (so.numAnimations > 0) {
			if (deltaTime > 30) deltaTime = 30; // prevent too long deltas

			float deltaPoseTime = deltaTime / 1000 * so.TicksPerSec * so.AnimationSpeed;
			if (so.AnimationDirection == 0) { // step forward through the animations
				sceneObjs[i].poseTime += deltaPoseTime;
				if (so.poseTime > so.DurationInTicks) {
					sceneObjs[i].poseTime = 0; // restart the animations instead of cycling backwards
					//sceneObjs[i].poseTime -= 2 * deltaPoseTime;
					//sceneObjs[i].AnimationDirection = 1;
				}
			}
			else { // // step backward through the animation key-frames
				sceneObjs[i].poseTime -= deltaPoseTime;;
				if (so.poseTime < 0.0) {
					sceneObjs[i].poseTime += 2 * deltaPoseTime;;
					sceneObjs[i].AnimationDirection = 0;
				}
			} // end if

			  // now do the walking calculations
			if (sceneObjs[i].walkOrientation == 0) {
				sceneObjs[i].walkDistance += deltaTime / 1000 * sceneObjs[i].walkSpeed;
				if (sceneObjs[i].walkDistance > sceneObjs[i].walkLength) {
					sceneObjs[i].walkDistance -= 2 * deltaTime / 1000 * sceneObjs[i].walkSpeed;
					sceneObjs[i].walkOrientation = 1; // reverse
													  //sceneObjs[i].angles[1] = 0;    // reverse face direction
				}
			}
			else {
				sceneObjs[i].walkDistance -= deltaTime / 1000 * sceneObjs[i].walkSpeed;
				if (sceneObjs[i].walkDistance < 0) {
					sceneObjs[i].walkDistance += 2 * deltaTime / 1000 * sceneObjs[i].walkSpeed;
					sceneObjs[i].walkOrientation = 0; // reverse
													  //sceneObjs[i].angles[1] = 180; // restore face direction
				}
			}
			// now calculate the vector representing the walk from the objects rest pos
			sceneObjs[i].walkVec = glm::vec4(glm::normalize(sceneObjs[i].walkDir) * sceneObjs[i].walkDistance, 1);   // mismatch in vector sizes probably
			//printf("walk Distance %f\n",sceneObjs[1].walkDistance);

		}//end if animated  and end of Update the animation pose time and the walk position for this object
		 //------------------------------------------------------------------------------

		int outline;  // outline = 1 if object is to be highlighted
		if (toolObj == i && i> 3) outline = 1; else outline = 0; // outline the current object (1 to outline)
		drawMesh(sceneObjs[i], outline); // draw outlined or not

	} // END FOR
	DEBUGdisplayed = 0;
	glutSwapBuffers();
} // end display()
  //-----------------------
  //*BC* This glut callback is activated when a mouse key is pressed or scroll when wheel is turned.
  //*BC* First: to start a dragging session (via activateTool() ) when left or middle buttons are pressed
  //*BC*  and to stop the session when the button is released
  //*BC* Second: to scroll in or out via the viewDist variable  
  //-----------------------
static void mouseClickOrScroll(int button, int state, int x, int y)
{
	// *BC* added
	if (button == GLUT_RIGHT_BUTTON) { return; }   // to be removed later
	DEBUGdisplayed = 1;
	// *BC* end
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (glutGetModifiers() != GLUT_ACTIVE_SHIFT) activateTool(button);
		else activateTool(GLUT_LEFT_BUTTON);
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) deactivateTool();
	else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN) { activateTool(button); }
	else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_UP) deactivateTool();

	else if (button == 3) { // 3 is Mouse wheel forward - scroll in
		viewDist = (viewDist < 0.0 ? viewDist : viewDist*0.8) - 0.05;
		printf("BC scroll up .. viewDist=%f\n", viewDist);
	}
	else if (button == 4) { // 4 is Mouse wheel back - scroll out
		viewDist = (viewDist < 0.0 ? viewDist : viewDist*1.25) + 0.05;
		printf("BC scroll down .. viewDist=%f\n", viewDist);
	}
} // end mouseClickOrScroll()

  //----------------------------------------------------------------------------
  //*BC* this GLUT callback updates the current mouse position in display coordinates
  //*BC* x=0 is left,   y=0 is top 
  //----------------------------------------------------------------------------
static void mousePassiveMotion(int x, int y)
{
	mouseX = x;
	mouseY = y;
	//printf("BC passive mouse x=%d, y=%d\n",x,y);
} //end mousePassiveMotion()

  //----------------------------------------------------------------------------
  //*BC** This GLUT callback function just exits if ESC key on keyboard is pressed ( all other keys ignored)
  //----------------------------------------------------------------------------
#define ANGLE 45
void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		// BC-4 start
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		nearDist = 0.2 + float(key - '5') / 20;   // adjust the nearDist according to key
		printf("nearDist %f\n", nearDist);
		reshape(windowWidth, windowHeight);  // force recalculation of frustrum
		break;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
		nearFactor = 1.0 + float(key - 'e') / 4;   // adjust the nearFactor according to key
		printf("nearFactor %f\n", nearFactor);
		reshape(windowWidth, windowHeight);  // force recalculation of frustrum
		break;
	case 'A':
		if (testAmbient < .015) testAmbient = 1; else testAmbient = .01;
		printf("Ambient: %1.1f Specular: %1.1f Diffuse: %1.1f Shine %2.1f\n", testAmbient, testSpecular, testDiffuse, testShininess);
		break;
	case 'S':
		if (testSpecular < .015) testSpecular = 1; else testSpecular = .01;
		printf("Ambient: %1.1f Specular: %1.1f Diffuse: %1.1f Shine %2.1f\n", testAmbient, testSpecular, testDiffuse, testShininess);
		break;
	case 'D':
		if (testDiffuse < .015) testDiffuse = 1; else testDiffuse = .01;
		printf("Ambient: %1.1f Specular: %1.1f Diffuse: %1.1f Shine %2.1f\n", testAmbient, testSpecular, testDiffuse, testShininess);
		break;
	case 'F':
		if (testShininess < .015) testShininess = 20; else testShininess = .01;
		printf("Ambient: %1.1f Specular: %1.1f Diffuse: %1.1f Shine %2.1f\n", testAmbient, testSpecular, testDiffuse, testShininess);
		break;
	case 'm':
		if (lightModel < 1.99) lightModel = 2.0; else lightModel = 1.0;
		printf("light model = %s\n", (lightModel <1.99) ? "Phong" : "Binn Phong");
		break;
	case 'o':
		SpotBeamWidthDegrees -= 1;
		if (SpotBeamWidthDegrees < 1.0) SpotBeamWidthDegrees = 1.0;
		break;
	case 'p':
		SpotBeamWidthDegrees += 1;
		if (SpotBeamWidthDegrees > 90.0) SpotBeamWidthDegrees = 90.0;
		break;
	case 'j':
		SpotConeDir.x -= 0.3;
		break;
	case 'J':
		SpotConeDir.x += 0.3;
		break;
	case 'k':
		SpotConeDir.y -= 0.3;
		break;
	case 'K':
		SpotConeDir.y += 0.3;
		break;
	case 'l':
		SpotConeDir.z -= 0.3;
		break;
	case 'L':
		SpotConeDir.z += 0.3;
		break;
		// BC-4 end		 
	case 033:
		exit(EXIT_SUCCESS);
		break;
	}
} // end keyboard

  //----------------------------------------------------------------------------
  //*BC** This GLUT callback function is called when GLUT has no other callbacks pending
  // ----------------------------------------------------------------------------
void idle(void)
{
	// here we will add some code to only display every 20mS
	static int glut_old_elapsed_time = 0;
	int glut_new_elapsed_time = glutGet(GLUT_ELAPSED_TIME);			// milli-secs since GLUT was started
	deltaTime = glut_new_elapsed_time - glut_old_elapsed_time;		// deltaTime used by animations
																	// printf("new time %d, old time%d, deltaTime %f\n",glut_new_elapsed_time,glut_old_elapsed_time,deltaTime);
	glut_old_elapsed_time = glut_new_elapsed_time;

	glutPostRedisplay();
} // end idle()

  //----------------------------------------------------------------------------
  //*BC** This GLUT callback function just adjusts the screen size (and the projection matrix)
  //----------------------------------------------------------------------------
void reshape(int width, int height)
{

	std::cout << "Reshape() callback activated" << std::endl;

	windowWidth = width;
	windowHeight = height;

	glViewport(0, 0, width, height);

	// You'll need to modify this so that the view is similar to that in the
	// sample solution.
	// In particular: 
	//     - the view should include "closer" visible objects (slightly tricky)
	//     - when the width is less than the height, the view should adjust so
	//         that the same part of the scene is visible across the width of
	//         the window.

	// *BC-4* see http://www.lighthouse3d.com/tutorials/view-frustum-culling/ for a graphical explanation of frustrum
	GLfloat aspectRatio = (float)width / (float)height; // BC-4 created this variable for clarity only
														//BC-4 GLfloat nearDist = 0.2; // made this a global so can be changed by keyboard
	if (aspectRatio > 1.0) {// width > height
		/* SceneCamera.setFrustrum	(	-nearDist*aspectRatio / nearFactor,						// left side of near rectangle
										nearDist*aspectRatio / nearFactor,    					// right side of near rectangle
										-nearDist / nearFactor, 								// bottom of near rectangle
										nearDist / nearFactor,									// top of near rectangle
										(float)nearDist, (float)100.0);							// near and far plane distances from camera
        */
		projection = glm::frustum(-nearDist*aspectRatio / nearFactor,// left side of near rectangle
			nearDist*aspectRatio / nearFactor,    					// right side of near rectangle
			-nearDist / nearFactor, 								// bottom of near rectangle
			nearDist / nearFactor,									// top of near rectangle
			(float)nearDist, (float)100.0);							// near and far plane distances from camera
	}
	// BC-5
	else {  				// height > width
		projection = glm::frustum(-nearDist / nearFactor,   		// left 1/2 of near rectangle
			nearDist / nearFactor,    								// right 1/2 of near rectangle
			-nearDist / aspectRatio / nearFactor, 					// bottom 1/2 near rectangle
			nearDist / aspectRatio / nearFactor,	 				//top of 1/2 rectangle
			(float)nearDist, (float)100.0);							// near and far plane distances from camera
	}

} // end reshape()

  //----------------------------------------------------------------------------
  //*BC** This GLUT callback just updates window title (frame-rate, window size)
  //----------------------------------------------------------------------------
void timer(int unused)
{
	char title[256];
	sprintf_s(title, "%s %s: %d Frames Per Second @ %d x %d",
		lab, programName, numDisplayCalls, windowWidth, windowHeight);

	glutSetWindowTitle(title);

	numDisplayCalls = 0;
	glutTimerFunc(1000, timer, 1);
} // end timer

  //----------------------------------------------------------------------------
  // texture loading
  //----------------------------------------------------------------------------
  // *BC* Textures are loaded only when needed during a drawMesh() call - but when loaded they stay loaded
  // Loads a texture by number, and binds it for later use.    
void loadTextureIfNotAlreadyLoaded(int texNo)
{
	// this assumes the textures all have names like "textureTexNo.bmp"
	if (textureIDs[texNo] != NOT_LOADED) return; // The texture is already loaded.

	if (texNo < 0 || texNo >= numTextures)
		failInt("Error in loading texture - wrong texture number:", texNo);

	char fileName[220];
	sprintf_s(fileName, "%s/texture%d.bmp", dataDir, texNo);
	//sprintf_s(fileName, "C:\\MSProjects\\scene-start\\x64\\Debug\\dice.bmp");
	textureIDs[texNo] = Load_Texture(fileName);

	/*
	glActiveTexture(GL_TEXTURE0); CheckError();

	// Based on: http://www.opengl.org/wiki/Common_Mistakes
	glBindTexture(GL_TEXTURE_2D, textureIDs[i]); CheckError();

	glGenerateMipmap(GL_TEXTURE_2D); CheckError();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); CheckError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); CheckError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CheckError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); CheckError();

	glBindTexture(GL_TEXTURE_2D, 0); CheckError(); // Back to default texture
	*/
}
//---------------------------------------------------------------------------
// Mesh loading 
//---------------------------------------------------------------------------
// *BC* Meshes are loaded only when needed during a drawMesh() call - but when loaded they stay loaded
// *BC* an alternative strategy would have been to preload all meshes
// The following uses the Open Asset Importer library via loadMesh in 
// gnatidread.h to load models in .x format, including vertex positions, 
// normals, and texture coordinates.
// You shouldn't need to modify this - it's called from drawMesh below.

//-----------------------------------------------------------------------------

void loadMeshIfNotAlreadyLoaded(int meshNumber)
{
	printf("meshNumber %d\n", meshNumber);

	if (meshNumber > numMeshes || meshNumber < 0) {
		printf("Error - no such model number %d of %d\n", meshNumber, numMeshes);
		exit(1);
	}

	if (meshes[meshNumber] != NULL)
		return; // Already loaded

	printf("Starting LoadScene...\n");

	//aiMesh* mesh = loadMesh(meshNumber);  //This line replaced by part-2 lines below
	//meshes[meshNumber] = mesh; //This line replaced by part-2 lines below
	const aiScene* scene = loadScene(meshNumber); 	// Part -2
	scenes[meshNumber] = scene; 					// Part -2
	aiMesh* mesh = scene->mMeshes[0];			 	// Part -2
	meshes[meshNumber] = mesh;					 	// Part -2

	//*****************************************************************	
	
	// BC Part-2 start ( just some test code to sample the scene and mesh data)
	// BC note: Our program calls display objects "meshes" and so meshes[meshNumber] are program variables
	// These names "clash" with the assimp aiMesh,mNumMeMeshes and Meshes which refer to meshes within each aiScene
	printf("\nScene has %d meshes\n", scene->mNumMeshes);  // will be one for our models
	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		printf("Mesh %d has --\n", i);
		printf("    %d vertices\n", scene->mMeshes[i]->mNumVertices);
		printf("    %d faces (triangles etc)\n", scene->mMeshes[i]->mNumFaces);
		printf("    %d bones\n\n", scene->mMeshes[i]->mNumBones);
	}

	if (scene->mMeshes[0]->mNumBones > 0)
	{
		// print out the number of vertices affected by each bone
		float total_weight_vertex_777 = 0;
		for (int n = 0; n < (int)scene->mMeshes[0]->mNumBones; n++)
		{
			printf("No of vertices affected by bone[%d] = %d\n", n, scene->mMeshes[0]->mBones[n]->mNumWeights);

			for (int w = 0; w< (int)scene->mMeshes[0]->mBones[n]->mNumWeights; w++)
			{
				// print out the weights affecting vertex 777 (for example)
				if (scene->mMeshes[0]->mBones[n]->mWeights[w].mVertexId == 777) {
					printf("Weight of bone[%d] on vertex[%d] is %f\n",
						n,
						scene->mMeshes[0]->mBones[n]->mWeights[w].mVertexId,
						scene->mMeshes[0]->mBones[n]->mWeights[w].mWeight);
					total_weight_vertex_777 += scene->mMeshes[0]->mBones[n]->mWeights[w].mWeight;
				} // end if vertex == 777
			}// end for
		} // end for
		printf("total of bone weights for vertex 777 is %f - Should be total 1.0\n\n", total_weight_vertex_777);
	}// end if

	if (scene->HasAnimations()) { // this assumes only one animation per scene object
		DurationInTicks[meshNumber] = scene->mAnimations[0]->mDuration;
		TicksPerSec[meshNumber] = scene->mAnimations[0]->mTicksPerSecond;
		//		PoseTime[meshNumber] = 0.0;    // set the starting pose_time to 0 ( maybe needs to be > 0)
		printf("Object %d has %d animations\n", meshNumber, scene->mNumAnimations);
		printf("Animation duration in ticks %f\n", scene->mAnimations[0]->mDuration);
		printf("Animation ticks per Second %f\n\n", scene->mAnimations[0]->mTicksPerSecond);
	}
	else {
		DurationInTicks[meshNumber] = 0.0;
		TicksPerSec[meshNumber] = 0.0;
		//		PoseTime[meshNumber] = 0.0;    // set the starting pose_time to 0 ( maybe needs to be > 0)
		printf("Object %d has no animations\n\n", meshNumber);
	}
	//****************************** BC Part-2 end

	glBindVertexArray(vaoIDs[meshNumber]);

	// Create and initialize a buffer object for positions and texture coordinates, initially empty.
	// mesh->mTextureCoords[0] has space for up to 3 dimensions, but we only need 2.
	GLuint buffer;
	glGenBuffers(1, &buffer);  // buffer is the address of buffer[0]
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(3 + 3 + 3)*mesh->mNumVertices, NULL, GL_STATIC_DRAW);   // vertices, normals and tex coords in same buffer

	int nVerts = mesh->mNumVertices;
	// Next, we load the position, texCoords and normal data in parts.    
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * nVerts, mesh->mVertices);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 3 * nVerts, sizeof(float) * 3 * nVerts, mesh->mTextureCoords[0]);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 6 * nVerts, sizeof(float) * 3 * nVerts, mesh->mNormals);

	// Load the element index data from the assimp model into a local elements[] array
	GLuint * elements = new GLuint[mesh->mNumFaces * 3];

	//std::vector<GLuint> elements(mesh->mNumFaces * 3, 0);  / vector version not working

	for (GLuint i = 0; i < mesh->mNumFaces; i++) {
		elements[i * 3] = mesh->mFaces[i].mIndices[0];
		elements[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
		elements[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
	}
	// create a buffer for the indices and fill with indices data
	GLuint elementBufferId;
	glGenBuffers(1, &elementBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId);
	// note indices are integers
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh->mNumFaces * 3, elements, GL_STATIC_DRAW);

	// set the attribute pointers to correct values
	// vPosition it actually 4D - the conversion sets the fourth dimension (i.e. w) to 1.0                 
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	CheckError();
	glEnableVertexAttribArray(vPosition);
	CheckError();

	// vTexCoord is actually 2D - the third dimension is ignored (it's always 0.0)
	glVertexAttribPointer(vTexCoord, 3, GL_FLOAT, GL_FALSE, 0,	BUFFER_OFFSET(sizeof(float) * 3 * mesh->mNumVertices));
	CheckError();
	glEnableVertexAttribArray(vTexCoord);
	CheckError();

	// vNormal attribute pointer
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,	BUFFER_OFFSET(sizeof(float) * 6 * mesh->mNumVertices));
	CheckError();
	glEnableVertexAttribArray(vNormal);
	CheckError();

	//********** Part-2 start
	// Get boneIDs and boneWeights for each vertex from the imported mesh data

	// read HowToMake2DArrays.txt to explain the variable length arrays below
	// read HowTheBonesWork.txt to explain how the animations work
	
	// make temp arrays for boneIDs - max 4 bones per vertex (in C99 would be GLint boneIDs[mesh->mNumVertices][4];)
	GLint **boneIDs_ptrs = new GLint*[mesh->mNumVertices];	// an array of pointers to GLint
	GLint *boneIDs = new GLint[mesh->mNumVertices * 4];     // a contiguous array of GLints boneIDs         
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		boneIDs_ptrs[i] = &boneIDs[i * 4];					// all the pointers now have addresses of BoneID arrays[4]

	// make temp arrays for boneWeights - max 4 bones per vertex (in C99 would be GLint boneIDs[mesh->mNumVertices][4];)
	GLfloat **boneWeights_ptrs = new GLfloat*[mesh->mNumVertices];		// an array of pointers to GLint
	GLfloat *boneWeights = new GLfloat[mesh->mNumVertices * 4];			// a contiguous array of GLints boneIDs         
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		boneWeights_ptrs[i] = &boneWeights[i * 4];						// all the pointers now have addresses of boneWeight arrays[4]

	getBonesAffectingEachVertex(mesh, boneIDs_ptrs, boneWeights_ptrs);  // after this the arrays are loaded

	if (meshNumber == 55) { // 55 is sphere
		std::cout << std::endl << "bone IDs and weights.........." << std::endl;
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			for (int j = 0; j < 4; j++)
				std::cout << boneIDs_ptrs[i][j] << ":" << boneWeights_ptrs[i][j] << "  ";
			std::cout << std::endl;
		}
	}
	// if mesh->mNumBones = 0................
	// boneIDs[0] -> boneIDs[4] are set to 0 
	// boneWeights[0] is set to 1
	// boneWeights[1] -> boneWeights[3] are set to 0 
	// so code below should work irrespective of presence of bone data in the model
	printf("vBoneIDs  = %d\n", vBoneIDs);
	printf("vBoneWeights  = %d\n", vBoneWeights);

	GLuint buffers[2];
	glGenBuffers(2, buffers);  // Add two vertex buffer objects

	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]); CheckError();
	glBufferData(GL_ARRAY_BUFFER, sizeof(int) * 4 * mesh->mNumVertices, boneIDs, GL_STATIC_DRAW); CheckError();
	// index     size type               stride

	//printf("vBoneIDs  = %d\n",vBoneIDs);
	glVertexAttribPointer(vBoneIDs, 4, GL_INT, GL_FALSE, 0, BUFFER_OFFSET(0));  CheckError(); // Error
	glEnableVertexAttribArray(vBoneIDs);     CheckError();									        // Error

	glBindBuffer(GL_ARRAY_BUFFER, buffers[1]); CheckError();
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * mesh->mNumVertices, boneWeights, GL_STATIC_DRAW); CheckError();

	glVertexAttribPointer(vBoneWeights, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0)); CheckError();   //Error
	glEnableVertexAttribArray(vBoneWeights);    CheckError();	// Error

	glBindVertexArray(0);

	// free the array of elements
	delete[] elements;

	// free the boneIDs
	delete[] boneIDs;				// delete the arrays of GLInts
	delete[] boneIDs_ptrs;			// delete the array of pointers

									// free the boneWeights
	delete[] boneWeights;			// delete the arrays of GLfloats				
	delete[] boneWeights_ptrs;		// delete the array of pointers

	//********** Part-2 end
} //*BC* end loadMeshIfNotAlreadyLoaded

glm::mat4 RotateX(float angle_in_degrees)							// create a matrix to rotate around X axis
{
	float radians = DegreesToRadians*angle_in_degrees;
	return (glm::rotate(radians, glm::vec3(1, 0, 0)));
}
glm::mat4 RotateY(float angle_in_degrees)							// create a matrix to rotate around Y axis
{
	float radians = DegreesToRadians*angle_in_degrees;
	return (glm::rotate(radians, glm::vec3(0, 1, 0)));
}
glm::mat4 RotateZ(float angle_in_degrees)							// create a matrix to rotate around Z axis
{
	float radians = DegreesToRadians*angle_in_degrees;
	return (glm::rotate(radians, glm::vec3(0, 0, 1)));
}

  //----------------------------------------------------------------------------
  //Menu set up and menu functions
  //----------------------------------------------------------------------------
static void objectMenu(int id)
{
	deactivateTool();
	addObject(id);
}

static void texMenu(int id)
{
	deactivateTool();
	if (currObject >= 0) {
		sceneObjs[currObject].texId = id;
		glutPostRedisplay();
	}
}

static void groundMenu(int id)
{
	deactivateTool();
	sceneObjs[0].texId = id;
	glutPostRedisplay();
}

static void lightMenu(int id)
{
	deactivateTool();
	if (id == 70) {
		toolObj = 1;
		setToolCallbacks(adjustLocXZ, camRotZ(),
			adjustBrightnessY, glm::mat2(1.0, 0.0, 0.0, 10.0));

	}
	else if (id >= 71 && id <= 74) {    // *BC* not sure why not just id=71 as 72,73 and 74 are unused
		toolObj = 1;
		setToolCallbacks(adjustRedGreen, glm::mat2(1.0, 0, 0, 1.0),
			adjustBlueBrightness, glm::mat2(1.0, 0, 0, 1.0));
	}
	// BC-8 start
	else if (id == 80) {
		toolObj = 2;
		setToolCallbacks(adjustLocXZ, camRotZ(),
			adjustBrightnessY, glm::mat2(1.0, 0.0, 0.0, 10.0));

	}
	else if (id == 81) {
		toolObj = 2;
		setToolCallbacks(adjustRedGreen, glm::mat2(1.0, 0, 0, 1.0),
			adjustBlueBrightness, glm::mat2(1.0, 0, 0, 1.0));
	} // BC-8 end
	  // BC-8 start
	else if (id == 90) {
		toolObj = 3;
		setToolCallbacks(adjustLocXZ, camRotZ(),
			adjustBrightnessY, glm::mat2(1.0, 0.0, 0.0, 10.0));

	}
	else if (id == 91) {
		toolObj = 3;
		setToolCallbacks(adjustRedGreen, glm::mat2(1.0, 0, 0, 1.0),
			adjustBlueBrightness, glm::mat2(1.0, 0, 0, 1.0));
	} // BC-8 end

	else {
		printf("Error in lightMenu\n");
		exit(1);
	}
}

static int createArrayMenu(int size, const char menuEntries[][128], void(*menuFn)(int))
{
	int nSubMenus = (size - 1) / 10 + 1;

	// in C99 can have variable length arrays like ...  int subMenus[nSubMenus]; 
	// We can use vector of elements if not C99
	std::vector<int> subMenus(nSubMenus);     // this is equivalent to:   int subMenus[nSubMenus]; 

	for (int i = 0; i < nSubMenus; i++) {
		subMenus[i] = glutCreateMenu(menuFn);
		for (int j = i * 10 + 1; j <= glm::min(i * 10 + 10, size); j++)
			glutAddMenuEntry(menuEntries[j - 1], j);
		CheckError();
	}
	int menuId = glutCreateMenu(menuFn);

	for (int i = 0; i < nSubMenus; i++) {
		char num[6];
		sprintf_s(num, "%d-%d", i * 10 + 1, glm::min(i * 10 + 10, size));
		glutAddSubMenu(num, subMenus[i]);
		CheckError();
	}

	return menuId;
}

static void materialMenu(int id)
{
	deactivateTool();
	if (currObject < 0) return;
	if (id == 10) {
		toolObj = currObject;
		setToolCallbacks(adjustRedGreen, glm::mat2(1, 0, 0, 1),
			adjustBlueBrightness, glm::mat2(1, 0, 0, 1));
	}
	// BC-3  Start
	else if (id == 20) {
		toolObj = currObject;
		setToolCallbacks(adjustDiffuseAmbient, glm::mat2(1, 0, 0, 1),   		// *BC* scaling 0-1 
			adjustSpecShine, glm::mat2(1, 0, 0, 100));		 	// *BC* scaling 0-1 and 0-100
	}
	// BC-3 end
	// You'll need to fill in the remaining menu items here.                                                
	else {
		printf("Error in materialMenu\n");
	}
}

static void mainmenu(int id)
{
	deactivateTool();
	if (id == 41 && currObject >= 0) {
		toolObj = currObject;
		setToolCallbacks(adjustLocXZ, camRotZ(),
			adjustScaleY, glm::mat2(0.05, 0, 0, 10));
	}
	if (id == 50)
		doRotate();
	if (id == 55 && currObject >= 0) {
		setToolCallbacks(adjustAngleYX, glm::mat2(400, 0, 0, -400),
			adjustAngleZTexscale, glm::mat2(400, 0, 0, 15));
	}
	if (id == 99) exit(0);
}

static void makeMenu()
{
	int objectId = createArrayMenu(numMeshes-1, objectMenuEntries, objectMenu);

	int materialMenuId = glutCreateMenu(materialMenu);
	glutAddMenuEntry("R/G/B/All", 10);
	// glutAddMenuEntry("UNIMPLEMENTED: Ambient/Diffuse/Specular/Shine",20);
	glutAddMenuEntry("Ambient/Diffuse/Specular/Shine", 20); // *BC-3      

	int texMenuId = createArrayMenu(numTextures, textureMenuEntries, texMenu);
	int groundMenuId = createArrayMenu(numTextures, textureMenuEntries, groundMenu);

	int lightMenuId = glutCreateMenu(lightMenu);
	glutAddMenuEntry("Move Light 1", 70);
	glutAddMenuEntry("R/G/B/All Light 1", 71);
	glutAddMenuEntry("Move Light 2", 80);
	glutAddMenuEntry("R/G/B/All Light 2", 81);
	glutAddMenuEntry("Move Light 3", 90);
	glutAddMenuEntry("R/G/B/All Light 3", 91);

	glutCreateMenu(mainmenu);

	glutAddMenuEntry("Rotate/Move Camera", 50);

	glutAddSubMenu("Add object", objectId);
	glutAddMenuEntry("Position/Scale", 41);
	glutAddMenuEntry("Rotation/Texture Scale", 55);
	glutAddSubMenu("Material", materialMenuId);
	glutAddSubMenu("Texture", texMenuId);
	glutAddSubMenu("Ground Texture", groundMenuId);
	glutAddSubMenu("Lights", lightMenuId);
	glutAddMenuEntry("EXIT", 99);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}
//---------------------------
// helper functions
//---------------------------
void fileErr(char* fileName)
{
	printf("Error reading file: %s\n", fileName);
	printf("When not in the CSSE labs, you will need to include the directory containing\n");
	printf("the models on the command line, or put it in the same folder as the executable.");
	exit(1);
} // end fileErr()

