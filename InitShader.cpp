#include "stdafx.h"
#include "SceneHeaders.h"

#pragma warning(disable : 4996)

// Create a NULL-terminated string by reading the provided file
static char*
readShaderSource(const char* shaderFile)
{
    FILE* fp = fopen(shaderFile, "r");

    if ( fp == NULL ) { return NULL; }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char* buf = new char[size + 1];
    size = fread(buf, 1, size, fp);

    buf[size] = '\0';
    fclose(fp);

	//printf("\n\nShader Text ......\n%s\n\n\n", buf);

    return buf;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint
InitShader(const char* vShaderFile, const char* fShaderFile)
{
    struct Shader { // define struct Shader
		const char*  filename;
		GLenum       type;
		GLchar*      source;
	};
	
	Shader shaders[2] = { // define and array of 2 shaders
		{ vShaderFile, GL_VERTEX_SHADER, NULL },
		{ fShaderFile, GL_FRAGMENT_SHADER, NULL }
    };

    GLuint program = glCreateProgram();
    
    for ( int i = 0; i < 2; ++i ) {
		Shader& s = shaders[i];
		s.source = readShaderSource( s.filename );
		if ( shaders[i].source == NULL ) {
			std::cerr << "Failed to read " << s.filename << std::endl;

			char ch;
			std::cout << "press any key (not ENTER) to continue...";
			std::cin >> ch;

			exit( EXIT_FAILURE );
		}

		GLuint shader = glCreateShader( s.type );
		glShaderSource( shader, 1, (const GLchar**) &s.source, NULL );
		glCompileShader( shader );

		GLint  compiled;
		glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
		if ( !compiled ) {
		  std::cerr << s.filename << " failed to compile:" << std::endl;
			GLint  logSize;
			glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logSize );
			char* logMsg = new char[logSize];
			glGetShaderInfoLog( shader, logSize, NULL, logMsg );
			std::cerr << logMsg << std::endl;
			delete [] logMsg;

			char ch;
			std::cout << "press any key (not ENTER) to continue...";
			std::cin >> ch;

			exit( EXIT_FAILURE );
		} // end if (!compiled)

		delete [] s.source;

		glAttachShader( program, shader );
    } // end for (each of 2 shader programs)

    /* link  and error check */
    glLinkProgram(program);

    GLint  linked;
    glGetProgramiv( program, GL_LINK_STATUS, &linked );
    if ( !linked ) {
		std::cerr << "Shader program failed to link" << std::endl;
		GLint  logSize;
		glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logSize);
		char* logMsg = new char[logSize];
		glGetProgramInfoLog( program, logSize, NULL, logMsg );
		std::cerr << logMsg << std::endl;
		delete [] logMsg;

		char ch;
		std::cout << "press any key  (not ENTER) to continue...";
		std::cin >> ch;

		exit( EXIT_FAILURE );
    }

    /* use program object */
    glUseProgram(program);

    return program;
} // end InitShader()
