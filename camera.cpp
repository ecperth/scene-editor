#include "stdafx.h"
#include "camera.h"

camera::camera()
{
	// start with identify matrices
	_projection = glm::mat4(1); 
	_view = glm::mat4(1);
}

camera::~camera()
{
	// no clean up required
}

// sets a perspective projection matrix - using glm::frustum()
// NOTE: left and right are the sizes of the left and right sections of the front face
//       top and bottom are the sizes of the top and bottom sections of the front face
void camera::setFrustrum(float Left_side, float right_side,    		// left and right side of near rectangle
						 float bottom,    float top,				// bottom and top of near rectangle
						 float nearDist,  float farDist)			// near and far plane distances from camera
{
	_projection = glm::frustum(Left_side, right_side,bottom, top,nearDist, farDist);
}
// sets an orthographic projection matrix - using glm::frustum()
void camera::setOrthographic(float Left_side, float right_side,    	// left and right side of near rectangle
							 float bottom, float top,				// bottom and top of near rectangle
							 float nearDist, float farDist)			// near and far plane distances from camera
{
	_projection = glm::ortho(Left_side, right_side, bottom, top, nearDist, farDist);
}

// sets a perspective projection matrix - using glm::perspective()
void camera::setPerspective(float Fov, float AspectRatio,    	// field of view and aspect ratio
							float nearDist, float farDist)		// near and far plane distances from camera
{
	_projection = glm::perspective(Fov, AspectRatio, nearDist, farDist);
}

void camera::setView(glm::vec3 position,			// point representing the camera location eg (0,0,-1)
					glm::vec3 looktoPoint,			// point the camera is looking towards eg (0,0,0) or point+direction vector
					glm::vec3 upVec)					// direction vector eg up (0,1,0) 
{
	_view = glm::lookAt(position, looktoPoint, upVec);
}

glm::mat4 camera::getProjMatrix(void) {
	return _projection;											// return the projection matrix
}    
glm::mat4 camera::getViewMatrix(void)  {
	return _view;												// return the view matrix
}		    

void camera::zoom() {}											// zoom in or out
void camera::pan() {}											// pan left or right or up or down
void camera::rotate() {}										// rotate
void camera::move(int &loc) { loc = x; }						//  move

