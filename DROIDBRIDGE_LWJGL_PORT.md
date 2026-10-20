# DroidBridge LWJGL / RenderSpec integration

The fork now includes DBL-derived native input and graphics bridge sources plus a built-in **DroidBridge Mesa (external EGL)** renderer entry. The renderer is separate from the existing GL4ES, Zink, VirGL, Freedreno/OSMesa, and Panfrost choices.

## Selecting the DBL path

In the launcher renderer selection screen choose **DroidBridge Mesa (external EGL)** only when you have a compatible Mesa EGL provider. It configures the DBL freedreno_kgsl native bridge mode and requests the LWJGL proxy soname libGLDroidBridge.so.

**Important:** this branch does not contain or bundle libEGL_mesa.so, the Mesa/Gallium driver files, or the libGLDroidBridge.so proxy as a standalone shared library. The proxy name is a sentinel intercepted by the native LWJGL loader hook; the hook must be able to open the real EGL provider. Without a compatible provider, selecting this renderer is expected to fail to create a usable graphics context. Keep using your current renderer when you do not have that provider installed.

## Supplying a custom provider path

By default, DBL looks for libEGL_mesa.so by soname. If your compatible provider is installed elsewhere, add a line in the launcher's game-home custom_env.txt file. Use the actual library path available on your device:

```text
DROIDBRIDGE_RENDERSPEC_EGL=/absolute/path/to/libEGL_mesa.so
```

The launch setup passes this provider to the RenderSpec hook and the native EGL bridge. The hook checks DROIDBRIDGE_RENDERSPEC_EGL, DROIDBRIDGE_MESA_EGL, POJAVEXEC_EGL, POJAV_EGL_LIBRARY, and POJAVEXEC_EGL_LIBRARY, then attempts libEGL_mesa.so and libEGL.so as fallbacks. A successful dlopen alone does not guarantee that the provider can resolve the required GL functions or create a context.

The legacy custom-env flag DROIDBRIDGE_OPENGL_PROXY=1 alone is not enough to enable the path on a normal renderer. The code guards the override unless the DBL renderer is selected or an explicit DROIDBRIDGE_RENDERSPEC_EGL provider path is also configured.

## What was ported

- The DBL native input bridge and Java-side callback API, including compatibility overloads for Zalith's existing touch controls.
- DBL EGL initialization, GL/EGL/OSMesa loader tables, bridge configuration, environment structure, linker-namespace helpers, and native utilities/logging.
- An opt-in LWJGL OpenGL-provider override that retains the existing renderer library by default.
- EGL/GLES link flags for the pojavexec and driver_helper native modules.

## Validation status

Source integration is committed, but the Android NDK/Gradle build and real-device Minecraft launch have not been verified. GitHub Actions currently reports no workflow runs for this branch. Treat the DBL renderer as experimental until an APK build and an in-game rendering/input test succeed.
