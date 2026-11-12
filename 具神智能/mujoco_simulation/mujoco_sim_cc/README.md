# SO-ARM101: Qt 6 / C++ / MuJoCo

A C++17 Qt 6 migration of `run_sim.py`, with a Qt Widgets window, MuJoCo native OpenGL rendering, an automated A/B pick-and-place state machine, and `.npz` arm-only replay. The original XML/STL assets are not modified.

## Architecture

- `Simulation`: MuJoCo scene/model ownership, forward kinematics, grab/release with relative transforms, scripted motion.
- `Replay`: reads the `traj.npy` member of a `.npz` ZIP archive using the system `tar` utility; supports little-endian float32/float64 C-contiguous arrays of shape `(N,7)`.
- `Viewer`: `QOpenGLWidget` with MuJoCo's official `mjv_*` and `mjr_*` renderer. Qt creates the GL context and window. Qt3D/Qt Quick 3D renderers cannot natively consume MuJoCo `mjvScene`, so they are not used here.
- Qt `QMatrix4x4` handles 4x4 transforms; Eigen 3.4 handles array-based mesh centroid arithmetic. Qt classes handle UI, files, strings, containers, timing, and CLI parsing.

## Prerequisites

- Qt 6 (Widgets, Gui, OpenGL, OpenGLWidgets), CMake >=3.24 and a C++17 MSVC kit.
- Existing `E:/Mujoco/build` compiled RelWithDebInfo MuJoCo, sources under `E:/Mujoco`.
- Existing `E:/eigen-3.4.0`.
- Scene assets under `F:/mujoco_sim`: `scene.xml`, `so101_new_calib.xml`, `assets/*.stl`.
- For `.npz` replay, `tar` must be available in `PATH` (standard on modern Windows). Qt does not expose a stable public ZIP reader, so the system tool avoids a new third-party library.

## Build

Open `CMakeLists.txt` as a project in Qt Creator and select a Qt 6 + MSVC 2022 kit matching the MuJoCo build ABI. Configure with the existing defaults, or override:

```
-DSO_ARM101_RESOURCE_ROOT=F:/mujoco_sim
-DMUJOCO_SOURCE_DIR=E:/Mujoco
-DMUJOCO_BUILD_DIR=E:/Mujoco/build
-DEIGEN_ROOT=E:/eigen-3.4.0
```

If Qt's package path cannot be found, set `CMAKE_PREFIX_PATH` to your Qt installation's `msvc2022_64` folder via Qt Creator.

## Run

```
so_arm101_sim.exe
so_arm101_sim.exe --replay F:/mujoco_sim/demo.npz --start 18 --speed 1.0
so_arm101_sim.exe --replay F:/mujoco_sim/demo.npz --start 18 --len 12
so_arm101_sim.exe --resource F:/mujoco_sim
```

The scripted mode dynamically adds the red slider to a **temporary copy** of the XML resources and cycles A -> B -> A. Replay loads the unmodified scene and only animates the real-arm joint trajectory, as in the default `run_sim.py --replay` path. Camera: left-drag rotate, right-drag pan, wheel zoom.

## Important distinctions / verification

- This is **kinematic animation**: the arm's `qpos` is assigned and `mj_forward` updates poses. It does not run `mj_step` actuator dynamics, identical to the Python demo's concept.
- The scripted slider is manually attached/released based on the gripper transform; it is *not* a physics-based grasp.
- The temporary scene stages the three inputs only: scene.xml, so101_new_calib.xml, and assets directory. If the XML references other files, include those in the staging helper.
- Scripted grasp-point calculation matches the Python mesh centroid calculation, with the average evaluated by Eigen. This requires `mesh_vert` to use MuJoCo's `float` vertex layout.
- Replay ZIP extraction assumes Windows `tar` can unpack `.npz` into `traj.npy`; no Python runtime is required.
- On some GPU/Qt/MuJoCo combinations the framebuffer and profile may require minor driver-specific configuration. This project has **not been compiled on your Windows machine** because its local dependency paths and model assets are not available in this environment. In particular, verify GL render and model load on your Qt Creator/MSVC setup before trusting visual behavior.
- If the build has a linker error on MuJoCo functions, ensure `mujoco.lib` and Qt are built for the same MSVC x64 architecture/configuration.
