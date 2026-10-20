package com.movtery.zalithlauncher.renderer.renderers

import com.movtery.zalithlauncher.renderer.RendererInterface

/**
 * Experimental DroidBridge RenderSpec EGL provider route.
 *
 * This selects DBL's native desktop-GL bridge configuration, but does not
 * bundle Mesa binaries. A compatible libEGL_mesa.so must be available to the
 * app linker, or supplied through DROIDBRIDGE_RENDERSPEC_EGL in custom_env.txt.
 */
class DroidBridgeMesaRenderer : RendererInterface {
    override fun getRendererId(): String = "freedreno_kgsl"

    override fun getUniqueIdentifier(): String = "b3d64ebd-6903-4a2c-bdd3-9caa1d9fdbe7"

    override fun getRendererName(): String = "DroidBridge Mesa (external EGL)"

    override fun getRendererEnv(): Lazy<Map<String, String>> = lazy {
        linkedMapOf(
            "DROIDBRIDGE_OPENGL_PROXY" to "1",
            "DROIDBRIDGE_MESA" to "1",
            "DROIDBRIDGE_MESA_MODE" to "freedreno_kgsl",
            "DROIDBRIDGE_MESA_DRIVER" to "kgsl",
            "POJAV_RENDERER_MESA_MODE" to "freedreno_kgsl",
            "LIBGL_ES" to "2",
            "GALLIUM_DRIVER" to "",
            "EGL_PLATFORM" to "android",
            "DROIDBRIDGE_MESA_DESKTOP_GL" to "1",
            "DROIDBRIDGE_EGL_FORCE_DESKTOP_GL" to "1",
            "DROIDBRIDGE_EGL_NO_SYSTEM_FALLBACK" to "1",
            "DROIDBRIDGE_MESA_EGL" to "libEGL_mesa.so",
            "POJAV_EGL_LIBRARY" to "libEGL_mesa.so",
            "POJAVEXEC_EGL_LIBRARY" to "libEGL_mesa.so",
            "POJAV_RENDERER_LIBRARY" to "libEGL_mesa.so",
            "POJAVEXEC_RENDERER" to "libEGL_mesa.so",
            "MESA_LOADER_DRIVER_OVERRIDE" to "kgsl",
            "MESA_GL_VERSION_OVERRIDE" to "3.3COMPAT",
            "MESA_GLSL_VERSION_OVERRIDE" to "330",
            "MESA_GLSL_CACHE_DISABLE" to "false",
            "MESA_SHADER_CACHE_DISABLE" to "false",
            "allow_higher_compat_version" to "true",
            "force_glsl_extensions_warn" to "true",
            "allow_glsl_extension_directive_midshader" to "true",
            "LIBGL_MIPMAP" to "3",
            "LIBGL_NOINTOVLHACK" to "1",
            "LIBGL_NORMALIZE" to "1",
            "LIBGL_NOERROR" to "0"
        )
    }

    override fun getDlopenLibrary(): Lazy<List<String>> = lazy { emptyList() }

    // This is the native EGL provider, not an actual file named libGLDroidBridge.so.
    // LWJGL's requested proxy soname is supplied separately by JREUtils.
    override fun getRendererLibrary(): String = "libEGL_mesa.so"

    override fun getRendererEGL(): String = "libEGL_mesa.so"
}
