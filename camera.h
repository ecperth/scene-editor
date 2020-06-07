#pragma once
#include "SceneHeaders.h"

// This class prepares the transform matrices 
// for the projection ( ortho or perpective ) and for positioning the camera
// It also has member functions for panning, zooming and moving the camera to specific locations
// Step 1 - move all existing camera functions here

class camera
{
private:

public:

	camera();
	~camera();
	// sets a perspective projection matrix
	void setFrustrum (float Left_side, 	float right_side,    			// left and right side of near rectangle
					  float bottom,     float top,						// bottom and top of near rectangle
					  float nearDist,   float farDist);					// near and far plane distances from camera

	void camera::setPerspective(float Fov, float AspectRatio,    		// field of view and aspect ratio
								float nearDist, float farDist);			// near and far plane distances from camera

	void camera::setOrthographic(float Left_side, float right_side,    	// left and right side of near rectangle
								float bottom, float top,				// bottom and top of near rectangle
								float nearDist, float farDist);			// near and far plane distances from camera

	// sets the view matrix 
	void camera::setView(glm::vec3 position,			// point representing the camera location eg (0,0,-1)
						 glm::vec3 looktoPoint,			// point the camera is looking towards eg (0,0,0) or point+direction vector
						 glm::vec3 upVec);				// direction vector eg up (0,1,0) 		
	
	void zoom();						// zoom in or out
	void pan();							// pan left or right or up or down
	void rotate();						// rotate
	void move(int &loc);						//  move
	glm::mat4 getProjMatrix(void);    // return the projection matrix
	glm::mat4 getViewMatrix(void);		// return the view matrix

	glm::mat4 _projection;				// the projection matrix
	glm::mat4 _view;					// the view matrix

	glm::quat cameraRot;				// quaternion representing current rotation of camera
	glm::vec3 cameratLoc;				// Location of camera

	glm::vec3 loc;						// the location of the camera
	glm::vec3 up;						// the up direction of the camera
	glm::vec3 dir;						// the direction the camera is facing
};

