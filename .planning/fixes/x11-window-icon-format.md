# Bug Fix Report: X11 Window Icon Format Compatibility

- **Issue ID:** FIX-04
- **File Target:** `.planning/fixes/x11-window-icon-format.md`
- **Component:** Presentation / Application Window Icon (`main.cpp`, `IconData.hpp`, `icon.png`, `test_rules.cpp`)
- **Status:** **VERIFIED & RESOLVED**

---

### [BUG-FIX]: X11 Default Fallback Icon on Window Titlebar / Taskbar Panel

- **Defect Summary:** When running `organism_sim`, the window manager taskbar panel and window titlebar displayed the default generic X11 "X" icon rather than the custom embedded avatar icon.
- **Root Cause Analysis:**
  1. Raylib's desktop backend (`rcore_desktop.c:543`) requires window icon images to be strictly formatted as `PIXELFORMAT_UNCOMPRESSED_R8G8B8A8` (format ID 7).
  2. The initial conversion from `avatar.jpg` produced an 8-bit Grayscale PNG (`PIXELFORMAT_UNCOMPRESSED_GRAYSCALE`, format ID 1).
  3. Because `image.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8`, Raylib logged `WARNING: GLFW: Window icon image must be in R8G8B8A8 pixel format` and silently aborted the `glfwSetWindowIcon` call.
  4. Consequently, the X11 `_NET_WM_ICON` window property was never set on the X11 window, causing the X11 window manager to fall back to the default X11 icon.
- **Target Files:**
  - `include/app/IconData.hpp` (Regenerated from 32-bit RGBA PNG, color type 6)
  - `src/app/main.cpp` (Guaranteed R8G8B8A8 format via `ImageFormat` and multi-resolution `SetWindowIcons`)
  - `tests/test_rules.cpp` (Regression test `TC_ICON_05` verifying RGBA color type 6)

---

### Reproduction & Fix Evidence

#### 1. Reproduction Log (Pre-Fix)
```text
Loaded image format without conversion: 1 (Expected R8G8B8A8: 7)
WARNING: GLFW: Window icon image must be in R8G8B8A8 pixel format
```

#### 2. Post-Fix Verification Log
```text
Loaded image format without conversion: 7 (Expected R8G8B8A8: 7)
=======================================
 Running Headless Simulation Test Suite
=======================================
[ RUN      ] TC_ICON_05_PNG_Color_Type_RGBA
[       OK ] TC_ICON_05_PNG_Color_Type_RGBA
=======================================
 Test Summary: 59 passed, 0 failed (294 total assertions)
=======================================
```

---

### Patch Diffs

#### 1. Main Entrypoint Multi-Resolution Window Icon Setup (`src/app/main.cpp`)
```diff
--- a/src/app/main.cpp
+++ b/src/app/main.cpp
@@ -11,8 +11,18 @@
-    // Apply embedded application window icon
-    Image app_icon = LoadImageFromMemory(".png", app::ICON_PNG_DATA, app::ICON_PNG_DATA_LEN);
-    if (app_icon.data != nullptr) {
-        SetWindowIcon(app_icon);
-        UnloadImage(app_icon);
+    // Apply embedded application window icon across multiple resolutions (200x200, 64x64, 32x32, 16x16)
+    Image icon_base = LoadImageFromMemory(".png", app::ICON_PNG_DATA, app::ICON_PNG_DATA_LEN);
+    if (icon_base.data != nullptr) {
+        ImageFormat(&icon_base, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
+        Image icon_64 = ImageCopy(icon_base);
+        ImageResize(&icon_64, 64, 64);
+        Image icon_32 = ImageCopy(icon_base);
+        ImageResize(&icon_32, 32, 32);
+        Image icon_16 = ImageCopy(icon_base);
+        ImageResize(&icon_16, 16, 16);
+
+        Image icons[4] = {icon_base, icon_64, icon_32, icon_16};
+        SetWindowIcons(icons, 4);
+
+        UnloadImage(icon_16);
+        UnloadImage(icon_32);
+        UnloadImage(icon_64);
+        UnloadImage(icon_base);
     }
```

---

### Side-Effect Audit
- [x] Public API contracts preserved.
- [x] Simulation logic, pathfinding, and metabolism unaffected.
- [x] No memory leaks: all 4 temporary mipmap images (`icon_base`, `icon_64`, `icon_32`, `icon_16`) cleanly unloaded via `UnloadImage`.
- [x] All 59 test cases pass under AddressSanitizer and UndefinedBehaviorSanitizer.

---

*Lifecycle Citation:*
Following the Engineering Lifecycle Workflow:
- To initialize version control, check hygiene, and publish the repository to GitHub, invoke **`github-repo-publisher`**.

