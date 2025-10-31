# RGSEngine v0.1

**RGSEngine** is a 3D game engine, the goal of this first release is to build a a geometry viewer with drag & drop, an orbital camera, and a basic UI editor made with ImGui.

## Repository Link

[https://github.com/Loving-Studios/RGSEngine](https://github.com/Loving-Studios/RGSEngine)

## Team Members

* **Pablo (XXPabloS):** [https://github.com/XXPabloS](https://github.com/XXPabloS)
* **Victor (TheWolfG145):** [https://github.com/TheWolfG145](https://github.com/TheWolfG145)
* **Claudia (Claurm12):** [https://github.com/Claurm12](https://github.com/Claurm12)

---

## How to Use the Engine

### Camera Controls

The camera system is designed to mimic the controls found in Unity:

* **Free Look Mode (Hold Right Mouse Button):**
    * **Move Mouse:** Look around the scene freely.
    * **W, A, S, D:** Fly the camera in an FPS-like style.
    * **Hold SHIFT:** Doubles the camera's movement speed.
* **Orbit Mode (Hold ALT + Left Mouse Button):**
    * Orbits the camera around the currently selected `GameObject`. Orbiting is disabled if no object is selected.
* **Zoom (Mouse Wheel):**
    * Adjusts the camera's Field of View (FOV).
* **Focus (Press F):**
    * Instantly frames the currently selected `GameObject` in the center of the view.

### Editor Interface

The UI is built with ImGui and features a fully dockable interface. All windows can be moved, resized, and attached to the main viewport.

* **Hierarchy:** Lists all `GameObjects` in the scene.
    * **Visibility Checkbox:** Allows enabling or disabling any `GameObject` (and its children). Disabled objects are not rendered or updated.
    * **Selection:** Clicking an object selects it for the **Inspector** and sets it as the **Orbit** target.
* **Inspector:** Displays the Components of the selected `GameObject`.
    * **Transform:** View and **edit** the Position, Rotation (in degrees), and Scale of the object.
    * **Mesh:** Shows mesh information (VAO/VBO/IBO IDs, Index Count).
    * **Texture:** Shows texture information (Texture ID, Size, Path).
* **Console:** A real-time log that captures all `LOG()` messages from the engine, including module initialization, object creation, and errors.
* **Configuration:** Adjust engine settings and view system information.
    * **FPS Graph:** A histogram plotting the last 100 frames.
    * **Camera Controls:** Sliders for `Camera Speed`, `Camera Sensitivity`, and `Camera FOV`.
    * **Window Controls:** Checkboxes for `Fullscreen`, `Borderless`, `Resizable`, and a `Reset Size` button.
    * **Hardware & Software:** Displays CPU cores, System RAM, GPU Vendor/Renderer, VRAM (on NVIDIA cards), and all library versions (SDL3, OpenGL, ImGui, DevIL).

### Main Menu Bar

* **File > Exit:** Shuts down the application.
* **View:** Toggle visibility for all editor windows. Includes a `Reset Layout` option to restore the default window docking.
* **Create:** Create and spawn basic 2D and 3D geometric primitives (Pyramid, Cube, Sphere, etc.) into the scene.
* **Help:** Provides links to the project's Documentation, Bug Reporter, and Releases, as well as an "About" window.

---

## Extra Features

* **Editable Transform Component:** The Inspector's Transform component is not read-only; Position, Rotation, and Scale can be modified at runtime.
* **Default Docking Layout:** The editor boots with a clean, pre-defined window layout (Hierarchy left, Inspector right) which can be restored via `View > Reset Layout`.
* **Live Window Controls:** `Fullscreen`, `Borderless`, and `Resizable` modes can be toggled live from the Configuration panel.
* **VRAM Monitor:** The Configuration panel includes VRAM monitoring (Total, Available, and Usage) for NVIDIA GPUs.
