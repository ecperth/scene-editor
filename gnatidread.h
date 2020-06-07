// UWA CITS3003 
// Graphics 'n Animation Tool Interface & Data Reader (gnatidread.h)

// You shouldn't need to modify the code in this file, but feel free to.
// If you do, it would be good to mark your changes with comments.

//#include "bitmap.h"

char dataDir[256];  // Stores the path to the models-textures folder.
const int numTextures = 30; 
const int numMeshes = 58;  // ie Mesh 0 to 57 - 0 is the ground plain and not one of the menu items

// A type for a 2D texture, with height and width in pixels
typedef struct {
    int height;
    int width;
    GLubyte *rgbData;   // Array of bytes with the colour data for the texture
} texture;

static int mouseX=0, mouseY=0;         	// raw int mouse coords Updated in the mouse-passive-motion function.
static glm::vec2 prevPos; 					// previous mouse pos in screen coords (0-1)
static glm::vec2 clickPrev;					// previous mouse pos in screen coords (0-1) (NOT USED)
static glm::mat2 leftTrans, middTrans;
static int currButton = -1; 			// current mouse button pressed

// -------------- Strings for the texture and mesh menus ---------------------------------
char textureMenuEntries[numTextures][128] = {
    "1 Plain", "2 Rust", "3 Concrete", "4 Carpet", "5 Beach Sand", 
    "6 Rocky", "7 Brick", "8 Water", "9 Paper", "10 Marble", 
    "11 Wood", "12 Scales", "13 Fur", "14 Denim", "15 Hessian",
    "16 Orange Peel", "17 Ice Crystals", "18 Grass", "19 Corrugated Iron", "20 Styrofoam",
    "21 Bubble Wrap", "22 Leather", "23 Camouflage", "24 Asphalt", "25 Scratched Ice",
    "26 Rattan", "27 Snow", "28 Dry Mud", "29 Old Concrete", "30 Leopard Skin"
};

char objectMenuEntries[numMeshes][128] = {
    "1 Thin Dinosaur","2 Big Dog","3 Saddle Dinosaur", "4 Dragon", "5 Cleopatra", 
    "6 Bone I", "7 Bone II", "8 Rabbit", "9 Long Dragon", "10 Buddha", 
    "11 Sitting Rabbit", "12 Frog", "13 Cow", "14 Monster", "15 Sea Horse", 
    "16 Head", "17 Pelican", "18 Horse", "19 Kneeling Angel", "20 Porsche I", 
    "21 Truck", "22 Statue of Liberty", "23 Sitting Angel", "24 Metal Part", "25 Car", 
    "26 Apatosaurus", "27 Airliner", "28 Motorbike", "29 Dolphin", "30 Spaceman", 
    "31 Winnie the Pooh", "32 Shark", "33 Crocodile", "34 Toddler", "35 Fat Dinosaur", 
    "36 Chihuahua", "37 Sabre-toothed Tiger", "38 Lioness", "39 Fish", "40 Horse (head down)", 
    "41 Horse (head up)", "42 Skull", "43 Fighter Jet I", "44 Toad", "45 Convertible", 
    "46 Porsche II", "47 Hare", "48 Vintage Car", "49 Fighter Jet II", "50 Gargoyle", 
    "51 Chef", "52 Parasaurolophus", "53 Rooster", "54 T-rex", "55 Sphere","56 Monkey","57 Man Walking"
};

// function prototypes
void fail(const char *, char *);					// error message and exit
void failInt(const char *, int);					// error message and exit
//texture* loadTexture(char *);						// load a texture from a file(low level)
//texture* loadTextureNum(int);						// load a texture number (low level)
void aiInit();										// initialise assimp (model loader)
aiMesh* loadMesh(int);								// load a specific mesh number (low level)
static glm::vec2 currMouseXYscreen(float x, float y);	// convert display (0-height, width) to screen coords (0-1)
static void doToolUpdateXY(int, int);				// passes new mouse location to active tool     
static glm::mat2 rotZ(float );							// returns a mat2 to rotate around z axis
static glm::vec2 currMouseXYworld(float rotSidewaysDeg);	// convert mouse to world coordinates
static void setToolCallbacks( void(*)(glm::vec2), glm::mat2, void(*)(glm::vec2), glm::mat2); // set the tool callback
static void doNothingCallback(glm::vec2);				// tool callback to do nothing (not really used)
static void activateTool(int button);				// activate tool when mouse is pressed
static void deactivateTool();						// deactivate tool when mouse is released

// ------Functions to fail with an error message then a string or int------ 
void fail(const char *msg1, char *msg2) {
    fprintf(stderr, "%s %s\n", msg1, msg2);
    exit(1);
}

void failInt(const char *msg1, int i) {
    fprintf(stderr, "%s %d\n", msg1, i);
    exit(1);
}

//----------------------------------------------------------------------------
// Initialise the Open Asset Importer toolkit
void aiInit()
{
    struct aiLogStream stream;

    // get a handle to the predefined STDOUT log stream and attach
    // it to the logging system. It remains active for all further
    // calls to aiImportFile(Ex) and aiApplyPostProcessing.
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
    aiAttachLogStream(&stream);

    // ... same procedure, but this stream now writes the
    // log messages to assimp_log.txt
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
    aiAttachLogStream(&stream);
}

// Load a mesh by number from the models-textures directory via the Open Asset Importer
aiMesh* loadMesh(int meshNumber) {
    char filename[256];
    sprintf_s(filename, "%s/model%d.x", dataDir, meshNumber);
    const aiScene* scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_Quality 
                                        | aiProcess_ConvertToLeftHanded);
    return scene->mMeshes[0];
}


//-----Code for using the mouse to adjust floats - you shouldn't need to modify this code.
// Calling setTool(vX, vY, vMat, wX, wY, wMat) below makes the left button adjust *vX and *vY
// as the mouse moves in the X and Y directions, via the transformation vMat which can be used
// for scaling and rotation. Similarly the middle button adjusts *wX and *wY via wMat.
// Any of vX, vY, wX, wY may be NULL in which case nothing is adjusted for that component.

static void doNothingCallback(glm::vec2 xy) { return; }

static void(*leftCallback)(glm::vec2) = &doNothingCallback;
static void(*middCallback)(glm::vec2) = &doNothingCallback;

static glm::vec2 currMouseXYscreen(float x, float y) {
    return glm::vec2( x/windowWidth, (windowHeight-y)/windowHeight );
} 

// *BC* This GLUT callback is called when the mouse is moved with a mouse key pressed
// *BC* It calls leftCallback() or middCallback() and passes the cursor change vector
static void doToolUpdateXY(int x, int y) { 
    if (currButton == GLUT_LEFT_BUTTON || currButton == GLUT_MIDDLE_BUTTON) {
		// currButton is -1 if the tool is inactive
		glm::vec2 currPos = glm::vec2(currMouseXYscreen(x,y));
        if (currButton==GLUT_LEFT_BUTTON)  {  
            leftCallback(leftTrans * (currPos - prevPos));
		}	
        else // (currButton==GLUT_MIDD_BUTTON)
            middCallback(middTrans * (currPos - prevPos));
            
        prevPos = currPos;
        glutPostRedisplay();
    }
}

// *BC* this function provides a mat2 to "correct" the xy screen coords when the camera is rotated around z axis
// - ie to rotate the  mouse coords back around the z axis. 
static glm::mat2 rotZ(float rotSidewaysDeg) {
	glm::mat4 rot4 = RotateZ(rotSidewaysDeg);
    return glm::mat2(rot4[0][0], rot4[0][1], rot4[1][0], rot4[1][1]);
}

//static vec2 currXY(float rotSidewaysDeg) { return rotZ(rotSidewaysDeg) * vec2(currRawX(), currRawY()); }
// *BC* calculate world coords from raw mouse YX coords
static glm::vec2 currMouseXYworld(float rotSidewaysDeg)
{ 
	glm::vec2 screenXY;  // *BC* added these variables to enable them to be printed
	glm::vec2 worldXY;
	screenXY = currMouseXYscreen(mouseX, mouseY);
	worldXY = rotZ(rotSidewaysDeg) * screenXY;
printf("Rotz degrees=%f   Display x=%d, y=%d  Screen x=%f, y=%f      World x=%f, y=%f  \n", \
        rotSidewaysDeg, mouseX, mouseY,screenXY.x,screenXY.y, worldXY.x, worldXY.y);	
	return worldXY;
}

// See the comment about 40 lines above
static void setToolCallbacks( void(*newLeftCallback)(glm::vec2 transformedMovement), glm::mat2 leftT,
                              void(*newMiddCallback)(glm::vec2 transformedMovement), glm::mat2 middT)
{
    leftCallback = newLeftCallback;
    leftTrans = leftT; 
    middCallback = newMiddCallback;
    middTrans = middT;

    currButton=-1;  // No current button to start with

    // std::cout << leftXYold << " " << middXYold << std::endl; // For debugging
}

static void activateTool(int button) {
    currButton = button;
    clickPrev = currMouseXYscreen(mouseX, mouseY);   // I believe this statement is redundant
	prevPos = clickPrev;

    std::cout << "*BC* - activateTool - at: x=" << mouseX << " y=" << mouseY << std::endl;  // For debugging
}

static void deactivateTool() {
    currButton=-1; 
	std::cout << "*BC* - deactivateTool" << std::endl;  // For debugging
}

//-------------------------------------------------------------
