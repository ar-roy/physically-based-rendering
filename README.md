# Physically Based Rendering (PBR)

## Motivation
PBR, also known as physically-based rendering, is a collection of techniques that are based on the physical world. It aims to mimic light in a physically plausible way. The foundation of PBR is the theory of microfacets, which describes any surface as tiny little perfectly reflective mirrors in at a microscopic scale.

## Deliverable
Demonstrate an interactive application capable of rendering quality objects with proper lighting and textures. The basic interactive features would include the customization of parameters and the moving of visualized objects. One potential bonus feature would be to load a predefined model with embedded textures and to visualize it with PBR. Another potential bonus feature would be to render the objects in a certain environment using image-based lighting.

## Checkpoints

### PBR Lighting
* Non-interactive application for rendering of simple object (sphere) with PBR lighting
* PBR lighting implementation to be done on the Fragment Shader code
* Up to 12 light sources
* The material property will be uniform for each object

### PBR Lighting + Texturing
* Non-interactive application for rendering of simple object(s) (sphere) with PBR lighting and texturing
* Addition of texturing to allow the object(s) to have non-uniform reflectance

### Interactive Functionalities
* Object can be rotated around the origin point
* Users can select different textures
* Users can adjust properties of textures
* Users can adjust the shape of the object
* Users can adjust properties of the light source
* Movable camera 
