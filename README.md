# <img src="https://user-images.githubusercontent.com/78436416/197327097-b2892935-c019-4148-8776-d2214d7f62c8.png" alt="drawing" width="75"/> ImperialEngine 
Vulkan real-time rendering engine named after my cat Marcus Aurelius



## Notable things implemented (fancy keywords)
* Producer-Consumer multi-threading architecture
* Data Oriented EnTT ECS
* Dear ImGUI
* Bindless Rendering*
* GPU-driven Rendering
* View Frustum Culling
* Async Transfer Queue
* Simple automatic Mesh LOD system based on distance from camera
* Mesh Shading
* Meshlet Backface Cone Culling
* Performance testing and analysis system using Python

## How to Build and Run
* Must have VS22, Vulkan SDK, C++ 20
* Clone repository and target x64 architecture with VS22

# Research Summary (temporary)
For my bachelor's final degree project I compared traditional (CPU-Driven), GPU-Driven and GPU-Driven Mesh shading pipelines.
This research can be split into 3 parts: 
1. Finding the optimal triangle cluster size for Mesh shading.
2. Finding the effects of rendering optimizations that were applied to these pipelines.
3. Comparing the performance of the implemented rendering pipelines with different scenes and scenarios.

## Finding the optimal triangle cluster size for Mesh shading.
NVIDIA recommends triangle cluster size of up to 64 vertices and up to 84 triangles. To find out if this applies to this graphics engine and hardware used - several performance tests were done with different triangle cluster size configurations. From the results shown in the figure below we can see that with the Sponza scene triangle clustes with up to 32 vertices performed about 30% better than other triangle cluster configurations. Other tests done had similar results with meshlets (triangle clusters) with the size of up to 32 vertices and up to 32 triangles performing the best.
<br>
![Sponza average performance-barchart](https://github.com/matasx8/ImperialEngine/assets/78436416/ccf50498-539d-4e79-9bb9-099e4ba07b11)
<br>
The conclusion for this part is that the optimal meshlet size for Imperial Engine is  up to 32 vertices and up to 32 triangles.

## Finding the effects of rendering optimizations
To determine the effects of the applied rendering optimizations (VF Culling, mesh LOD and Meshlet Backface Cone Culling) several performance tests with several different scenes were run. The average results can be seen in the figure below.
<br>
![image](https://github.com/matasx8/ImperialEngine/assets/78436416/ee9ff519-cb0d-4ad7-ae07-a50b001c971c)
<br>
View Frustum culling and mesh LOD had the most positive effects on performance since these optimizations can relatively easily filter out large chunks of geometry from going down the graphics pipeline. Meshlet backface cone culling had the least noticable effects on average. The mesh shading pipeline had only an additional 0.4ms increase in performance. It is important to mention that cone culling has no effect on the traditional and GPU-driven piplines because this optimization is only applied to the mesh shading pipeline.

## Traditional vs GPU-driven vs Mesh shading
One of the many performance tests done to compare these three pipelines was one where each frame a bunch of random objects are added for rendering. Using the data collected from this test a relationship between the number of triangles processed and performance of each rendering pipeline can be found. This is shown in the figure below. The performance of the traditional and GPU-driven pipelines was more or less the same. But it is apparant that it is much more expensive to render triangles with the current mesh shading pipeline. One thing to mention is that the mesh shading pipeline has to process around 20% less triangles due to meshlet cone culling. But it is generally more expensive to render these triangles.
<br>
![image](https://github.com/matasx8/ImperialEngine/assets/78436416/97f5fc16-7084-4677-abac-566955a371a5)
<br>

## Conclusions
1. There are many ways how to optimize or implement a traditional, GPU-driven or mesh shading pipeline. But generally a naive version of these pipelines should have similar results where on average the GPU-driven pipeline has about the same performance as the traditional pipeline. This is because of the pipeline architecture of the graphics pipeline. Culling done on the CPU is much slower than on the GPU, but the time spent on optimizations done on the CPU overlaps with work done on the GPU. One way to avoid penalty of culling not overlapping with rendering on the GPU is using asynchronous compute. Due to a strict deadline an async compute queue was not implemented so the time spent culling on the GPU usually added some overhead ofr the GPU part of the pipeline. It is also likely that adding additional optimizations (like occlusion culling) might further the performance advantages of GPU-driven.
2. Even though the traditional and GPU driven pipelines on average had the same performance, only the GPU-driven pipeline had test cases where it outperformed the other by up to 30%. Another benefit is freeing up CPU resources for other work that before might have caused a bottleneck.
3. It was determined that the optimal meshlet size for this system is up to 32 vertices and up to 32 triangles. But even with using the optimal meshlet configuration, generally the mesh shading pipeline performed on average 10% slower than traditional and regular GPU-driven pipelines. The only noticable benefit was rendering huge meshes (7M triangles and more). In these cases the mesh shading pipeline performed up to 30% better.
