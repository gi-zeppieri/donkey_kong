# Donkey Kong 1981 (C++ / Raylib Fork)

This project is a modern fork of an existing *Donkey Kong* open-source port. The primary goal of this repository is to modernize the graphics backend, achieve pixel-perfect arcade aesthetics, and fix underlying visual and logical bugs.

## Graphics Library Update
The original codebase's rendering system was overhauled and ported to use the Raylib framework. This modernization effort involved:
* Migrating the asset loader to utilize Raylib's `LoadTexture()` and `Texture2D` structures.
* Replacing legacy drawing methods with Raylib's `DrawTexturePro()` to allow for dynamic horizontal flipping and precise scaling.
* Rewriting the `sprites.cc` coordinate maps to pull perfectly cropped frames from a consolidated arcade sprite sheet, ensuring 100% accurate pixel placement.

## How to Compile and Run
Ensure you have CMake, a modern C++ compiler, and Raylib installed on your system. 

1. **Build the game:** Run the following commands from the root directory of the project:
`mkdir build`
`cd build`
`cmake ..`
`cmake --build .`

2. **Run the executable:** Ensure you launch the game from a directory where it can properly locate the `assets/` folder:
`./donkey_kong_1981`

---

## Summary of Encountered Issues & Fixes

### I. Collision & Physics Issues

**Ladder Hitboxes**
* **Issue:** The simplified physics of the cloned engine required adjusting Mario's alignment to successfully mount and dismount ladders without getting snagged on platform geometry.
* **Fix:** Realigned the player state coordinates to allow smooth transitions between the vertical ladder grid and horizontal platform planes.

### II. Rendering & Visual Artifacts

**Background Color Keying**
* **Issue:** Early asset extraction methods failed to properly strip the background colors, resulting in solid cyan and dark blue boxes trapping the characters. 
* **Fix:** Implemented precise RGB alpha-masking loops to convert those specific arcade background shades to transparent pixels.

**Sprite Grid Misalignment**
* **Issue:** Arbitrary hardcoded pixel coordinates in the original source caused Donkey Kong's body to be sliced in half and left Pauline completely invisible.
* **Fix:** Mapped the exact X and Y pixel bounding boxes directly from the native arcade sprite sheet to ensure pixel-perfect rendering in the C++ engine.

### III. Game State & Logic Flaws

**Inverted Controls**
* **Issue:** Mario walked left when the right key was pressed because the game engine automatically flips sprites based on directional input, and feeding it pre-flipped image assets caused a double-inversion.
* **Fix:** Ensured the base frames in the exported `jump_man.png` faced their default native direction so the engine's internal `flip_x` logic functioned normally.
