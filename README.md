# RGSEngine v2.0

**RGSEngine** is a 3D game engine, the goal of this second release is to get rid of our dependence on FBX to run our games, organize resources in a consistent way, and introduce several optimizations and tools in the graphics / editor pipeline such as frustum culling, octree acceleration structures, custom asset formats, scene serialization, and a full resource management workflow.

## 📎 Repository Link

[https://github.com/Loving-Studios/RGSEngine](https://github.com/Loving-Studios/RGSEngine)

## 👥 Team Members

* **Pablo (XXPabloS):** [https://github.com/XXPabloS](https://github.com/XXPabloS)
* **Victor (TheWolfG145):** [https://github.com/TheWolfG145](https://github.com/TheWolfG145)
* **Claudia (Claurm12):** [https://github.com/Claurm12](https://github.com/Claurm12)

---

## 📘 How to Use the Engine

### 🎮 Controls

The camera system is designed to mimic the Unity-style controls:

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
* **Deselect (Press Q):**
    * Deselects the currently selected `GameObject`, and hides the gizmo for comfort.
* **Mouse Picking (Click Left Mouse Button):**
    * Select the desired `GameObject` with a mouse click, activate the object's gizmo and sets orbit target.

## 🔍 Mouse Picking
Implemented using raycasting through the octree with optional debug visualization.

---

## 🖥️ Editor Interface

The UI is built with ImGui and features a fully dockable interface. All windows can be moved, resized, and attached to the main viewport.

## 📂 Hierarchy Window

Lists all `GameObjects` in the scene.

* **Visibility Checkbox:** Allows enabling or disabling any `GameObject` (and its children). Disabled objects are not rendered or updated.
* **Selection:** Clicking an object selects it for the **Inspector** and sets it as the **Orbit** target.
* **Create Empty Button:** Clicking the button creates an empty GameObject, which is added to the **Inspector** as `GameObject_empty`.
* **Delete GameObject:** With an `GameObject` selected in the **Hierarchy** and the **Supr** key or **Delete** key is pressed, the `GameObject` is deleted.
* **Right-Click Menu:** Context menu to create child objects or delete the current selection.
* **Drag & Drop Reparenting:** You can drag a `GameObject` and drop it onto another to create a parent-child relationship. The engine automatically prevents illegal operations (like parenting an object to itself or its own descendants).
* **Translation, Rotation, and Scaling:** By clicking on each option, Imguizmo Ui is activated for each action selected in the `GameObject`.
* **Local and World Transformation:** By clicking on each option, it is possible to modify the local or global position of the `GameObject`.

## 🧩 Inspector Window

Displays detailed information and allows real-time editing of the selected `GameObject` components.

### **Transform Component**
* **Numeric Editing:** distinct fields for Position, Rotation (Euler degrees), and Scale.
* **Visual Manipulation (ImGuizmo):** Selecting an object activates the **ImGuizmo** overlay in the scene view. You can switch between **Translate (W)**, **Rotate (E)**, and **Scale (R)** modes, as well as toggle between **Local** and **World** space coordinates using the UI toggles or hotkeys.

### **Mesh Component**
* **Native File Explorer:** Clicking "Select Mesh..." opens the **Windows Native File Explorer**, allowing you to browse your disk and load any `.fbx` file directly into the mesh component.
* **Drag & Drop Assignment:** You can drag a mesh file from the Assets window directly into this slot.
* **Normals Debug:** Includes checkboxes to visualize **Vertex Normals** and **Face Normals** in the scene view for debugging shading issues.
* **Metadata:** Displays internal resource paths (`.rgs`), VRAM buffer IDs (VAO/VBO/IBO), and total index count.

### **Texture Component**
* **Native File Explorer:** Clicking "Select Texture..." opens the **Windows Native File Explorer** to load `.png`, `.jpg`, `.dds`, or `.tga` files.
* **Transparency & Blending:**
    * **Alpha Test:** Toggle support for cutout transparency (e.g., foliage) with an adjustable **Alpha Threshold** slider.
    * **Blending:** Toggle support for semi-transparent surfaces (e.g., glass). Includes selectable **Source** and **Destination** blend factors (e.g., `GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA`).
* **Default Checker:** A toggle to revert to the internal debug checkerboard pattern if a texture is missing.
* **Metadata:** Shows texture dimensions, format, and OpenGL Texture ID.

### **Camera Component**
* *Note: This section is visible only if the selected object has a Camera Component.*
* **Frustum Visualization:** Modifying these values updates the visual frustum lines in the scene in real-time.
* **Field of View (FOV):** Slider to adjust the vertical FOV (1.0 to 179.0 degrees).
* **Clipping Planes:** Sliders to adjust the **Near** and **Far** clipping planes.

## 🖥 Console Window

A real-time log that captures all `LOG()` messages from the engine. It includes auto-scrolling and a **Clear** button. It tracks:
* Module initialization steps.
* Asset import processes (Assimp/DevIL logs).
* Errors and Warnings.

## ⚙️ Configuration Window

Adjust engine settings and view system information.

### **Engine & Editor Settings**
* **Camera Controls:** Sliders for `Camera Speed`, `Camera Sensitivity`, and `Camera FOV`.
* **Window Controls:** Live toggles for `Fullscreen`, `Borderless`, and `Resizable` modes without restarting. Includes a `Reset Size` button.
* **FPS Graph:** A rolling histogram plotting the framerate of the last 100 frames.

### **Hardware Information**
* **Hardware & Software:** Displays CPU cores, System RAM, GPU Vendor/Renderer, and library versions (SDL3, OpenGL, ImGui, DevIL).
* **VRAM Monitor (NVIDIA):** Uses the `GL_NVX_gpu_memory_info` extension to display a progress bar with Total, Available, and Used Video RAM.

### Main Menu Bar

* **File > Exit:** Shuts down the application.
* **View:** Toggle visibility for all editor windows. Includes a `Reset Layout` option to restore the default window docking.
* **Create:** Create and spawn basic 2D and 3D geometric primitives (Pyramid, Cube, Sphere, etc.) into the scene.
* **Help:** Provides links to the project's Documentation, Bug Reporter, and Releases, as well as an "About" window.

# 📁 Assets Window

A dedicated explorer for browsing and managing assets inside the `/Assets` folder.

### Features:
- **Tree-like folder browser:** Visualizes the directory structure of your project.
- **Drag & drop import:** Supports dragging files directly from Windows Explorer into the engine.
- **Smart Deletion:** Deleting an asset from the editor automatically removes the associated metadata and the processed binary files in `/Library`.
- **Automatic Refresh:** The engine detects changes and updates the file tree dynamically.
- **Asset Metadata Display:**
  - File type identification.
  - UUID (Unique Identifier).
  - Reference counting (shows how many GameObjects use this resource).

---

# 🧬 Core Systems

## 🏗 Custom File Format

To optimize loading times and independence, the engine converts all assets into custom internal binary formats stored inside `/Library`. This ensures that assets are loaded efficiently without parsing heavy formats like FBX at runtime.

| Asset Type | Source Format | Engine Format | Path in Library |
|------------|---------------|---------------|-----------------|
| Meshes     | .fbx, .obj    | **.rgs** | `/Library/Meshes/` |
| Textures   | .png, .jpg, .dds | **.rgst** | `/Library/Textures/` |
| Metadata   | n/a           | **.meta** | Next to original asset |

### `.meta` files contain:
- **UUID:** A unique hash ensuring references are maintained even if files are renamed.
- **Resource Type:** Identifies if it is a mesh, texture, etc.
- **Timestamp:** Used to detect modifications and trigger re-importing.
- **Import Options:** Settings applied to the asset.

The `/Library` folder is fully volatile; it can be deleted and the engine will automatically regenerate it from the `/Assets` folder using the `.meta` files on the next startup.

## 💾 Scene Serialization

The engine supports full scene serialization. The current state of the scene can be saved and reloaded, preserving:
- GameObjects hierarchy and parent-child relationships.
- Component data (Transform values, Mesh references, Texture references, Camera settings).
- Linkage to resources via UUIDs.

*Note: The scene state is automatically captured when entering Play Mode.*

## 🎬 Simulation Controls (Play / Pause / Stop)

Located in the main toolbar, the simulation system allows testing the game within the editor:
- **Play:** Captures the current scene state in memory and starts the internal game clock/update loop.
- **Pause:** Freezes the `Time::deltaTime` and updates, allowing for inspection.
- **Stop:** Halts the simulation and **restores** the scene to the exact state captured before pressing Play.

## 📦 Bounding Boxes (AABB)
Each mesh includes an automatically generated **Axis-Aligned Bounding Box (AABB)**. This is calculated during the import process by analyzing the mesh vertices and is used for:
- Debug visualization in the editor.
- Frustum Culling calculations.

## 🎯 Frustum Culling & Optimization

The engine implements optimizations to ensure performance:
- **Frustum Culling:** Objects outside the camera's viewing frustum are discarded from the render pipeline.
- **Octree Acceleration:** An Octree structure is used to partition the space, significantly speeding up spatial queries like Frustum Culling and Mouse Picking by avoiding iterating over every object in the scene.

---

## 📦 Resource Management

A robust **Resource Manager** has been implemented.

### Key Features:
- **Reference Counting:** Resources (Meshes/Textures) are loaded into memory only once. Multiple GameObjects share the same resource pointer. If the reference count drops to zero, the resource can be unloaded.
- **Importer Settings:**
    - **Textures:** Options for Filtering (Nearest/Linear), Wrapping, and Flip X/Y.
    - **Meshes:** Global scale adjustments and coordinate system fixes.
- **Drag & Drop:** Fully integrated with the UI to assign resources to components easily.

## 🌆 Default Scene Loading

The engine includes a sample scene to demonstrate capabilities. On startup, it automatically loads:

### **`StreetEnvironment.fbx`**
- The model is automatically imported into `.rgs` and `.rgst` formats.
- A full hierarchy of GameObjects is generated.
- If the file is missing, the engine falls back to a default `BakerHouse.fbx` or a primitive.

---

## ⭐ Extra Features and 📝 Additional Comments

* **Native Windows File Explorer:** Integration with the Windows API (`GetOpenFileNameA`) to allow browsing and selecting Meshes and Textures directly from the OS file dialogs, improving workflow efficiency.
* **Advanced Transparency Support:** The engine supports both **Alpha Testing** (cutout) and **Alpha Blending** (transparency) with configurable Source/Destination factors directly from the Inspector.
* **Editable Transform Component:** The Inspector allows direct modification of Position, Rotation, and Scale at runtime with immediate visual feedback.
* **ImGuizmo Integration:** Full support for visual manipulation (Translate, Rotate, Scale) directly in the scene view.
* **VRAM Monitor (NVIDIA):** A dedicated bar in the Configuration window shows real-time Video RAM usage, budget, and available memory (Specific for NVIDIA GPUs using `GL_NVX_gpu_memory_info`).
* **Live Window Controls:** Toggle `Fullscreen`, `Borderless`, and `Resizable` modes instantly without restarting the application.
* **Default Docking Layout:** The editor boots with a clean, pre-defined window layout (Hierarchy left, Inspector right, Console bottom) for better UX.

## ⚖️ Licenses

All external libraries used in this project (SDL3, OpenGL, ImGui, MathGeoLib, Assimp, DevIL) are open source.
