#ifndef __glad_h_
#define __glad_h_

#ifdef __gl_h_
#error OpenGL header already included - include glad/glad.h first
#endif
#define __gl_h_

#include <KHR/khrplatform.h>
#include <stddef.h>

/* ── Types ───────────────────────────────────────────────────────────────── */
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef void           GLvoid;
typedef khronos_int8_t   GLbyte;
typedef khronos_uint8_t  GLubyte;
typedef short          GLshort;
typedef unsigned short GLushort;
typedef int            GLint;
typedef unsigned int   GLuint;
typedef int            GLsizei;
typedef khronos_float_t  GLfloat;
typedef double         GLdouble;
typedef char           GLchar;
typedef khronos_int64_t  GLint64;
typedef khronos_uint64_t GLuint64;
typedef khronos_intptr_t  GLintptr;
typedef khronos_ssize_t   GLsizeiptr;

/* ── Constants ───────────────────────────────────────────────────────────── */
#define GL_FALSE                         0
#define GL_TRUE                          1
#define GL_ZERO                          0
#define GL_ONE                           1
#define GL_NONE                          0

/* Primitives */
#define GL_POINTS                        0x0000
#define GL_LINES                         0x0001
#define GL_TRIANGLES                     0x0004

/* Data types */
#define GL_BYTE                          0x1400
#define GL_UNSIGNED_BYTE                 0x1401
#define GL_SHORT                         0x1402
#define GL_UNSIGNED_SHORT                0x1403
#define GL_INT                           0x1404
#define GL_UNSIGNED_INT                  0x1405
#define GL_FLOAT                         0x1406

/* Errors */
#define GL_NO_ERROR                      0
#define GL_INVALID_ENUM                  0x0500
#define GL_INVALID_VALUE                 0x0501
#define GL_INVALID_OPERATION             0x0502
#define GL_OUT_OF_MEMORY                 0x0505

/* Buffers / clear */
#define GL_COLOR_BUFFER_BIT              0x00004000
#define GL_DEPTH_BUFFER_BIT              0x00000100
#define GL_STENCIL_BUFFER_BIT            0x00000400

/* Enable caps */
#define GL_DEPTH_TEST                    0x0B71
#define GL_CULL_FACE                     0x0B44
#define GL_BLEND                         0x0BE2
#define GL_SCISSOR_TEST                  0x0C11
#define GL_LINE_SMOOTH                   0x0B20

/* Depth */
#define GL_LESS                          0x0201
#define GL_LEQUAL                        0x0203

/* Cull face / winding */
#define GL_FRONT                         0x0404
#define GL_BACK                          0x0405
#define GL_FRONT_AND_BACK                0x0408
#define GL_CW                            0x0900
#define GL_CCW                           0x0901

/* Blend */
#define GL_SRC_ALPHA                     0x0302
#define GL_ONE_MINUS_SRC_ALPHA           0x0303

/* Polygon */
#define GL_FILL                          0x1B02
#define GL_LINE                          0x1B01

/* Textures */
#define GL_TEXTURE_2D                    0x0DE1
#define GL_TEXTURE_WIDTH                 0x1000
#define GL_TEXTURE_HEIGHT                0x1001
#define GL_TEXTURE_MIN_FILTER            0x2801
#define GL_TEXTURE_MAG_FILTER            0x2800
#define GL_TEXTURE_WRAP_S                0x2802
#define GL_TEXTURE_WRAP_T                0x2803
#define GL_NEAREST                       0x2600
#define GL_LINEAR                        0x2601
#define GL_NEAREST_MIPMAP_NEAREST        0x2700
#define GL_LINEAR_MIPMAP_NEAREST         0x2701
#define GL_NEAREST_MIPMAP_LINEAR         0x2702
#define GL_LINEAR_MIPMAP_LINEAR          0x2703
#define GL_REPEAT                        0x2901
#define GL_CLAMP_TO_EDGE                 0x812F
#define GL_RGBA                          0x1908
#define GL_RGB                           0x1907
#define GL_RED                           0x1903
#define GL_RGBA8                         0x8058
#define GL_RGB8                          0x8051
#define GL_UNPACK_ALIGNMENT              0x0CF5

/* Texture units */
#define GL_TEXTURE0                      0x84C0
#define GL_TEXTURE1                      0x84C1
#define GL_TEXTURE2                      0x84C2

/* Buffer targets */
#define GL_ARRAY_BUFFER                  0x8892
#define GL_ELEMENT_ARRAY_BUFFER          0x8893

/* Buffer usage */
#define GL_STATIC_DRAW                   0x88B4
#define GL_DYNAMIC_DRAW                  0x88E8
#define GL_STREAM_DRAW                   0x88E0

/* Shader types */
#define GL_FRAGMENT_SHADER               0x8B30
#define GL_VERTEX_SHADER                 0x8B31

/* Shader / program status */
#define GL_COMPILE_STATUS                0x8B81
#define GL_LINK_STATUS                   0x8B82
#define GL_INFO_LOG_LENGTH               0x8B84
#define GL_VALIDATE_STATUS               0x8B83

/* Framebuffer / viewport */
#define GL_VIEWPORT                      0x0BA2

/* ── Function pointer typedefs ───────────────────────────────────────────── */
/* OpenGL 1.0 / 1.1 — loaded via GetProcAddress on Windows too via glad */
typedef void (KHRONOS_APIENTRYP PFNGLENABLEPROC)(GLenum cap);
typedef void (KHRONOS_APIENTRYP PFNGLDISABLEPROC)(GLenum cap);
typedef void (KHRONOS_APIENTRYP PFNGLCLEARPROC)(GLbitfield mask);
typedef void (KHRONOS_APIENTRYP PFNGLCLEARCOLORPROC)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
typedef void (KHRONOS_APIENTRYP PFNGLCLEARDEPTHPROC)(GLdouble depth);
typedef void (KHRONOS_APIENTRYP PFNGLDEPTHFUNCPROC)(GLenum func);
typedef void (KHRONOS_APIENTRYP PFNGLDEPTHMASKPROC)(GLboolean flag);
typedef void (KHRONOS_APIENTRYP PFNGLBLENDFUNCPROC)(GLenum sfactor, GLenum dfactor);
typedef void (KHRONOS_APIENTRYP PFNGLCULLFACEPROC)(GLenum mode);
typedef void (KHRONOS_APIENTRYP PFNGLFRONTFACEPROC)(GLenum mode);
typedef void (KHRONOS_APIENTRYP PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei w, GLsizei h);
typedef void (KHRONOS_APIENTRYP PFNGLSCISSORPROC)(GLint x, GLint y, GLsizei w, GLsizei h);
typedef GLenum (KHRONOS_APIENTRYP PFNGLGETERRORPROC)(void);
typedef void (KHRONOS_APIENTRYP PFNGLLINEWIDTHPROC)(GLfloat width);
typedef void (KHRONOS_APIENTRYP PFNGLPOLYGONMODEPROC)(GLenum face, GLenum mode);
typedef void (KHRONOS_APIENTRYP PFNGLDRAWARRAYSPROC)(GLenum mode, GLint first, GLsizei count);
typedef void (KHRONOS_APIENTRYP PFNGLDRAWELEMENTSPROC)(GLenum mode, GLsizei count, GLenum type, const void *indices);
typedef void (KHRONOS_APIENTRYP PFNGLGENTEXTURESPROC)(GLsizei n, GLuint *textures);
typedef void (KHRONOS_APIENTRYP PFNGLDELETETEXTURESPROC)(GLsizei n, const GLuint *textures);
typedef void (KHRONOS_APIENTRYP PFNGLBINDTEXTUREPROC)(GLenum target, GLuint texture);
typedef void (KHRONOS_APIENTRYP PFNGLTEXIMAGE2DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (KHRONOS_APIENTRYP PFNGLTEXPARAMETERIPROC)(GLenum target, GLenum pname, GLint param);
typedef void (KHRONOS_APIENTRYP PFNGLPIXELSTOREIPROC)(GLenum pname, GLint param);
typedef void (KHRONOS_APIENTRYP PFNGLGETINTEGERVPROC)(GLenum pname, GLint *data);

/* OpenGL 1.3 */
typedef void (KHRONOS_APIENTRYP PFNGLACTIVETEXTUREPROC)(GLenum texture);

/* OpenGL 1.5 */
typedef void (KHRONOS_APIENTRYP PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (KHRONOS_APIENTRYP PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (KHRONOS_APIENTRYP PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (KHRONOS_APIENTRYP PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (KHRONOS_APIENTRYP PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);

/* OpenGL 2.0 */
typedef GLuint (KHRONOS_APIENTRYP PFNGLCREATESHADERPROC)(GLenum type);
typedef void (KHRONOS_APIENTRYP PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (KHRONOS_APIENTRYP PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (KHRONOS_APIENTRYP PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (KHRONOS_APIENTRYP PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void (KHRONOS_APIENTRYP PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint (KHRONOS_APIENTRYP PFNGLCREATEPROGRAMPROC)(void);
typedef void (KHRONOS_APIENTRYP PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (KHRONOS_APIENTRYP PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (KHRONOS_APIENTRYP PFNGLDETACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (KHRONOS_APIENTRYP PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (KHRONOS_APIENTRYP PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (KHRONOS_APIENTRYP PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (KHRONOS_APIENTRYP PFNGLUSEPROGRAMPROC)(GLuint program);
typedef GLint (KHRONOS_APIENTRYP PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void (KHRONOS_APIENTRYP PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (KHRONOS_APIENTRYP PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (KHRONOS_APIENTRYP PFNGLUNIFORM2FVPROC)(GLint location, GLsizei count, const GLfloat *value);
typedef void (KHRONOS_APIENTRYP PFNGLUNIFORM3FVPROC)(GLint location, GLsizei count, const GLfloat *value);
typedef void (KHRONOS_APIENTRYP PFNGLUNIFORM4FVPROC)(GLint location, GLsizei count, const GLfloat *value);
typedef void (KHRONOS_APIENTRYP PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (KHRONOS_APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (KHRONOS_APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void (KHRONOS_APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);

/* OpenGL 3.0 */
typedef void (KHRONOS_APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void (KHRONOS_APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (KHRONOS_APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (KHRONOS_APIENTRYP PFNGLGENERATEMIPMAPPROC)(GLenum target);
typedef void (KHRONOS_APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);

/* ── Extern function pointer declarations ────────────────────────────────── */
extern PFNGLENABLEPROC                 glad_glEnable;
extern PFNGLDISABLEPROC                glad_glDisable;
extern PFNGLCLEARPROC                  glad_glClear;
extern PFNGLCLEARCOLORPROC             glad_glClearColor;
extern PFNGLCLEARDEPTHPROC             glad_glClearDepth;
extern PFNGLDEPTHFUNCPROC              glad_glDepthFunc;
extern PFNGLDEPTHMASKPROC              glad_glDepthMask;
extern PFNGLBLENDFUNCPROC              glad_glBlendFunc;
extern PFNGLCULLFACEPROC               glad_glCullFace;
extern PFNGLFRONTFACEPROC              glad_glFrontFace;
extern PFNGLVIEWPORTPROC               glad_glViewport;
extern PFNGLSCISSORPROC                glad_glScissor;
extern PFNGLGETERRORPROC               glad_glGetError;
extern PFNGLLINEWIDTHPROC              glad_glLineWidth;
extern PFNGLPOLYGONMODEPROC            glad_glPolygonMode;
extern PFNGLDRAWARRAYSPROC             glad_glDrawArrays;
extern PFNGLDRAWELEMENTSPROC           glad_glDrawElements;
extern PFNGLGENTEXTURESPROC            glad_glGenTextures;
extern PFNGLDELETETEXTURESPROC         glad_glDeleteTextures;
extern PFNGLBINDTEXTUREPROC            glad_glBindTexture;
extern PFNGLTEXIMAGE2DPROC             glad_glTexImage2D;
extern PFNGLTEXPARAMETERIPROC          glad_glTexParameteri;
extern PFNGLPIXELSTOREIPROC            glad_glPixelStorei;
extern PFNGLGETINTEGERVPROC            glad_glGetIntegerv;
extern PFNGLACTIVETEXTUREPROC          glad_glActiveTexture;
extern PFNGLGENBUFFERSPROC             glad_glGenBuffers;
extern PFNGLDELETEBUFFERSPROC          glad_glDeleteBuffers;
extern PFNGLBINDBUFFERPROC             glad_glBindBuffer;
extern PFNGLBUFFERDATAPROC             glad_glBufferData;
extern PFNGLBUFFERSUBDATAPROC          glad_glBufferSubData;
extern PFNGLCREATESHADERPROC           glad_glCreateShader;
extern PFNGLDELETESHADERPROC           glad_glDeleteShader;
extern PFNGLSHADERSOURCEPROC           glad_glShaderSource;
extern PFNGLCOMPILESHADERPROC          glad_glCompileShader;
extern PFNGLGETSHADERIVPROC            glad_glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC       glad_glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC          glad_glCreateProgram;
extern PFNGLDELETEPROGRAMPROC          glad_glDeleteProgram;
extern PFNGLATTACHSHADERPROC           glad_glAttachShader;
extern PFNGLDETACHSHADERPROC           glad_glDetachShader;
extern PFNGLLINKPROGRAMPROC            glad_glLinkProgram;
extern PFNGLGETPROGRAMIVPROC           glad_glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC      glad_glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC             glad_glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC     glad_glGetUniformLocation;
extern PFNGLUNIFORM1IPROC              glad_glUniform1i;
extern PFNGLUNIFORM1FPROC              glad_glUniform1f;
extern PFNGLUNIFORM2FVPROC             glad_glUniform2fv;
extern PFNGLUNIFORM3FVPROC             glad_glUniform3fv;
extern PFNGLUNIFORM4FVPROC             glad_glUniform4fv;
extern PFNGLUNIFORMMATRIX4FVPROC       glad_glUniformMatrix4fv;
extern PFNGLVERTEXATTRIBPOINTERPROC    glad_glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC  glad_glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray;
extern PFNGLGENVERTEXARRAYSPROC        glad_glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC     glad_glDeleteVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC        glad_glBindVertexArray;
extern PFNGLGENERATEMIPMAPPROC         glad_glGenerateMipmap;

/* ── Convenience macros (gl*() → glad_gl*()) ─────────────────────────────── */
#define glEnable                 glad_glEnable
#define glDisable                glad_glDisable
#define glClear                  glad_glClear
#define glClearColor             glad_glClearColor
#define glClearDepth             glad_glClearDepth
#define glDepthFunc              glad_glDepthFunc
#define glDepthMask              glad_glDepthMask
#define glBlendFunc              glad_glBlendFunc
#define glCullFace               glad_glCullFace
#define glFrontFace              glad_glFrontFace
#define glViewport               glad_glViewport
#define glScissor                glad_glScissor
#define glGetError               glad_glGetError
#define glLineWidth              glad_glLineWidth
#define glPolygonMode            glad_glPolygonMode
#define glDrawArrays             glad_glDrawArrays
#define glDrawElements           glad_glDrawElements
#define glGenTextures            glad_glGenTextures
#define glDeleteTextures         glad_glDeleteTextures
#define glBindTexture            glad_glBindTexture
#define glTexImage2D             glad_glTexImage2D
#define glTexParameteri          glad_glTexParameteri
#define glPixelStorei            glad_glPixelStorei
#define glGetIntegerv            glad_glGetIntegerv
#define glActiveTexture          glad_glActiveTexture
#define glGenBuffers             glad_glGenBuffers
#define glDeleteBuffers          glad_glDeleteBuffers
#define glBindBuffer             glad_glBindBuffer
#define glBufferData             glad_glBufferData
#define glBufferSubData          glad_glBufferSubData
#define glCreateShader           glad_glCreateShader
#define glDeleteShader           glad_glDeleteShader
#define glShaderSource           glad_glShaderSource
#define glCompileShader          glad_glCompileShader
#define glGetShaderiv            glad_glGetShaderiv
#define glGetShaderInfoLog       glad_glGetShaderInfoLog
#define glCreateProgram          glad_glCreateProgram
#define glDeleteProgram          glad_glDeleteProgram
#define glAttachShader           glad_glAttachShader
#define glDetachShader           glad_glDetachShader
#define glLinkProgram            glad_glLinkProgram
#define glGetProgramiv           glad_glGetProgramiv
#define glGetProgramInfoLog      glad_glGetProgramInfoLog
#define glUseProgram             glad_glUseProgram
#define glGetUniformLocation     glad_glGetUniformLocation
#define glUniform1i              glad_glUniform1i
#define glUniform1f              glad_glUniform1f
#define glUniform2fv             glad_glUniform2fv
#define glUniform3fv             glad_glUniform3fv
#define glUniform4fv             glad_glUniform4fv
#define glUniformMatrix4fv       glad_glUniformMatrix4fv
#define glVertexAttribPointer    glad_glVertexAttribPointer
#define glEnableVertexAttribArray  glad_glEnableVertexAttribArray
#define glDisableVertexAttribArray glad_glDisableVertexAttribArray
#define glGenVertexArrays        glad_glGenVertexArrays
#define glDeleteVertexArrays     glad_glDeleteVertexArrays
#define glBindVertexArray        glad_glBindVertexArray
#define glGenerateMipmap         glad_glGenerateMipmap

/* ── Loader ──────────────────────────────────────────────────────────────── */
#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*GLADloadproc)(const char *name);
int gladLoadGLLoader(GLADloadproc load);

#ifdef __cplusplus
}
#endif

#endif /* __glad_h_ */
