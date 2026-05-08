#pragma once
// Minimal FFGL 2.1 header subset
// Full SDK: https://github.com/resolume/ffgl
#include <stdint.h>

#define FFGL_VERSION_MAJOR 2
#define FFGL_VERSION_MINOR 1

#define FF_SUCCESS      0
#define FF_FAIL         0xFFFFFFFF
#define FF_TRUE         1
#define FF_FALSE        0
#define FF_SUPPORTED    1
#define FF_UNSUPPORTED  0

#define FF_EFFECT       1
#define FF_SOURCE       2
#define FF_MIXER        3

#define FF_TYPE_BOOLEAN     0
#define FF_TYPE_EVENT       1
#define FF_TYPE_RED         2
#define FF_TYPE_GREEN       3
#define FF_TYPE_BLUE        4
#define FF_TYPE_XPOS        5
#define FF_TYPE_YPOS        6
#define FF_TYPE_STANDARD    10
#define FF_TYPE_OPTION      11
#define FF_TYPE_BUFFER      12
#define FF_TYPE_INTEGER     13
#define FF_TYPE_RGBA        14
#define FF_TYPE_TEXT        100

typedef uint32_t FFResult;
typedef void*    FFInstanceID;

struct FFGLViewportStruct {
    uint32_t x, y, width, height;
};

struct FFGLTextureStruct {
    uint32_t Width, Height;
    uint32_t HardwareWidth, HardwareHeight;
    uint32_t Handle;
};

struct ProcessOpenGLStruct {
    uint32_t            numInputTextures;
    FFGLTextureStruct** inputTextures;
    uint32_t            HostFBO;
};
