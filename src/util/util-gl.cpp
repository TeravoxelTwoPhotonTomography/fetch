// Glew must be included before OpenGL 
#ifdef _WIN32
#include <GL/glew.h>
#endif
#include <QtWidgets>

// On windows need to init glew library
#ifdef _WIN32
#include <assert.h>
#define INIT_GL_EXTENSIONS \
  assert(glewInit()==GLEW_OK)
#else
#define INIT_GL_EXTENSIONS
#endif



int checkGLError(void)
{			
  int err = glGetError();
  if (err!=GL_NO_ERROR)
  {
    qDebug("Got GL Error: %d",err);
    switch (err) {
    case GL_INVALID_ENUM:
      qDebug("\tinvalid enum");
      break;
    case GL_INVALID_VALUE:
      qDebug("\tinvalid value");
      break;
    case GL_INVALID_OPERATION:
      qDebug("\tinvalid operation");
      break;
    case GL_STACK_OVERFLOW:
      qDebug("\tstack overflow");
      break;
    case GL_STACK_UNDERFLOW:
      qDebug("\tstack underflow");
      break;
    case GL_OUT_OF_MEMORY:
      qDebug("\tout of memory");
      break;
#ifdef GL_TABLE_TOO_LARGE
    case GL_TABLE_TOO_LARGE:
      qDebug("\ttable too large");
      break;
#endif
    default:
      qDebug("\tunrecognized");
      break;
    }
    return 1;
  }
  //qDebug("Check GL Passed");
  return 0;
}	

void printOpenGLInfo(void)
{ 
  const GLubyte 
    *vendor     = glGetString(GL_VENDOR),
    *renderer   = glGetString(GL_RENDERER),
    *version    = glGetString(GL_VERSION);
#ifdef _WIN32
  const GLubyte 
    *extensions = glGetString(GL_EXTENSIONS);
#endif
  qDebug("%s",vendor);
  qDebug("%s",renderer);
  qDebug("%s",version);
#ifdef _WIN32
  const GLubyte 
    *glew_version = glewGetString(GLEW_VERSION);
  qDebug("GLEW Version %s",glew_version);
#endif
#if 0
  foreach(QString extname,QString((char*)extensions).split(" "))
    qDebug()<<extname;
#endif
}

static int g_gl_inited = 0;
void initOpenGLSentinal( void )
{
  if(!g_gl_inited)
  {
    INIT_GL_EXTENSIONS;
    printOpenGLInfo();
    checkGLError();
    g_gl_inited = 1;
  }
}
