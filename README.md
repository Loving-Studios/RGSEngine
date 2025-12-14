# RGSEngine v2.0

**RGSEngine** is a 3D game engine, the goal of this second release is to get rid of our dependence on FBX to run our games, organize resources in a consistent way, and introduce several optimizations and tools in the engine such as frustum culling, octree acceleration structures, custom asset formats, scene serialization, and a full resource management workflow.

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

### Scene Management
- **Save Scene:** Go to `File > Save Scene`. A Windows dialog will appear. Choose a location and name (`SceneExample.json`).
- **Load Scene:** Go to `File > Load Scene` and select a `.json` file.
- **Primitives:** Create primitives from the `Create` menu. They are automatically saved to the Library to ensure visibility after loading.

## 🔍 Mouse Picking
Precise object selection implemented via **Raycasting**. The system projects a ray from the camera through the mouse cursor into the 3D world.

* **High Performance:** Uses the **Octree** to instantly discard objects far from the ray, avoiding unnecessary checks.
* **Pixel-Perfect Precision:** The system performs a multi-step check for accuracy:
    1. **Broad Phase:** Checks intersection with the object's AABB (Axis-Aligned Bounding Box).
    2. **Narrow Phase:** If the AABB is hit, it iterates through the mesh triangles to ensure the click actually touches the geometry.
* **Editor Integration:** Clicking an object automatically highlights it in the Hierarchy.

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
* **Native File Explorer:** Clicking `Select Mesh...` opens the **Windows Native File Explorer**, allowing you to browse your disk and load any `.fbx` or `.rgs` file directly at runtime into the mesh component.
* **Mesh Loading:** You can add a mesh file from the **Assets window** directly by clicking `Load to Scene`.
* **Geometry Visualization:** Includes checkboxes to visualize **Vertex Normals**, **Face Normals** and **Axis-Aligned Bounding Boxes (AABB)** in the scene view.
* **Metadata:** Displays internal resource paths `.rgs`, GPU buffer IDs (VAO/VBO/IBO).

### **Texture Component**
* **Material Color:** Real-time modification of the material's base diffuse color. This value acts as a tint when a texture is applied or defines the solid material color when no texture is present.
* **Native File Explorer:** Clicking `Select Texture...` opens the **Windows Native File Explorer** to load `.png`, `.jpg`, `.dds`, or `.tga` files and loading optimized custom engine textures `.rgst` directly at runtime.
* **Transparency & Blending:**
    * **Alpha Test:** Toggle support for cutout transparency (ideal for foliage or grates) with an adjustable **Alpha Threshold** slider.
    * **Blending:** Toggle support for semi-transparent surfaces (glass, water). Includes selectable **Source** and **Destination** blend factors (Standard, Additive, Multiplicative).
* **Default Checker:** One-click switch to the internal checkerboard pattern for UV debugging. Automatically forces a white tint for accurate pattern visualization and restores the previous material state upon deactivation.
* **Metadata:** Shows texture dimensions, format, internal library path and OpenGL Texture ID.

### **Camera Component**
*Note: This section is visible only if the selected object has a Camera Component.*

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
- **Drag & drop import:** Supports dragging files directly from **Windows Explorer** into the engine.
- **Smart Deletion:** Deleting an asset from the editor automatically removes the associated metadata and the processed binary files in `/Library`.
- **Automatic Refresh:** The engine detects changes and updates the file tree dynamically.
- **Asset Metadata Display:**
  - File type identification.
  - UUID (Unique Identifier).
  - Reference counting (shows how many GameObjects use this resource).

---

# 🧬 Core Systems

## 🏗 Custom File Format & Smart Importer

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

### Smart Import Features:
* **Automatic Scaling:** The importer analyzes the bounding box of incoming models and automatically normalizes their scale to ensure they fit comfortably within the scene view, preventing massive or microscopic imports.
* **Texture Path Recovery:** If a model's texture path is broken (common in FBX files), the engine intelligently searches for the texture filename in the local directory to restore the link automatically.
* **Tangent Generation:** The importer calculates tangent space (`aiProcess_CalcTangentSpace`) for all meshes, readying the pipeline for normal mapping.

- **Custom File Format (.rgs & .rgst):**
  - **Library System:** Implemented a Unity-style `Library/` folder.
  - **Meshes:** Imported FBX files are converted to optimized binary `.rgs` files.
  - **Textures:** Images (PNG, JPG, TGA) are converted to optimized binary `.rgst` files.
  - **Fast Loading:** The engine prioritizes loading from the binary `Library` format for performance.

- **Primitives Persistence:**
  - Procedural primitives (Cube, Sphere, Pyramid, etc.) are automatically generated, saved to `Library`, and serialized correctly.

## 💾 Scene Serialization

The engine supports full scene serialization. The current state of the scene can be saved and reloaded, preserving:
- Full Scene Saving/Loading to **JSON** format (`nlohmann/json`).
- GameObjects hierarchy and parent-child relationships.
- Component data (Transform values, Mesh references, Texture references, Camera settings).
- Linkage to resources via UUIDs.
- **Windows Native Dialogs:** Integrated `SaveFileDialog` and `OpenFileDialog` for easy scene management.

## 🎬 Simulation Controls (Play / Pause / Stop)

Located in the main toolbar, the simulation system allows testing the game within the editor:
- **Play:** Captures the current scene state in memory and starts the internal game clock/update loop.
- **Pause:** Freezes the `Time::deltaTime` and updates, allowing for inspection.
- **Stop:** Halts the simulation and **restores** the scene to the exact state captured before pressing Play.

## 📦 Bounding Boxes & Visual Debugging

The engine includes robust visualization tools for debugging:
- **Reference Grid:** A procedural infinite grid on the XZ plane to assist with spatial orientation and object placement.
- **AABB (Axis-Aligned Bounding Box):** Automatically generated for every mesh to support culling.
- **Normals Visualization:** Options in the Inspector to render **Vertex Normals** and **Face Normals** vectors, helping to identify shading errors in imported geometry.
- **Frustum Lines:** Visual representation of the camera's viewing volume.

## 🎯 Frustum Culling & Optimization

The engine implements advanced optimizations to ensure high performance by rendering only what is necessary:
- **Frustum Culling:** Objects outside the camera's viewing frustum are discarded from the render pipeline, saving valuable GPU resources.
   -**Dynamic Camera Culling:** If no camera is selected, culling is calculated based on the Editor's view. When a Game Camera is selected, the system automatically switches to use that camera's perspective for culling calculations. This allows developers to debug exactly what the player will see from any camera in the scene without modifying code.
- **Octree Acceleration:** An Octree data structure partitions the 3D space, significantly speeding up spatial queries. This optimization applies to both Frustum Culling and Mouse Picking, avoiding the need to iterate over every single object in the scene.
### 🛠️ Debugging Tools
You can visualize the optimization process in real-time via the `Configuration -> Render` menu.

When **"Visualize Culling"** is enabled, bounding boxes change color to represent their state:
| Color | State | Description |
| :---: | :--- | :--- |
| 🟢 | **Visible** | Object is inside the frustum and being rendered. |
| 🔴 | **Culled** | Object is outside the frustum and discarded. |
| 🔵 | **Selected** | The currently selected object in the hierarchy. |

#### Performance Statistics
Open `View -> Performance Stats` to monitor efficiency metrics:
* **Total Objects:** Total count in the scene.
* **Rendered vs Culled:** Real-time counter of objects being drawn vs. skipped.
* **Octree Efficiency:** Visual bar showing the percentage of collision checks avoided thanks to the Octree.

#### 2. Octree Debugging & Control
Accessible via `View -> Octree Debug`. This window provides deep control over the spatial partitioning:
* **Visualize Structure:** You can render the actual Octree nodes in the scene.
* **Rebuild Octree:** A button to force a complete recalculation of the spatial tree. Useful if objects have been moved significantly or the scene structure has changed.
* **Octree Stats:** View real-time data on node count, max depth, and total objects inside the tree.
---

## 📦 Resource Management

A robust **Resource Manager** has been implemented.

### Key Features:
- **Reference Counting:** Resources (Meshes/Textures) are loaded into memory only once. Multiple GameObjects share the same resource pointer. If the reference count drops to zero, the resource can be unloaded.
- **Safety Deletion:** The engine prevents the deletion of assets that are currently referenced by active GameObjects in the scene, preventing crashes.
- **Library Regeneration:** If the `/Library` folder is deleted, the engine can fully reconstruct it from the source `/Assets` and `.meta` files.
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
