# DroidBridge LWJGL / RenderSpec integration

This fork now includes the DroidBridge-derived native input bridge, EGL initialization path, renderer bridge ABI header, and LWJGL `dlopen` hook. The DroidBridge proxy is intentionally opt-in so the existing Zalith renderer path remains the default.

## Enable the RenderSpec proxy

Use the launcher's game-home `custom_env.txt` file and add:

```text
DROIDBRIDGE_OPENGL_PROXY=1
```

Only enable this when your selected renderer/provider installation actually supplies a compatible EGL provider. The hook checks these provider values in order:

1. `DROIDBRIDGE_RENDERSPEC_EGL`
2. `DROIDBRIDGE_MESA_EGL`
3. `POJAVEXEC_EGL`
4. `POJAV_EGL_LIBRARY`
5. `POJAVEXEC_EGL_LIBRARY`

If none is supplied or opens successfully, the hook attempts `libEGL_mesa.so` and then `libEGL.so`. A successful `dlopen` does not by itself guarantee that the provider can resolve the GL entry points or create a working Minecraft context.

## What stays unchanged by default

When `DROIDBRIDGE_OPENGL_PROXY` is unset, Zalith continues to pass its selected renderer's normal library to LWJGL. The port does not bundle or manufacture Mesa/Freedreno binaries; those must come from a compatible renderer installation. Vulkan interception retains a normal `dlopen` fallback when its optional custom loader returns null.

## Verification status

The source integration has not yet been confirmed by a successful Android NDK/Gradle build or by a real-device Minecraft launch. Treat the RenderSpec option as experimental until the CI build and runtime checks pass.
