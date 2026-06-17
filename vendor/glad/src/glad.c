#include <glad/glad.h>
#include <string.h>

/* ── Function pointer definitions ────────────────────────────────────────── */
PFNGLENABLEPROC                  glad_glEnable                  = NULL;
PFNGLDISABLEPROC                 glad_glDisable                 = NULL;
PFNGLCLEARPROC                   glad_glClear                   = NULL;
PFNGLCLEARCOLORPROC              glad_glClearColor              = NULL;
PFNGLCLEARDEPTHPROC              glad_glClearDepth              = NULL;
PFNGLDEPTHFUNCPROC               glad_glDepthFunc               = NULL;
PFNGLDEPTHMASKPROC               glad_glDepthMask               = NULL;
PFNGLBLENDFUNCPROC               glad_glBlendFunc               = NULL;
PFNGLCULLFACEPROC                glad_glCullFace                = NULL;
PFNGLFRONTFACEPROC               glad_glFrontFace               = NULL;
PFNGLVIEWPORTPROC                glad_glViewport                = NULL;
PFNGLSCISSORPROC                 glad_glScissor                 = NULL;
PFNGLGETERRORPROC                glad_glGetError                = NULL;
PFNGLLINEWIDTHPROC               glad_glLineWidth               = NULL;
PFNGLPOLYGONMODEPROC             glad_glPolygonMode             = NULL;
PFNGLDRAWARRAYSPROC              glad_glDrawArrays              = NULL;
PFNGLDRAWELEMENTSPROC            glad_glDrawElements            = NULL;
PFNGLGENTEXTURESPROC             glad_glGenTextures             = NULL;
PFNGLDELETETEXTURESPROC          glad_glDeleteTextures          = NULL;
PFNGLBINDTEXTUREPROC             glad_glBindTexture             = NULL;
PFNGLTEXIMAGE2DPROC              glad_glTexImage2D              = NULL;
PFNGLTEXPARAMETERIPROC           glad_glTexParameteri           = NULL;
PFNGLPIXELSTOREIPROC             glad_glPixelStorei             = NULL;
PFNGLGETINTEGERVPROC             glad_glGetIntegerv             = NULL;
PFNGLACTIVETEXTUREPROC           glad_glActiveTexture           = NULL;
PFNGLGENBUFFERSPROC              glad_glGenBuffers              = NULL;
PFNGLDELETEBUFFERSPROC           glad_glDeleteBuffers           = NULL;
PFNGLBINDBUFFERPROC              glad_glBindBuffer              = NULL;
PFNGLBUFFERDATAPROC              glad_glBufferData              = NULL;
PFNGLBUFFERSUBDATAPROC           glad_glBufferSubData           = NULL;
PFNGLCREATESHADERPROC            glad_glCreateShader            = NULL;
PFNGLDELETESHADERPROC            glad_glDeleteShader            = NULL;
PFNGLSHADERSOURCEPROC            glad_glShaderSource            = NULL;
PFNGLCOMPILESHADERPROC           glad_glCompileShader           = NULL;
PFNGLGETSHADERIVPROC             glad_glGetShaderiv             = NULL;
PFNGLGETSHADERINFOLOGPROC        glad_glGetShaderInfoLog        = NULL;
PFNGLCREATEPROGRAMPROC           glad_glCreateProgram           = NULL;
PFNGLDELETEPROGRAMPROC           glad_glDeleteProgram           = NULL;
PFNGLATTACHSHADERPROC            glad_glAttachShader            = NULL;
PFNGLDETACHSHADERPROC            glad_glDetachShader            = NULL;
PFNGLLINKPROGRAMPROC             glad_glLinkProgram             = NULL;
PFNGLGETPROGRAMIVPROC            glad_glGetProgramiv            = NULL;
PFNGLGETPROGRAMINFOLOGPROC       glad_glGetProgramInfoLog       = NULL;
PFNGLUSEPROGRAMPROC              glad_glUseProgram              = NULL;
PFNGLGETUNIFORMLOCATIONPROC      glad_glGetUniformLocation      = NULL;
PFNGLUNIFORM1IPROC               glad_glUniform1i               = NULL;
PFNGLUNIFORM1FPROC               glad_glUniform1f               = NULL;
PFNGLUNIFORM2FVPROC              glad_glUniform2fv              = NULL;
PFNGLUNIFORM3FVPROC              glad_glUniform3fv              = NULL;
PFNGLUNIFORM4FVPROC              glad_glUniform4fv              = NULL;
PFNGLUNIFORMMATRIX4FVPROC        glad_glUniformMatrix4fv        = NULL;
PFNGLVERTEXATTRIBPOINTERPROC     glad_glVertexAttribPointer     = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC  glad_glEnableVertexAttribArray  = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray = NULL;
PFNGLGENVERTEXARRAYSPROC         glad_glGenVertexArrays         = NULL;
PFNGLDELETEVERTEXARRAYSPROC      glad_glDeleteVertexArrays      = NULL;
PFNGLBINDVERTEXARRAYPROC         glad_glBindVertexArray         = NULL;
PFNGLGENERATEMIPMAPPROC          glad_glGenerateMipmap          = NULL;

/* ── Loader ───────────────────────────────────────────────────────────────── */
static GLADloadproc s_load = NULL;

static void *get_proc(const char *name) {
    return s_load(name);
}

#define LOAD(var, type, name) var = (type)get_proc(name)

int gladLoadGLLoader(GLADloadproc load) {
    if (!load) return 0;
    s_load = load;

    LOAD(glad_glEnable,                  PFNGLENABLEPROC,                  "glEnable");
    LOAD(glad_glDisable,                 PFNGLDISABLEPROC,                 "glDisable");
    LOAD(glad_glClear,                   PFNGLCLEARPROC,                   "glClear");
    LOAD(glad_glClearColor,              PFNGLCLEARCOLORPROC,              "glClearColor");
    LOAD(glad_glClearDepth,              PFNGLCLEARDEPTHPROC,              "glClearDepth");
    LOAD(glad_glDepthFunc,               PFNGLDEPTHFUNCPROC,               "glDepthFunc");
    LOAD(glad_glDepthMask,               PFNGLDEPTHMASKPROC,               "glDepthMask");
    LOAD(glad_glBlendFunc,               PFNGLBLENDFUNCPROC,               "glBlendFunc");
    LOAD(glad_glCullFace,                PFNGLCULLFACEPROC,                "glCullFace");
    LOAD(glad_glFrontFace,               PFNGLFRONTFACEPROC,               "glFrontFace");
    LOAD(glad_glViewport,                PFNGLVIEWPORTPROC,                "glViewport");
    LOAD(glad_glScissor,                 PFNGLSCISSORPROC,                 "glScissor");
    LOAD(glad_glGetError,                PFNGLGETERRORPROC,                "glGetError");
    LOAD(glad_glLineWidth,               PFNGLLINEWIDTHPROC,               "glLineWidth");
    LOAD(glad_glPolygonMode,             PFNGLPOLYGONMODEPROC,             "glPolygonMode");
    LOAD(glad_glDrawArrays,              PFNGLDRAWARRAYSPROC,              "glDrawArrays");
    LOAD(glad_glDrawElements,            PFNGLDRAWELEMENTSPROC,            "glDrawElements");
    LOAD(glad_glGenTextures,             PFNGLGENTEXTURESPROC,             "glGenTextures");
    LOAD(glad_glDeleteTextures,          PFNGLDELETETEXTURESPROC,          "glDeleteTextures");
    LOAD(glad_glBindTexture,             PFNGLBINDTEXTUREPROC,             "glBindTexture");
    LOAD(glad_glTexImage2D,              PFNGLTEXIMAGE2DPROC,              "glTexImage2D");
    LOAD(glad_glTexParameteri,           PFNGLTEXPARAMETERIPROC,           "glTexParameteri");
    LOAD(glad_glPixelStorei,             PFNGLPIXELSTOREIPROC,             "glPixelStorei");
    LOAD(glad_glGetIntegerv,             PFNGLGETINTEGERVPROC,             "glGetIntegerv");
    LOAD(glad_glActiveTexture,           PFNGLACTIVETEXTUREPROC,           "glActiveTexture");
    LOAD(glad_glGenBuffers,              PFNGLGENBUFFERSPROC,              "glGenBuffers");
    LOAD(glad_glDeleteBuffers,           PFNGLDELETEBUFFERSPROC,           "glDeleteBuffers");
    LOAD(glad_glBindBuffer,              PFNGLBINDBUFFERPROC,              "glBindBuffer");
    LOAD(glad_glBufferData,              PFNGLBUFFERDATAPROC,              "glBufferData");
    LOAD(glad_glBufferSubData,           PFNGLBUFFERSUBDATAPROC,           "glBufferSubData");
    LOAD(glad_glCreateShader,            PFNGLCREATESHADERPROC,            "glCreateShader");
    LOAD(glad_glDeleteShader,            PFNGLDELETESHADERPROC,            "glDeleteShader");
    LOAD(glad_glShaderSource,            PFNGLSHADERSOURCEPROC,            "glShaderSource");
    LOAD(glad_glCompileShader,           PFNGLCOMPILESHADERPROC,           "glCompileShader");
    LOAD(glad_glGetShaderiv,             PFNGLGETSHADERIVPROC,             "glGetShaderiv");
    LOAD(glad_glGetShaderInfoLog,        PFNGLGETSHADERINFOLOGPROC,        "glGetShaderInfoLog");
    LOAD(glad_glCreateProgram,           PFNGLCREATEPROGRAMPROC,           "glCreateProgram");
    LOAD(glad_glDeleteProgram,           PFNGLDELETEPROGRAMPROC,           "glDeleteProgram");
    LOAD(glad_glAttachShader,            PFNGLATTACHSHADERPROC,            "glAttachShader");
    LOAD(glad_glDetachShader,            PFNGLDETACHSHADERPROC,            "glDetachShader");
    LOAD(glad_glLinkProgram,             PFNGLLINKPROGRAMPROC,             "glLinkProgram");
    LOAD(glad_glGetProgramiv,            PFNGLGETPROGRAMIVPROC,            "glGetProgramiv");
    LOAD(glad_glGetProgramInfoLog,       PFNGLGETPROGRAMINFOLOGPROC,       "glGetProgramInfoLog");
    LOAD(glad_glUseProgram,              PFNGLUSEPROGRAMPROC,              "glUseProgram");
    LOAD(glad_glGetUniformLocation,      PFNGLGETUNIFORMLOCATIONPROC,      "glGetUniformLocation");
    LOAD(glad_glUniform1i,               PFNGLUNIFORM1IPROC,               "glUniform1i");
    LOAD(glad_glUniform1f,               PFNGLUNIFORM1FPROC,               "glUniform1f");
    LOAD(glad_glUniform2fv,              PFNGLUNIFORM2FVPROC,              "glUniform2fv");
    LOAD(glad_glUniform3fv,              PFNGLUNIFORM3FVPROC,              "glUniform3fv");
    LOAD(glad_glUniform4fv,              PFNGLUNIFORM4FVPROC,              "glUniform4fv");
    LOAD(glad_glUniformMatrix4fv,        PFNGLUNIFORMMATRIX4FVPROC,        "glUniformMatrix4fv");
    LOAD(glad_glVertexAttribPointer,     PFNGLVERTEXATTRIBPOINTERPROC,     "glVertexAttribPointer");
    LOAD(glad_glEnableVertexAttribArray,  PFNGLENABLEVERTEXATTRIBARRAYPROC,  "glEnableVertexAttribArray");
    LOAD(glad_glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC, "glDisableVertexAttribArray");
    LOAD(glad_glGenVertexArrays,         PFNGLGENVERTEXARRAYSPROC,         "glGenVertexArrays");
    LOAD(glad_glDeleteVertexArrays,      PFNGLDELETEVERTEXARRAYSPROC,      "glDeleteVertexArrays");
    LOAD(glad_glBindVertexArray,         PFNGLBINDVERTEXARRAYPROC,         "glBindVertexArray");
    LOAD(glad_glGenerateMipmap,          PFNGLGENERATEMIPMAPPROC,          "glGenerateMipmap");

    return glad_glCreateProgram != NULL;
}
