# Library Mastery Plan

Each module: small task -> bigger task -> bigger task -> Master Task, done standalone
in `gsoc/learning/`, outside the real project, so the goal is understanding the
library itself, not shipping a feature.

---

## A0 — C++ Concurrency [DONE]
- [x] Task 1 (`0-cpp`): mutex-protected shared counter, verified with ThreadSanitizer, 200000/200000 exact.
- [x] Task 2 (`01-cpp`): `Cell` class, mutex + condition_variable, single-slot handoff between two threads.
- [x] Task 3 (`02-cpp`): `Cell<T>` generalized into a bounded thread-safe queue with `push`/`pop`/`stop`, stress-tested to 300000+ items.
- [x] Master Task (`03-cpp`): `WarmupLatch` + `Cell<int>` mimicking `CaptureSession`'s camera-thread/main-thread handoff, 300 trials passing.

## A1 — DRM & GBM [DONE]
- [x] Task 1 (`10-drm`): open render node, `drmGetVersion`, `gbm_create_device`, clean teardown (ASan-verified, no leaks).
- [x] Task 2 (`10-drm`/`11-drm`): `gbm_device_is_format_supported` queries across XRGB8888/ARGB8888/NV12 x RENDERING/LINEAR.
- [x] Task 3 (`11-drm`): allocate a real buffer, write a per-row pattern, export as dma-buf fd, verify round-trip via independent `mmap`. Full line-by-line `EXPLAINED.md` written.
- [x] Master Task (`12-drm`): multi-plane introspection (`gbm_bo_get_plane_count`/`get_offset`/`get_stride_for_plane`/`get_fd_for_plane`), modifier query. Error-checking/cleanup explicitly skipped per your call.
- [x] Bonus (`13-drm`): save a real red image from a `gbm_bo` buffer to a `.ppm` file. Root-caused a real segfault (uninitialized `gbm_bo_map` out-param), fixed the stride/BGRX-to-RGB conversion, discussed why real pipelines avoid the manual copy (`tjCompress2` with a pitch parameter, zero-copy dma-buf paths).

## A2 — EGL [NOT STARTED]
- [ ] Task 1: headless pbuffer context. `eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, gbm_device)` (fetched via `eglGetProcAddress`, it's an extension), `eglInitialize`, `eglBindAPI(EGL_OPENGL_ES_API)`, `eglChooseConfig` with `EGL_PBUFFER_BIT`, `eglCreatePbufferSurface`, `eglCreateContext` (GLES3), `eglMakeCurrent`, print `GL_VERSION`/`GL_RENDERER`/`GL_VENDOR`.
- [ ] Task 2: import a dma-buf as an EGL image. Take a `gbm_bo` from A1, `eglCreateImageKHR` with `EGL_LINUX_DMA_BUF_EXT`, passing fd/stride/offset/modifier as attributes. Confirm the image is valid (non-`EGL_NO_IMAGE_KHR`).
- [ ] Task 3: bind that EGL image as a GL texture via `glEGLImageTargetTexture2DOES`, and read a pixel back with `glReadPixels` after rendering a trivial full-screen quad, to prove the dma-buf's contents are visible to the GPU.
- [ ] Master Task: full round trip — allocate a `gbm_bo`, CPU-write a pattern into it, import as EGL image, bind as texture, render it to a second pbuffer/framebuffer, `glReadPixels`, and verify the read-back pixels match what you wrote. This is the exact mechanism the real project uses to get camera frames onto the GPU without a copy.

## A3 — GLES / GLSL [NOT STARTED]
- [ ] Task 1: compile and link a minimal vertex+fragment shader pair, check `glGetShaderiv(COMPILE_STATUS)`/`glGetProgramiv(LINK_STATUS)` and print the info log on failure (deliberately break the shader once to see a real error).
- [ ] Task 2: draw a full-screen textured quad (VBO with position+texcoord, a sampler2D fragment shader), textured from a `glTexImage2D`-uploaded still image (any PNG/PPM), rendered into a pbuffer, read back and saved to disk.
- [ ] Task 3: same quad, but drive the fragment shader with a uniform (e.g. a time-based color tint or a brightness slider), proving you can control per-frame rendering parameters from the CPU side.
- [ ] Master Task: implement one real blending mode from the project's shaders from scratch — e.g. the dual-camera feather blend (`smoothstep`-based alpha) — as its own standalone GLSL fragment shader, fed by two textures, and verify the seam looks right against a synthetic two-color test input.

## A4 — DMA-BUF (cross-cutting) [NOT STARTED]
- [ ] Task 1: export a `gbm_bo` as a dma-buf fd (already done implicitly in A1), then use `ioctl(DMA_BUF_IOCTL_SYNC)` around a CPU read/write to understand explicit CPU/GPU synchronization (`DMA_BUF_SYNC_START`/`_END`).
- [ ] Task 2: pass a dma-buf fd between two separate processes via a Unix domain socket using `SCM_RIGHTS` fd-passing, and verify the receiving process sees the same buffer contents.
- [ ] Task 3: query and print a dma-buf's size via `lseek(fd, 0, SEEK_END)` or `fstat`, without needing the original `gbm_bo` handle, to see how "just an fd" carries no format/stride metadata on its own (that's why libcamera/GBM pass those out-of-band).
- [ ] Master Task: chain A1 + A2 + this module — allocate via GBM, write via CPU, pass the fd through a socket to a second process, have that process import it via EGL and render it, closing the full "capture in one place, display in another" loop that mirrors real multi-process compositor architectures.

## B1 — libcamera [IN PROGRESS, nearly done]
- [x] Task 1 (`20-libcam`): enumerate cameras, acquire, read+decode properties via `properties::properties` name lookup and `ControlValue::toString()`, clean teardown (no libcamera warnings).
- [x] Task 2 (`21-libcam`): `generateConfiguration`, inspect `StreamConfiguration` fields (`size`, `colorSpace`, `bufferCount`, `pixelFormat`, `frameSize`), `validate()`, `configure()`.
- [x] Task 3 (`22-libcam`): `FrameBufferAllocator`, single request/buffer round trip, async completion callback with `reuse()`+`queueRequest()` re-arm loop — real continuous capture, sequence numbers verified incrementing.
- [~] Master Task (`23-libcam`, `24-libcam`): multi-buffer pipeline (one request per allocated buffer, confirmed via unbroken sequence numbers), `WarmupLatch`-based synchronization, YUYV->RGB conversion derived from first principles (BT.601 matrix, `Kr`/`Kb` derivation, full-range vs limited-range), frame saved and visually verified against a real webcam capture, FPS calculation fixed (nanosecond unit bug, then wrong-numerator bug). Remaining: stop the capture loop after N frames instead of running forever, set a real control (e.g. `Brightness`), and clean teardown (`stop()`/`release()`/`my_cm.stop()` in order) — all scoped and understood, not yet typed in.

## B2 — GStreamer [NOT STARTED]
- [ ] Task 1: `gst-launch-1.0` command-line pipelines only, no code — `videotestsrc ! videoconvert ! autovideosink`, then a real `v4l2src device=/dev/videoN`, then insert a caps filter (`video/x-raw,format=YUYV`) and deliberately trigger a negotiation error to read GStreamer's own diagnostic.
- [ ] Task 2: same pipelines built programmatically via the C API (`gst_parse_launch` first, as the easy path), run the main loop, handle `GST_MESSAGE_EOS`/`GST_MESSAGE_ERROR` from the bus.
- [ ] Task 3: build the pipeline element-by-element instead of via a parse string (`gst_element_factory_make`, manual `gst_element_link`), and pull frames out via an `appsink` callback instead of a video sink, printing buffer size/timestamp per frame.
- [ ] Master Task: hardware-accelerated decode/encode path relevant to the project's known RPi5 constraint (HEVC only, no H.264 hw decode) — build a pipeline that decodes a real HEVC file via `v4l2h265dec` (or equivalent) into raw frames, confirm it actually engages hardware (not software fallback) via `GST_DEBUG` output.

## C1 — Wayland [NOT STARTED]
- [ ] Task 1: connect to a Wayland display (`wl_display_connect`), bind the core globals via a registry listener (`wl_compositor`, `wl_shell`/`xdg_wm_base`), print what globals the compositor advertises.
- [ ] Task 2: create a `wl_surface` and get it on screen as a blank/solid-color window using `wl_shm` (CPU-side shared memory buffer, no GPU yet).
- [ ] Task 3: swap the `wl_shm` buffer for an EGL-backed one (`wl_egl_window` + `eglCreateWindowSurface`), so you're rendering with GLES into a real on-screen window instead of a pbuffer.
- [ ] Master Task: reproduce a minimal version of the project's own `WaylandPreviewWindow` — render your A3 textured-quad shader live into a real Wayland window, updating every frame, and handle window resize/close events correctly.

## D1 — Projective Geometry [NOT STARTED]
- [ ] Task 1: by hand (no OpenCV), implement a 3x3 homography apply function — given a matrix and a point, compute the projected point including the perspective divide, and verify against a known simple case (e.g. a pure scale+translate matrix).
- [ ] Task 2: given 4 point correspondences, solve for the homography matrix yourself (set up the linear system, solve via Gaussian elimination or a small linear-algebra call), and verify it reproduces the 4 input points exactly.
- [ ] Task 3: warp a whole test image through your homography (inverse-map each output pixel back into the source image, bilinear sample), and visually confirm against a known reference (e.g. OpenCV's `warpPerspective` on the same matrix).
- [ ] Master Task: reproduce the IPM (inverse perspective mapping) math behind the project's bird's-eye-view stitch from scratch — given synthetic checkerboard-like test points and known camera extrinsics, derive the ground-plane homography and warp a synthetic camera image into a top-down view, cross-checked against the real project's calibration output.

## D2 — OpenCV [NOT STARTED]
- [ ] Task 1: load an image, convert color spaces (`cvtColor`), and reproduce your own YUYV->RGB conversion from Module B1 using `cv::COLOR_YUV2RGB_YUYV`, diffing against your hand-written version to confirm they agree.
- [ ] Task 2: `cv::findChessboardCorners`/`cv::calibrateCamera` on a set of checkerboard images, reproducing (at small scale) the same intrinsics-calibration workflow behind the project's real ChArUco calibration.
- [ ] Task 3: `cv::warpPerspective` using a homography from Module D1, compare pixel-for-pixel against your own hand-written warp.
- [ ] Master Task: full calibrate -> undistort -> warp pipeline on a real captured sequence from your webcam (Module B1), producing a bird's-eye-view-style output, the same conceptual pipeline as the real project's dual-camera BEV stitch, just single-camera and CPU-side.

## D3 — Image Pyramids [NOT STARTED]
- [ ] Task 1: build a Gaussian pyramid by hand (repeated blur+downsample), display each level's size, verify against `cv::pyrDown`.
- [ ] Task 2: build the corresponding Laplacian pyramid (each level = that Gaussian level minus the upsampled next level), and verify perfect reconstruction (sum the Laplacian pyramid back up and confirm you get the original image, matching the telescoping-sum proof from `18_blending_mathematics.md`).
- [ ] Task 3: blend two images at a single pyramid level using a simple mask, to see why blending at one resolution alone produces visible seams.
- [ ] Master Task: full multiband blend — build Laplacian pyramids for two source images, blend each level with a smoothly-varying mask, collapse back to one image, and visually confirm the seam is far less visible than a single-level blend, reproducing the real project's multiband blend mode from first principles.

## Capstone — Cross-Library Integration [NOT STARTED]
Three tiers, each strictly harder, combining everything above into something closer to the real project:
- [ ] Tier 1: single real camera (libcamera) -> dma-buf -> EGL import -> GLES render -> Wayland window, live, no blending yet. This is "the whole real project's plumbing, one camera, no math."
- [ ] Tier 2: two cameras (or one real + one synthetic/file-replayed via GStreamer) -> per-camera EGL textures -> a homography-warped GLSL shader per camera (Module D1 math running on GPU, not CPU) -> composited into one Wayland window.
- [ ] Tier 3: add a real blend mode (feather or multiband, Module A3/D3) at the seam between the two warped views, live, at interactive frame rate — at that point this is a from-scratch, self-built miniature of the real project's core pipeline, and a natural point to go read the real project's source and compare design decisions.
