#ifdef _WIN32
#include <GL\glew.h>
#endif

#include "common.h"
#include <GL\GL.h>

int checkGLError(void)
{			
	int err = glGetError();
	if (err!=GL_NO_ERROR)
	{
		debug("Got GL Error: %d",err);
		switch (err) {
			case GL_INVALID_ENUM:
				debug("\tinvalid enum");
				break;
			case GL_INVALID_VALUE:
				debug("\tinvalid value");
				break;
			case GL_INVALID_OPERATION:
				debug("\tinvalid operation");
				break;
			case GL_STACK_OVERFLOW:
				debug("\tstack overflow");
				break;
			case GL_STACK_UNDERFLOW:
				debug("\tstack underflow");
				break;
			case GL_OUT_OF_MEMORY:
				debug("\tout of memory");
				break;
#ifdef GL_TABLE_TOO_LARGE
			case GL_TABLE_TOO_LARGE:
				debug("\ttable too large");
				break;
#endif
			default:
				debug("\tunrecognized");
				break;
		}
		return 1;
	}
	return 0;
}	

void printOpenGLInfo(void)
{ 
	const GLubyte 
		*vendor     = glGetString(GL_VENDOR),
		*renderer   = glGetString(GL_RENDERER),
		*version    = glGetString(GL_VERSION),
		*extensions = glGetString(GL_EXTENSIONS);
	debug("%s",vendor);
	debug("%s",renderer);
	debug("%s",version);
#ifdef _WIN32
  const GLubyte 
    *glew_version = glewGetString(GLEW_VERSION);
  debug("GLEW Version %s",glew_version);
#endif
#if 0
  foreach(QString extname,QString((char*)extensions).split(" "))
    debug()<<extname;
#endif
}
