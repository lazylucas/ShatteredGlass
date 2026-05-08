#pragma once
// Minimal GL extension prototypes for Windows
#include <GL/gl.h>
#include <stddef.h>

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

typedef char      GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_VERTEX_SHADER     0x8B31
#define GL_FRAGMENT_SHADER   0x8B30
#define GL_COMPILE_STATUS    0x8B81
#define GL_LINK_STATUS       0x8B82
#define GL_ARRAY_BUFFER      0x8892
#define GL_STATIC_DRAW       0x88B4
#define GL_TEXTURE0          0x84C0
#define GL_TEXTURE_2D        0x0DE1
#define GL_TRIANGLE_STRIP    0x0005
#define GL_FLOAT             0x1406
#define GL_FALSE             0
#define GL_LINEAR            0x2601
#define GL_CLAMP_TO_EDGE     0x812F

typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC)            (GLenum);
typedef void   (APIENTRYP PFNGLSHADERSOURCEPROC)            (GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void   (APIENTRYP PFNGLCOMPILESHADERPROC)           (GLuint);
typedef void   (APIENTRYP PFNGLGETSHADERIVPROC)             (GLuint, GLenum, GLint*);
typedef void   (APIENTRYP PFNGLGETSHADERINFOLOGPROC)        (GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC)           (void);
typedef void   (APIENTRYP PFNGLATTACHSHADERPROC)            (GLuint, GLuint);
typedef void   (APIENTRYP PFNGLLINKPROGRAMPROC)             (GLuint);
typedef void   (APIENTRYP PFNGLGETPROGRAMIVPROC)            (GLuint, GLenum, GLint*);
typedef void   (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC)       (GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (APIENTRYP PFNGLUSEPROGRAMPROC)              (GLuint);
typedef void   (APIENTRYP PFNGLDELETESHADERPROC)            (GLuint);
typedef void   (APIENTRYP PFNGLDELETEPROGRAMPROC)           (GLuint);
typedef void   (APIENTRYP PFNGLGENVERTEXARRAYSPROC)         (GLsizei, GLuint*);
typedef void   (APIENTRYP PFNGLBINDVERTEXARRAYPROC)         (GLuint);
typedef void   (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)      (GLsizei, const GLuint*);
typedef void   (APIENTRYP PFNGLGENBUFFERSPROC)              (GLsizei, GLuint*);
typedef void   (APIENTRYP PFNGLBINDBUFFERPROC)              (GLenum, GLuint);
typedef void   (APIENTRYP PFNGLBUFFERDATAPROC)              (GLenum, GLsizeiptr, const void*, GLenum);
typedef void   (APIENTRYP PFNGLDELETEBUFFERSPROC)           (GLsizei, const GLuint*);
typedef void   (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC)     (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void   (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint);
typedef GLint  (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC)      (GLuint, const GLchar*);
typedef void   (APIENTRYP PFNGLUNIFORM1IPROC)               (GLint, GLint);
typedef void   (APIENTRYP PFNGLUNIFORM1FPROC)               (GLint, GLfloat);
typedef void   (APIENTRYP PFNGLUNIFORM2FPROC)               (GLint, GLfloat, GLfloat);
typedef void   (APIENTRYP PFNGLUNIFORM3FPROC)               (GLint, GLfloat, GLfloat, GLfloat);
typedef void   (APIENTRYP PFNGLACTIVETEXTUREPROC)           (GLenum);
