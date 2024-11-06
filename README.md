# Gems Path Tracer

## Introduction

This project is a by-product of reading the famous open-source real-time ray tracing textbook, [Ray Tracing Gems II](https://www.realtimerendering.com/raytracinggems/rtg2/index.html).
While reading, I implemented the features & algorithms of a selection of chapters. They will be updated in this README, which should contain
mostly screenshots. Detailed explanations and mathmatical derivations can be found in the book.

## Base Project

I didn't implement everything from scratch. Instead, I started my work off from a 600-stared project called [ChameleonRT](https://github.com/Twinklebear/ChameleonRT/) by Will Usher (Github: Twinklebear).
This base project has already provided almost all raytracing infrastructures, which includes
a scene parser + constructor, a Disney BSDF that unifies all common input materials, multiple importance sampling, and most importantly,
multiple raytracing backends (DXR, Vulkan, Embree, OptiX, OSPRay, Metal).

A side focus of my self-study is to familiarize myself with DXR and Vulkan, especially their memory management.
Because the base project didn't unify the shader files (which means a "lights.hlsl" and a "lights.glsl" are required by DXR
and Vulkan backends respectively), I only roll out the implementation in these 2 backends.

Hats off to **ChameleonRT**

## Different Cameras

This section comes from Chapter 3 of the book, Essential Ray Generation Shaders. Camera types being implemented
are: pinhole, thin lens, Panini, fish eye, orthographic. The scene is the Sponza Palace.

First, orthographic is very different. For different pixels, camera rays have the same direction but different origins,
which is the exact opposite of most other camera types in a graphics system.

Below is the comparison against a pinhole ray. The pinhole camera has a fov of 90 degree, and the orthographic has a
fov distance of 6m, in world space.

<img src="screenshots/orthographic_compare.png" alt="orthographic_compare" width="1920"/>

The below two 2x2 comparisons demonstrate the feature differences of the rest 4 types of camera in 60 and 90 degree fov, respectively.
Top row is pinhole, thin lens; bottom row is Panini, fish eyes.

<img src="screenshots/fov60compare.png" alt="fov60compare" width="1920"/>

<img src="screenshots/fov90compare.png" alt="fov90compare" width="1920"/>

Thin Lens camera simulates the focus blur, and we can see the pixels around
the black door are blurry.

Generalized Panini camera gives a better looking for wide fov by decreasing
the disproportion between the center region and the left/right region, which
is really obvious in a pinhole camera with same fov.

Unlike a pinhole camera which equalizes distance between pixels, a fisheye camera
equalizes angular distance. As a result, vertical lines are bented outwards, and
this is called barrel distortion.