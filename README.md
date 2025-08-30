# Vulkan Experiments
This repository contains a bunch of small projects, experiments, and graphical demos made using the Vulkan API. These are being created as learning exercises. Each experiment will have their own respective READMEs explaining what my goal or intention was with creating these little demos.

Vulkan is a (relatively) new and *incredibly verbose* cross-platform graphics API. If you're new to graphics programming, I would *not* personally recommend starting at Vulkan, unless you're a very patient learner. Even for me, having spent 7 years with OpenGL, just the first experiment with Vulkan has shaken and challenged my understanding of how the GPU works. Much of the code in these early experiments will not strictly be *mine* as a result, since I'm pulling a lot of information out of various sources, like the Vulkan docs and https://vulkan-tutorial.com/.

Here's a list of the completed and in-progress projects I have so far:

**1:** "Hello Triangle" - The "hello world" of Vulkan programs. This program draws a triangle to the screen using nearly 1,000 lines of code. This simple demo is incredibly derivative of the Khronos Vulkan tutorial. (completed)

**1a:** "Hello Triangle" but re-abstracted and "personalized" by me to fit my criteria for future projects and tests. (copmleted)

**1b:** "Hello Triangle" improved - allowing you to resize the window. (completed)

**2:** "Vulkan Framework" - A more complete Vulkan framework than what was in 1a. Includes support for drawing commands using vertex & index buffers, as well as types of uniforms like uniform buffers and 2D image samplers. I wrote my own memory allocator for this.

**2a:** "Vulkan Framework (updated)" - An updated version of experiment 2. This version of the framework will continue to be updated as future versions of the framework release.

**3:** - "Voxel Engine" - A simple voxel engine mirroring the ones I used to create a lot back when I was learning OpenGL (and C++ in general).

It's worth noting that this repository isn't just a source for code, but also my notes and documentation regarding Vulkan and the projects I'm developing. I'm mentioning this because while certain projects (such as the first "Hello Triangle") might be complete code-wise, I may still update the documentation I have for these projects and even the code comments as my understanding of the Vulkan process improves.

Also, as I get better and better with Vulkan, the experiments themselves may pivot towards other topics that are often interlinked with graphics, such as physical simulations or algorithm visualizers.
