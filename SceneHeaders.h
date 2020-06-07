#pragma once
#include "stdafx.h"
#define GLEW_STATIC   // this forces static linking of GLEW.a library (as opposed to link at runtime)
#include <glew\glew.h>

//#define FREEGLUT_STATIC       // use the dll version of glut because some trouble with the static on Win
#include <freeglut\freeglut.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <ctype.h>		// for isprint()       REMOVE?
#include <windows.h>	// not sure if needed  REMOVE?

#include <ctime>		// for clock() 

#include <time.h>		//					   REMOVE?
#include <vector>		// needed for variable sized arrays

// Open Asset Importer header files (in ../../assimp--3.0.1270/include)
// This is a standard open source library for loading meshes, see gnatidread.h
#include <assimp\cimport.h>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

// NOTE: SOIL uses deprecated GL functions so must use COMPATIBILITY mode
#include <soil\SOIL.h> // this for the texture loading functions

// Include GLM  - the maths library (note: these are all just include files and contain the full class implementations)
#include <glm\glm.hpp>    // enables all the core in-line functions
#include <glm\gtx\transform.hpp>
#include <glm\gtc\matrix_transform.hpp>  
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\quaternion.hpp>   // basic quaternion functions
#include <glm\gtx\quaternion.hpp>   // extended quaternion functions

#ifndef __CHECKERROR_H__
#define __CHECKERROR_H__

//----------------------------------------------------------------------------

static const char*
ErrorString(GLenum error)
{
	const char*  msg = "";
	switch (error) {
#define Case( Token )  case Token: msg = #Token; break;
		Case(GL_NO_ERROR);
		Case(GL_INVALID_VALUE);
		Case(GL_INVALID_ENUM);
		Case(GL_INVALID_OPERATION);
		Case(GL_STACK_OVERFLOW);
		Case(GL_STACK_UNDERFLOW);
		Case(GL_OUT_OF_MEMORY);
#undef Case	
	}

	return msg;
}

//----------------------------------------------------------------------------

static void
_CheckError(const char* file, int line)
{
	GLenum  error;

	while ((error = glGetError()) != GL_NO_ERROR) {
		fprintf(stderr, "[%s:%d] %s\n", file, line, ErrorString(error));
		fflush(stderr);
	}


	//---- Original version that prints even when there's no error
	//
	// do {
	// fprintf( stderr, "[%s:%d] %s\n", file, line, ErrorString(error) );
	// } while ((error = glGetError()) != GL_NO_ERROR );

}

//----------------------------------------------------------------------------

#define CheckError()  _CheckError( __FILE__, __LINE__ )

//----------------------------------------------------------------------------

#endif // !__CHECKERROR_H__

//-----------------------------------------------------------
//  Define M_PI in the case it's not defined in the math header file
#ifndef M_PI
#  define M_PI  3.14159265358979323846
#endif
// Define a helpful macro for handling offsets into buffer objects
#define BUFFER_OFFSET( offset )   ((GLvoid*) (offset))
//  Helper function to load vertex and fragment shader files
GLuint InitShader(const char* vertexShaderFile, const char* fragmentShaderFile);

//  Defined constant for when numbers are too small to be used in the
//    denominator of a division operation.  This is only used if the
//    DEBUG macro is defined.
const GLfloat  DivideByZeroTolerance = GLfloat(1.0e-07);

//  Degrees-to-radians constant 
const GLfloat  DegreesToRadians = M_PI / GLfloat(180.0);

