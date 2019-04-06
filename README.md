# Small DirectX Game-Engine

This is a DirectX application with some basic Game engine features.
- Direct3D 12.0 (Feature Level: 11.0)
- Microsoft DirectX Graphics Infrastructure (DXGI) 1.4

## Features
- [x] Use Warp Device by not DirectX supported device
- [x] Toogle fullscreen at runtime (Modify buffers, SwapChain, camera, etc.)
- [x] Draw triangle
- [x] Draw quad
- [x] Draw quads with depth information
- [x] Draw dynamic triangles (triangle, quad, pentagon, hexagon, octagon, ...)
- [x] Draw images
- [ ] Draw text
- [x] Splash/Loading-Screen
- [x] Vertex/Pixel-Shader (Shader Model 5.0)
- [x] V-Sync
- [x] Draw world
	- [x] Load mesh (.obj [Blender], Texture)
		- [x] Optimization of vertices and indices on load
	- [x] Load dynamic objects and meshes for level (Level-File)
	- [x] Load mesh once for all level objects (resource optimization)
- [x] Camera First-Person
	- [x] Free look
	- [x] Movement (W,A,S,D)
	- [x] 85° restriction
	- [x] Quaternionen rotation (Mouse input)
- [x] Update with frame delta
- [x] Collision
	- [x] Axis Aligned Bounding Box (AABB)
	- [x] Object-oriented Bounding Box (OOBB)
	- [x] Two-step collision detection (1. AABB -> 2. OOBB)
 - [ ] Exact static mesh collision
- [x] Frustum
	- [x] Get Frustum from Camera
	- [x] Frustum culling (Draw only objects in frustum)
	- [x] Release mesh when not culled (Reload mesh if culled again)


## Key-Mapping
- W: Move forward
- A: Move left
- S: Move down
- D: Move right
- H: Show help (console)
- I: Show current fps (console)
- F11: Toggle fullscreen
- ESC: Close application

## Miscellaneous
- Small Benchmark on startup

## Screenshots
![DXGE](screenshots/DXGE.png?raw=true "DXGE")

## Renderer
![Renderer](screenshots/Renderer.PNG?raw=true "Renderer")
