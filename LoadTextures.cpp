#include "stdafx.h"	// this function loads one texture file

#include "SceneHeaders.h"

GLuint Load_Texture( const char *filename)
{
	GLuint textureID = SOIL_load_OGL_texture( filename, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	if(textureID == 0){
		printf("SOIL Error loading file %s\n",filename);
		getchar();
		exit(0);
	}
	// we can assume that the texture object is still bound after return from SOIL_load_OGL_texture()

	//glGenerateMipmap(GL_TEXTURE_2D);   // get OGL to create the mipmap instead of SOIL

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    glBindTexture( GL_TEXTURE_2D, 0 );  // unbind the texture
	
    return textureID;

} // end Load_Texture()