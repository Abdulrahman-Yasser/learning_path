# `main.cpp` — line by line

Covers Module A1 (DRM & GBM) Tasks 1-3: open the render node, query the
driver, query format support, allocate a real buffer, write a pattern
into it, and verify the round trip through its exported dma-buf fd.

## Includes

```cpp
#include <fstream>
#include <stdio.h>
#include <xf86drm.h>
#include <fcntl.h>    // For open()
#include <gbm.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
using namespace std;
```
`<fstream>` and `<sys/stat.h>` are unused leftovers from an earlier
detour (a file/`fstat`-based approach that got removed) — harmless, but
worth deleting since they no longer serve anything. The rest: `<stdio.h>`
for `printf`, `<xf86drm.h>` for the `drm*` functions, `<fcntl.h>` for
`open`/`O_RDWR`, `<gbm.h>` for everything GBM, `<unistd.h>` for `close`,
`<sys/mman.h>` for `mmap`/`munmap`.

## Task 1 — open the render node, query the driver

```cpp
int height = 120, width = 120;
```
Named constants for the buffer's dimensions, used everywhere below
instead of repeating `120`. Since they never change, `const int` would
be slightly more honest, but not a bug either way.

```cpp
int fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
if(fd == -1){
    printf("Error openning the file \n");
}
```
Opens the GPU's render node, getting a raw file descriptor. `O_RDWR`
since later code both reads and writes through GPU-related resources;
`O_CLOEXEC` so this fd automatically closes if this process ever
`exec()`s a child, rather than leaking into it. The error branch prints
but doesn't `return` — a minor gap (if this ever actually failed, every
subsequent call would operate on `fd = -1` and fail confusingly instead
of stopping cleanly here) but not one that bites in practice, since this
open essentially always succeeds on this machine.

```cpp
drmVersionPtr mine;
mine = drmGetVersion(fd);
printf("maj: %d, min: %d, name: %s, desc: %s\n", mine->version_major, mine->version_minor, mine->name, mine->desc);
drmFreeVersion(mine);
```
Asks the kernel DRM driver bound to this fd who it is — `drmGetVersion`
allocates and returns a struct with the driver's name (`amdgpu`),
description, and version numbers, which gets printed, then
`drmFreeVersion` releases that struct. Print happens *before* the free —
this ordering is what fixes the use-after-free bug hit earlier while
building this.

```cpp
gbm_device* my_device;
my_device = gbm_create_device(fd);
```
Wraps the same fd in a `gbm_device*` — the handle every subsequent GBM
call needs. GBM doesn't open anything of its own; it's purely a layer on
top of the fd already opened above.

## Task 2 — which formats/usages does this GPU actually support

```cpp
int supported;
supported = gbm_device_is_format_supported(my_device, GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);
printf("\nGBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING : %d\n", supported);
```
...and five more blocks identical in shape, varying the format
(`XRGB8888`/`ARGB8888`/`NV12`) and usage (`RENDERING`/`LINEAR`). Each
call is a pure query — "can this device allocate a buffer of this format
for this purpose" — no actual allocation happens in any of these six
calls. This is what proved `NV12` isn't allocatable through GBM on this
GPU at all, for either purpose — a real, empirically confirmed hardware
fact, not a hypothetical.

## Task 3 — allocate, write, export, verify

```cpp
gbm_bo* my_bo = gbm_bo_create(my_device, width, height, GBM_FORMAT_XRGB8888, GBM_BO_USE_LINEAR);
```
An actual allocation this time: a real 120×120 opaque-RGB buffer,
requested with a plain (non-GPU-tiled) memory layout. Returns a
`gbm_bo*` — the handle for this one specific buffer.

```cpp
uint32_t my_stride=0;
void *map_data = NULL;
```
Two variables `gbm_bo_map()` is about to interact with. `my_stride` will
receive the buffer's *real* row pitch (which can be larger than
`width * bytes_per_pixel` due to alignment padding — measured as 512
here, not the naive 480, on this GPU). `map_data` is initialized to
`NULL` — it's about to receive something, but **not** pixel data (see
the next block).

```cpp
void* map_handle = gbm_bo_map(my_bo, 0, 0, width, height, GBM_BO_TRANSFER_WRITE, &my_stride, &map_data);
```
The single most important line in this file, and the one that took the
most rounds to get right. `gbm_bo_map()`'s **return value**
(`map_handle`) is the actual pointer to the mapped pixel memory — that's
what gets read/written through. Its `&my_stride`/`&map_data`
out-parameters are filled in as side effects: `my_stride` becomes the
real row pitch, and `map_data` becomes an **opaque, driver-private
token** — not pixel memory at all, just a handle `gbm_bo_unmap()` needs
later to know which mapping to tear down. The name `map_data` is
genuinely misleading (real GBM API, not a naming choice made here) —
it sounds like "the mapped data," but it isn't. Getting this backwards
(writing through `map_data` instead of `map_handle`) is exactly what
caused a heap-buffer-overflow crash earlier in this exercise — 120 rows
of pattern data slammed into an ~80-byte internal driver bookkeeping
structure instead of the real, much larger pixel buffer.

Mapping covers `x=0,y=0` through the full `width`/`height` (the whole
buffer), declaring intent as `GBM_BO_TRANSFER_WRITE` — a *different*
enum from `GBM_BO_USE_LINEAR` (allocation-time usage vs. mapping-time
transfer intent — two unrelated concepts that happen to both look like
"flags" passed to different GBM functions).

```cpp
for (int y = 0; y < 120; y++) {
    uint8_t *row = (uint8_t*)map_handle + y * my_stride;
    row[0] = (uint8_t)y ;
}
```
The actual pattern write — walks all 120 rows using the *real*
`my_stride` (never a guessed `width*4`), and in each row writes exactly
one recognizable byte: the row's own index. `map_handle` here is
correctly the real pixel pointer (the return value), which is what makes
this safe.

```cpp
gbm_bo_unmap(my_bo, map_data);
```
Releases the CPU mapping — correctly passing `map_data` (the opaque
token) here, not `map_handle`. Must happen before other consumers (like
the independent `mmap()` below) can reliably see the buffer's contents.

```cpp
int dma_fd = gbm_bo_get_fd(my_bo);
```
Exports this specific buffer as a dma-buf file descriptor — a second,
completely independent handle to the *same* physical memory, valid
outside GBM and even outside this process. This is exactly the
mechanism `EGL_EXT_image_dma_buf_import` consumes in the real project.

```cpp
size_t buf_size = (size_t)my_stride * height;
void *check = mmap(nullptr, buf_size, PROT_READ, MAP_SHARED, dma_fd, 0);
```
The independent verification step: map the buffer's *exported fd*
directly, via the raw `mmap()` syscall, completely bypassing GBM's own
API. `nullptr` lets the kernel pick the address; `buf_size` is computed
from the *real* stride, not a guess; `PROT_READ` since this is read-only
verification; **`MAP_SHARED`** so the real, live memory is seen rather
than a private copy-on-write snapshot (`MAP_PRIVATE` was a bug in an
earlier round); `dma_fd` — the buffer's own fd, not the render-node fd;
offset `0`.

```cpp
if (dma_fd == -1){
    printf("error fstat\n");
}
```
A leftover check from an earlier file-based detour — the error message
("error fstat") doesn't even match what's being checked anymore (there's
no `fstat` call left in this file). Also in the wrong place: it runs
*after* `dma_fd` was already used in the `mmap()` call above, so a real
failure here would already have caused `mmap()` to run with an invalid
fd first. Harmless in practice only because `dma_fd` never actually
fails here — worth deleting or moving earlier, and fixing the message,
as a cleanup pass.

```cpp
bool ok =true;
uint8_t *bytes = (uint8_t *)check;
for(int y = 0; y < height; y++){
    if(bytes[y*my_stride] != y){
        printf("THERE IS A MISMATCH !!!\n");
        ok = false;
        break;
    }
}
if(ok){
    printf("Everything went well !\n");
}
```
The actual comparison: walk the same 120 rows through `check` (the
independent mapping) and confirm each row's first byte still equals its
row index — the same values written through `map_handle` earlier, now
read back through a totally different access path. Any mismatch reports
and stops early (`break`); completing the loop without one means
success. This is the actual proof that the exported fd genuinely refers
to the same memory GBM wrote through — the entire point of Task 3.

```cpp
munmap(check, buf_size);
close(dma_fd);
gbm_bo_destroy(my_bo);
gbm_device_destroy(my_device);
close(fd);
return 0;
```
Cleanup, in dependency order — each step undoing exactly the setup step
that created it, in reverse: unmap the independent verification mapping,
close the exported fd, destroy the buffer object, destroy the device
wrapper, close the original render-node fd. The same "reverse of
construction order" principle shows up in `WaylandPreviewWindow::cleanup()`
and `teardown_egl()` in the real project too — a pattern worth
recognizing everywhere once you've seen it once.

## Bugs actually hit building this file (worth remembering)

1. **Use-after-free**: printing driver info *after* `drmFreeVersion()`
   instead of before.
2. **Circular deadlock** *(from the C++ concurrency module, same
   category)*: not directly hit here, but the same "state ordering
   matters" lesson.
3. **Missing `gbm_device_destroy()`**: a real, measured 86KB/764-allocation
   leak in the actual GPU driver stack (`radeonsi_dri.so`), caught by
   `LeakSanitizer` — not hypothetical, an actual observed leak.
4. **`gbm_bo_map()`'s two outputs reversed**: writing through the opaque
   `map_data` token instead of the real `map_handle` return value —
   caused a genuine heap-buffer-overflow segfault, root-caused via a
   step-by-step diagnostic script and confirmed against the real
   `/usr/include/gbm.h` header rather than trusting a remembered API
   shape.
5. **`MAP_PRIVATE` vs `MAP_SHARED`**: using a private copy-on-write
   mapping when the whole point was to observe the *real*, shared
   memory.
6. **Dangling `if` with no braces**: an `if` with no body silently
   "adopting" the entire next `if` statement as its own body — valid,
   unambiguous C++ grammar that nonetheless does the opposite of what
   the blank-line-separated layout visually suggests.
