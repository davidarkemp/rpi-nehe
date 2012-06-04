/*
* Please do what you want with this code. 
* By using this code, you agree that I am not responsible for
* anything that happens due to you using it.
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include "bcm_host.h"

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

typedef struct
{
   uint32_t screen_width;
   uint32_t screen_height;
// OpenGL|ES objects
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;

   GLuint verbose;
   GLuint vshader;
   GLuint fshader;
   GLuint mshader;
   GLuint program;
   GLuint program2;
   GLuint tex_fb;
   GLuint tex;
   GLuint buf;
// julia attribs
   GLuint unif_color, attr_vertex, unif_scale, unif_offset, unif_tex, unif_centre; 
// mandelbrot attribs
   GLuint attr_vertex2, unif_scale2, unif_offset2, unif_centre2;
} CUBE_STATE_T;

static CUBE_STATE_T _state, *state=&_state;

#define check() assert(glGetError() == 0)

static void showlog(GLint shader)
{
   // Prints the compile log for a shader
   char log[1024];
   glGetShaderInfoLog(shader,sizeof log,NULL,log);
   printf("%d:shader:\n%s\n", shader, log);
}

static void showprogramlog(GLint shader)
{
   // Prints the information log for a program object
   char log[1024];
   glGetProgramInfoLog(shader,sizeof log,NULL,log);
   printf("%d:program:\n%s\n", shader, log);
}
    
/***********************************************************
 * Name: init_ogl
 *
 * Arguments:
 *       CUBE_STATE_T *state - holds OGLES model info
 *
 * Description: Sets the display, OpenGL|ES context and screen stuff
 *
 * Returns: void
 *
 ***********************************************************/
static void init_ogl(CUBE_STATE_T *state)
{
   int32_t success = 0;
   EGLBoolean result;
   EGLint num_config;

   static EGL_DISPMANX_WINDOW_T nativewindow;

   DISPMANX_ELEMENT_HANDLE_T dispman_element;
   DISPMANX_DISPLAY_HANDLE_T dispman_display;
   DISPMANX_UPDATE_HANDLE_T dispman_update;
   VC_RECT_T dst_rect;
   VC_RECT_T src_rect;

   static const EGLint attribute_list[] =
   {
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_NONE
   };
   
   static const EGLint context_attributes[] = 
   {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
   };
   EGLConfig config;

   // get an EGL display connection
   state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(state->display!=EGL_NO_DISPLAY);
   check();

   // initialize the EGL display connection
   result = eglInitialize(state->display, NULL, NULL);
   assert(EGL_FALSE != result);
   check();

   // get an appropriate EGL frame buffer configuration
   result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
   assert(EGL_FALSE != result);
   check();

   // get an appropriate EGL frame buffer configuration
   result = eglBindAPI(EGL_OPENGL_ES_API);
   assert(EGL_FALSE != result);
   check();

   // create an EGL rendering context
   state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
   assert(state->context!=EGL_NO_CONTEXT);
   check();

   // create an EGL window surface
   success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
   assert( success >= 0 );

   dst_rect.x = 0;
   dst_rect.y = 0;
   dst_rect.width = state->screen_width;
   dst_rect.height = state->screen_height;
      
   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.width = state->screen_width << 16;
   src_rect.height = state->screen_height << 16;        

   dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
   dispman_update = vc_dispmanx_update_start( 0 );
         
   dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
      0/*layer*/, &dst_rect, 0/*src*/,
      &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
      
   nativewindow.element = dispman_element;
   nativewindow.width = state->screen_width;
   nativewindow.height = state->screen_height;
   vc_dispmanx_update_submit_sync( dispman_update );
      
   check();

   state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
   assert(state->surface != EGL_NO_SURFACE);
   check();

   // connect the context to the surface
   result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
   assert(EGL_FALSE != result);
   check();

   // Set background color and clear buffers
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //solid black
   glClear( GL_COLOR_BUFFER_BIT );

   check();
}

GLuint LoadShader(const char *source, GLenum type)
{
   GLuint shader = glCreateShader(type);
   check();

   glShaderSource(shader, 1, &source, NULL);
   check();

   glCompileShader(shader);
   check();

   GLint compiled;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
   if(compiled != GL_TRUE)
   {
   	showlog(shader);
	glDeleteShader(shader);
	return 0;	
   }

   return shader;
}

//==============================================================================

int main ()
{
   int terminate = 1;
   bcm_host_init();

   // Clear application state
   memset( state, 0, sizeof( *state ) );
      
   // Start OGLES
   init_ogl(state);
   //init_shaders(state);

   char vShaderSource[] =
	   "attribute vec4 vPosition; \n"
          // "uniform float offset; \n"
	   "void main() \n"
	   "{ \n"
	   //" vec4 nPosition = (vPosition+offset); \n"
	   " gl_Position = vPosition; \n"
	   "} \n";

   //everything is black
   char fShaderSource[] = 
   	"precision mediump float; \n"
	"void main() \n"
	"{ \n"
	" gl_FragColor = vec4(1.0, 1.0, 1.0, 0.5); \n"
	"} \n";



   GLuint vshader = LoadShader(vShaderSource, GL_VERTEX_SHADER);
   GLuint fshader = LoadShader(fShaderSource, GL_FRAGMENT_SHADER);

   GLuint programObj = glCreateProgram();
   GLint linked;
   check();
   assert(programObj != 0);

   glAttachShader(programObj, vshader);
   glAttachShader(programObj, fshader);

   glBindAttribLocation(programObj, 0, "vPosition");
   check();

   glLinkProgram(programObj);
   check();


   glGetProgramiv(programObj, GL_LINK_STATUS, &linked);
   if(linked != GL_TRUE)
   {
   	showprogramlog(programObj);
	glDeleteProgram(programObj);
	return -1;
   }
//   GLint offset = glGetUniformLocation(programObj, "offset");
   check();

   GLfloat vVertices[] = 
   	{ 0.25f, -0.25f, 0.0f,
	  0.5f,  0.25f, 0.0f,
	  0.75f, -0.25f, 0.0f
	  
	};

   glViewport(0,0, state->screen_width, state->screen_height);
   glClear(GL_COLOR_BUFFER_BIT);

	
   glUseProgram(programObj);

   //glUniform1f(offset, 0.5);
   glVertexAttribPointer(0,3, GL_FLOAT, GL_FALSE, 0, vVertices);
   glEnableVertexAttribArray(0);
   glDrawArrays(GL_TRIANGLES, 0, 3);
   glDisableVertexAttribArray(0);



   GLfloat square[] = 
   	{ 
          -0.75f, -0.25f, 0.0f, // Bottom Left
	  -0.25f, -0.25f, 0.0f, // Bottom Right
          -0.75f,  0.25f, 0.0f, // Top Left
	  -0.25f,  0.25f, 0.0f  //Top Right
	};
   //glUniform1f(offset, -0.5);
   glVertexAttribPointer(0,3, GL_FLOAT, GL_FALSE, 0, square);
   glEnableVertexAttribArray(0);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
   glDisableVertexAttribArray(0);

   eglSwapBuffers(state->display, state->surface);
   			

   getchar();
   while (!terminate)
   {
   }
   return 0;
}

