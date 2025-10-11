# Introduction to Real-Time Computer Graphics Labs

- **Author**: Artur Pelcharskyi
- **OS**: Windows 11
- **API**: DirectX11
- **Total Late Days**: 3

## Homework 1: A Funny Cube
- **Late Days**: 2
- **Demo**: [YouTube Video](https://www.youtube.com/watch?v=ZIw-_7JyE0U)

### Main tasks:
- Created a project window
- Implemented an Euler camera controlled via mouse and keyboard
- Handcrafted a cube
- Implemented two different pixel shaders

### Aditional tasks:
- Integrated complex shaders from ShaderToy: [Shader 1](https://www.shadertoy.com/view/Xd23Dh) and [Shader 2](https://www.shadertoy.com/view/lsl3RH)
- Added an FPS counter
- Implemented functionality to load textures from image files


## Homework 2: Instancing and Models
- **Late Days**: 1
- **Demo**: [YouTube Video](https://youtu.be/AWov3wH2uvc?si=aDZYS79XognPZDGZ)

### Main tasks:
- Implemented a **3D model loader** using **Assimp**
- Loaded and rendered a **textured 3D model** in the scene
- Implemented **instanced rendering** to draw **10000 textured cubes** distributed across the 3D world
- Added a large **ground plane** with a tileable texture
- Conducted **performance tests** comparing frame times for **instanced vs non-instanced** cube rendering and plotted the results

### Aditional tasks:
- Implemented a Mesh System to support instanced rendering for any imported 3D model (handling multiple meshes per model)
- Added a **separate transformation matrix** for objects, allowing them to move or rotate independently of the camera (real-time spinning/moving animation)
- Integrated **ImGui** to provide an interface for **moving and rotating** individual objects in the scene

### Instancing vs no instancing