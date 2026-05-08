// ════════════════════════════════════════════════════════════════════════════
//  Cinematic Shattered Glass — FFGL2 Plugin for Resolume
//  Single-file implementation that the CMakeLists references.
// ════════════════════════════════════════════════════════════════════════════

#include "../FFGLSDK/FFGL.h"
#include "Shaders.h"

#include <windows.h>
#include <GL/gl.h>
#include "glext.h"

#include <chrono>
#include <cstring>
#include <vector>
#include <string>

// ════════════════════════════════════════════════════════════════════════════
//  Plugin metadata — change this 4-byte ID if you build a variant
// ════════════════════════════════════════════════════════════════════════════
static const char* kPluginName       = "Shattered Glass";
static const char  kPluginUniqueID[4] = {'C','S','G','1'};   // CSG version 1

// ════════════════════════════════════════════════════════════════════════════
//  Parameter indices (15 total — matches README)
// ════════════════════════════════════════════════════════════════════════════
enum ParamIdx {
    P_IMPACT_X = 0,
    P_IMPACT_Y,
    P_IMPACT_FORCE,
    P_CRACK_SPREAD,
    P_RADIAL_CRACKS,
    P_STRESS_RINGS,
    P_REFRACTION_DEPTH,
    P_SHARD_SEPARATION,
    P_EDGE_LIGHT,
    P_CHROMATIC,
    P_DIRT,
    P_BLOOM,
    P_ANIM_SPEED,
    P_SEED,
    P_FREEZE,
    PARAM_COUNT
};

// ════════════════════════════════════════════════════════════════════════════
//  GL function pointers
// ════════════════════════════════════════════════════════════════════════════

#define GL_FUNC(T, n) static T n = nullptr
GL_FUNC(PFNGLCREATESHADERPROC,            glCreateShader);
GL_FUNC(PFNGLSHADERSOURCEPROC,            glShaderSource);
GL_FUNC(PFNGLCOMPILESHADERPROC,           glCompileShader);
GL_FUNC(PFNGLGETSHADERIVPROC,             glGetShaderiv);
GL_FUNC(PFNGLGETSHADERINFOLOGPROC,        glGetShaderInfoLog);
GL_FUNC(PFNGLCREATEPROGRAMPROC,           glCreateProgram);
GL_FUNC(PFNGLATTACHSHADERPROC,            glAttachShader);
GL_FUNC(PFNGLLINKPROGRAMPROC,             glLinkProgram);
GL_FUNC(PFNGLGETPROGRAMIVPROC,            glGetProgramiv);
GL_FUNC(PFNGLGETPROGRAMINFOLOGPROC,       glGetProgramInfoLog);
GL_FUNC(PFNGLUSEPROGRAMPROC,              glUseProgram);
GL_FUNC(PFNGLDELETESHADERPROC,            glDeleteShader);
GL_FUNC(PFNGLDELETEPROGRAMPROC,           glDeleteProgram);
GL_FUNC(PFNGLGENVERTEXARRAYSPROC,         glGenVertexArrays);
GL_FUNC(PFNGLBINDVERTEXARRAYPROC,         glBindVertexArray);
GL_FUNC(PFNGLDELETEVERTEXARRAYSPROC,      glDeleteVertexArrays);
GL_FUNC(PFNGLGENBUFFERSPROC,              glGenBuffers);
GL_FUNC(PFNGLBINDBUFFERPROC,              glBindBuffer);
GL_FUNC(PFNGLBUFFERDATAPROC,              glBufferData);
GL_FUNC(PFNGLDELETEBUFFERSPROC,           glDeleteBuffers);
GL_FUNC(PFNGLVERTEXATTRIBPOINTERPROC,     glVertexAttribPointer);
GL_FUNC(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
GL_FUNC(PFNGLGETUNIFORMLOCATIONPROC,      glGetUniformLocation);
GL_FUNC(PFNGLUNIFORM1IPROC,               glUniform1i);
GL_FUNC(PFNGLUNIFORM1FPROC,               glUniform1f);
GL_FUNC(PFNGLUNIFORM2FPROC,               glUniform2f);
GL_FUNC(PFNGLUNIFORM3FPROC,               glUniform3f);
GL_FUNC(PFNGLACTIVETEXTUREPROC,           glActiveTexture);
#undef GL_FUNC

#define RESOLVE(T, n) n = (T)wglGetProcAddress(#n)
static bool s_glResolved = false;
static void ResolveGL() {
    if (s_glResolved) return;
    RESOLVE(PFNGLCREATESHADERPROC,            glCreateShader);
    RESOLVE(PFNGLSHADERSOURCEPROC,            glShaderSource);
    RESOLVE(PFNGLCOMPILESHADERPROC,           glCompileShader);
    RESOLVE(PFNGLGETSHADERIVPROC,             glGetShaderiv);
    RESOLVE(PFNGLGETSHADERINFOLOGPROC,        glGetShaderInfoLog);
    RESOLVE(PFNGLCREATEPROGRAMPROC,           glCreateProgram);
    RESOLVE(PFNGLATTACHSHADERPROC,            glAttachShader);
    RESOLVE(PFNGLLINKPROGRAMPROC,             glLinkProgram);
    RESOLVE(PFNGLGETPROGRAMIVPROC,            glGetProgramiv);
    RESOLVE(PFNGLGETPROGRAMINFOLOGPROC,       glGetProgramInfoLog);
    RESOLVE(PFNGLUSEPROGRAMPROC,              glUseProgram);
    RESOLVE(PFNGLDELETESHADERPROC,            glDeleteShader);
    RESOLVE(PFNGLDELETEPROGRAMPROC,           glDeleteProgram);
    RESOLVE(PFNGLGENVERTEXARRAYSPROC,         glGenVertexArrays);
    RESOLVE(PFNGLBINDVERTEXARRAYPROC,         glBindVertexArray);
    RESOLVE(PFNGLDELETEVERTEXARRAYSPROC,      glDeleteVertexArrays);
    RESOLVE(PFNGLGENBUFFERSPROC,              glGenBuffers);
    RESOLVE(PFNGLBINDBUFFERPROC,              glBindBuffer);
    RESOLVE(PFNGLBUFFERDATAPROC,              glBufferData);
    RESOLVE(PFNGLDELETEBUFFERSPROC,           glDeleteBuffers);
    RESOLVE(PFNGLVERTEXATTRIBPOINTERPROC,     glVertexAttribPointer);
    RESOLVE(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    RESOLVE(PFNGLGETUNIFORMLOCATIONPROC,      glGetUniformLocation);
    RESOLVE(PFNGLUNIFORM1IPROC,               glUniform1i);
    RESOLVE(PFNGLUNIFORM1FPROC,               glUniform1f);
    RESOLVE(PFNGLUNIFORM2FPROC,               glUniform2f);
    RESOLVE(PFNGLUNIFORM3FPROC,               glUniform3f);
    RESOLVE(PFNGLACTIVETEXTUREPROC,           glActiveTexture);
    s_glResolved = true;
}

// ════════════════════════════════════════════════════════════════════════════
//  Plugin instance
// ════════════════════════════════════════════════════════════════════════════
struct ParamInfo {
    const char* name;
    uint32_t    type;
    float       defaultVal;
};

static const ParamInfo kParams[PARAM_COUNT] = {
    { "Impact X",          FF_TYPE_XPOS,     0.50f },
    { "Impact Y",          FF_TYPE_YPOS,     0.50f },
    { "Impact Force",      FF_TYPE_STANDARD, 0.65f },
    { "Crack Spread",      FF_TYPE_STANDARD, 0.50f },
    { "Radial Cracks",     FF_TYPE_STANDARD, 0.70f },
    { "Stress Rings",      FF_TYPE_STANDARD, 0.40f },
    { "Refraction Depth",  FF_TYPE_STANDARD, 0.50f },
    { "Shard Separation",  FF_TYPE_STANDARD, 0.30f },
    { "Edge Light",        FF_TYPE_STANDARD, 0.55f },
    { "Chromatic",         FF_TYPE_STANDARD, 0.30f },
    { "Dirt",              FF_TYPE_STANDARD, 0.20f },
    { "Bloom",             FF_TYPE_STANDARD, 0.40f },
    { "Animation Speed",   FF_TYPE_STANDARD, 0.30f },
    { "Seed",              FF_TYPE_STANDARD, 0.50f },
    { "Freeze",            FF_TYPE_BOOLEAN,  0.00f },
};

class CinematicShatteredGlass {
public:
    CinematicShatteredGlass() {
        for (int i = 0; i < PARAM_COUNT; i++) m_values[i] = kParams[i].defaultVal;
    }

    FFResult InitGL(const FFGLViewportStruct* vp) {
        ResolveGL();
        if (!CompileProgram()) return FF_FAIL;
        CacheUniforms();
        BuildQuad();
        m_startTime = NowSeconds();
        return FF_SUCCESS;
    }

    FFResult DeInitGL() {
        if (m_program) { glDeleteProgram(m_program); m_program = 0; }
        if (m_vao)     { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
        if (m_vbo)     { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
        return FF_SUCCESS;
    }

    FFResult ProcessOpenGL(ProcessOpenGLStruct* pGL) {
        if (!pGL || pGL->numInputTextures < 1 || !pGL->inputTextures[0])
            return FF_FAIL;

        FFGLTextureStruct* tex = pGL->inputTextures[0];
        float t = NowSeconds() - m_startTime;

        glUseProgram(m_program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->Handle);
        glUniform1i(m_uTexture, 0);
        glUniform2f(m_uTexSize, (float)tex->HardwareWidth, (float)tex->HardwareHeight);
        glUniform1f(m_uTime, t);

        // ── Map UI [0..1] → shader ranges ─────────────────────────────────
        glUniform2f(m_uImpact,             m_values[P_IMPACT_X], m_values[P_IMPACT_Y]);
        glUniform1f(m_uImpactForce,        m_values[P_IMPACT_FORCE]);
        // Crack Spread maps inversely: low = many tiny shards, high = few large
        // Shader treats it as a spread factor; shader does mix(40, 6, val) internally
        glUniform1f(m_uCrackSpread,        m_values[P_CRACK_SPREAD]);
        glUniform1f(m_uRadialCracks,       m_values[P_RADIAL_CRACKS]);
        glUniform1f(m_uStressRings,        m_values[P_STRESS_RINGS]);
        glUniform1f(m_uRefractionDepth,    m_values[P_REFRACTION_DEPTH]);
        glUniform1f(m_uShardSeparation,    m_values[P_SHARD_SEPARATION]);
        glUniform1f(m_uEdgeLight,          m_values[P_EDGE_LIGHT]);
        glUniform1f(m_uChromatic,          m_values[P_CHROMATIC]);
        glUniform1f(m_uDirt,               m_values[P_DIRT]);
        glUniform1f(m_uBloom,              m_values[P_BLOOM]);
        glUniform1f(m_uAnimSpeed, 2.0f *   m_values[P_ANIM_SPEED]);
        glUniform1f(m_uSeed,               m_values[P_SEED]);
        glUniform1f(m_uFreeze, m_values[P_FREEZE] > 0.5f ? 1.0f : 0.0f);

        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glUseProgram(0);
        return FF_SUCCESS;
    }

    FFResult SetFloatParameter(unsigned int idx, float v) {
        if (idx >= PARAM_COUNT) return FF_FAIL;
        m_values[idx] = v;
        return FF_SUCCESS;
    }
    float GetFloatParameter(unsigned int idx) const {
        return idx < PARAM_COUNT ? m_values[idx] : 0.0f;
    }
    static const char* GetParamName(unsigned int idx)    { return idx < PARAM_COUNT ? kParams[idx].name : ""; }
    static uint32_t    GetParamType(unsigned int idx)    { return idx < PARAM_COUNT ? kParams[idx].type : FF_TYPE_STANDARD; }
    static float       GetParamDefault(unsigned int idx) { return idx < PARAM_COUNT ? kParams[idx].defaultVal : 0.0f; }

private:
    GLuint m_program = 0, m_vao = 0, m_vbo = 0;
    GLint  m_uTexture, m_uTexSize, m_uTime;
    GLint  m_uImpact, m_uImpactForce, m_uCrackSpread;
    GLint  m_uRadialCracks, m_uStressRings, m_uRefractionDepth;
    GLint  m_uShardSeparation, m_uEdgeLight, m_uChromatic;
    GLint  m_uDirt, m_uBloom, m_uAnimSpeed, m_uSeed, m_uFreeze;

    float  m_values[PARAM_COUNT];
    float  m_startTime = 0.0f;

    bool CompileProgram() {
        auto compile = [](GLenum type, const char* src) -> GLuint {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            GLint ok = 0;
            glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[2048] = {0};
                GLsizei len;
                glGetShaderInfoLog(s, sizeof(log)-1, &len, log);
                OutputDebugStringA("[ShatteredGlass] shader error:\n");
                OutputDebugStringA(log);
                glDeleteShader(s);
                return 0;
            }
            return s;
        };
        GLuint v = compile(GL_VERTEX_SHADER,   kVertSrc);
        GLuint f = compile(GL_FRAGMENT_SHADER, kFragSrc);
        if (!v || !f) return false;

        m_program = glCreateProgram();
        glAttachShader(m_program, v);
        glAttachShader(m_program, f);
        glLinkProgram(m_program);
        glDeleteShader(v);
        glDeleteShader(f);

        GLint ok = 0;
        glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048] = {0};
            GLsizei len;
            glGetProgramInfoLog(m_program, sizeof(log)-1, &len, log);
            OutputDebugStringA("[ShatteredGlass] link error:\n");
            OutputDebugStringA(log);
            glDeleteProgram(m_program);
            m_program = 0;
            return false;
        }
        return true;
    }

    void CacheUniforms() {
        m_uTexture           = glGetUniformLocation(m_program, "inputTexture");
        m_uTexSize           = glGetUniformLocation(m_program, "texSize");
        m_uTime              = glGetUniformLocation(m_program, "time");
        m_uImpact            = glGetUniformLocation(m_program, "impact");
        m_uImpactForce       = glGetUniformLocation(m_program, "impactForce");
        m_uCrackSpread       = glGetUniformLocation(m_program, "crackSpread");
        m_uRadialCracks      = glGetUniformLocation(m_program, "radialCracks");
        m_uStressRings       = glGetUniformLocation(m_program, "stressRings");
        m_uRefractionDepth   = glGetUniformLocation(m_program, "refractionDepth");
        m_uShardSeparation   = glGetUniformLocation(m_program, "shardSeparation");
        m_uEdgeLight         = glGetUniformLocation(m_program, "edgeLight");
        m_uChromatic         = glGetUniformLocation(m_program, "chromatic");
        m_uDirt              = glGetUniformLocation(m_program, "dirt");
        m_uBloom             = glGetUniformLocation(m_program, "bloom");
        m_uAnimSpeed         = glGetUniformLocation(m_program, "animSpeed");
        m_uSeed              = glGetUniformLocation(m_program, "seed");
        m_uFreeze            = glGetUniformLocation(m_program, "freeze");
    }

    void BuildQuad() {
        static const float quad[] = {
            -1.f,-1.f, 0.f,0.f,
             1.f,-1.f, 1.f,0.f,
            -1.f, 1.f, 0.f,1.f,
             1.f, 1.f, 1.f,1.f,
        };
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    static float NowSeconds() {
        using clk = std::chrono::steady_clock;
        return std::chrono::duration<float>(clk::now().time_since_epoch()).count();
    }
};

// ════════════════════════════════════════════════════════════════════════════
//  FFGL DLL exports
// ════════════════════════════════════════════════════════════════════════════
static CinematicShatteredGlass* s_plugin = nullptr;

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }

extern "C" {

__declspec(dllexport) FFResult __stdcall FF_GetInfo(void* out) {
    struct PluginInfo {
        uint32_t APIMajorVersion;
        uint32_t APIMinorVersion;
        char     UniqueID[4];
        char     Name[16];
        uint32_t PluginType;
    };
    auto* info = static_cast<PluginInfo*>(out);
    info->APIMajorVersion = 2;
    info->APIMinorVersion = 1;
    memcpy(info->UniqueID, kPluginUniqueID, 4);
    strncpy_s(info->Name, 16, kPluginName, 16);
    info->PluginType = FF_EFFECT;
    return FF_SUCCESS;
}

__declspec(dllexport) FFResult __stdcall FF_Initialise()  { return FF_SUCCESS; }
__declspec(dllexport) FFResult __stdcall FF_Deinitialise(){ return FF_SUCCESS; }

__declspec(dllexport) FFResult __stdcall FF_GetNumParameters(void*) {
    return (FFResult)PARAM_COUNT;
}

__declspec(dllexport) const char* __stdcall FF_GetParameterName(void* data) {
    uint32_t idx = *(uint32_t*)data;
    return CinematicShatteredGlass::GetParamName(idx);
}

__declspec(dllexport) FFResult __stdcall FF_GetParameterType(void* data) {
    uint32_t idx = *(uint32_t*)data;
    return (FFResult)CinematicShatteredGlass::GetParamType(idx);
}

__declspec(dllexport) float __stdcall FF_GetParameterDefault(void* data) {
    uint32_t idx = *(uint32_t*)data;
    return CinematicShatteredGlass::GetParamDefault(idx);
}

__declspec(dllexport) FFResult __stdcall FF_InstantiateGL(void* vp) {
    if (s_plugin) return FF_FAIL;
    s_plugin = new CinematicShatteredGlass();
    return s_plugin->InitGL((FFGLViewportStruct*)vp);
}

__declspec(dllexport) FFResult __stdcall FF_DeInstantiateGL(void*) {
    if (!s_plugin) return FF_FAIL;
    s_plugin->DeInitGL();
    delete s_plugin;
    s_plugin = nullptr;
    return FF_SUCCESS;
}

__declspec(dllexport) FFResult __stdcall FF_SetParameter(void* data) {
    struct SetParamStruct { uint32_t index; float value; };
    auto* p = (SetParamStruct*)data;
    if (!s_plugin) return FF_FAIL;
    return s_plugin->SetFloatParameter(p->index, p->value);
}

__declspec(dllexport) float __stdcall FF_GetParameter(void* data) {
    uint32_t idx = *(uint32_t*)data;
    if (!s_plugin) return 0.0f;
    return s_plugin->GetFloatParameter(idx);
}

__declspec(dllexport) FFResult __stdcall FF_ProcessOpenGL(void* data) {
    if (!s_plugin) return FF_FAIL;
    return s_plugin->ProcessOpenGL((ProcessOpenGLStruct*)data);
}

__declspec(dllexport) FFResult __stdcall FF_GetPluginCaps(void* data) {
    uint32_t cap = *(uint32_t*)data;
    if (cap == 0) return 1;  // min input textures
    if (cap == 1) return 1;  // max input textures
    return FF_FALSE;
}

}  // extern "C"
