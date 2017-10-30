# PushingVoxelsForward
Isosurface extraction using largely undiscovered techniques in C and OpenGL. Check out [https://voxelspace.net/](voxelspace.net)

[Follow the video series on YouTube!](https://www.youtube.com/watch?v=SkI62GxXi2I&list=PLuXQK9Ktey-rcBdZZFi-uxdcrmt-1v6kS)

This project aims to provide massive level of detail in realtime for smooth isosurfaces or voxel surfaces. The idea is to use SnapMC on a tetrahedral hierarchy.

## Building and Running
Only 64-bit building is supported on Windows using Visual Studio 2017, but with a little extra work it can be used to run on Linux and even 32-bit architectures. It uses the following external libraries:
- [cglm](https://github.com/recp/cglm)
- [glew](http://glew.sourceforge.net/)
- [glfw](http://www.glfw.org/)

## Screenshots
Marching Cubes vs SnapMC
![Imgur](https://i.imgur.com/tE2866o.png)
Marching Cubes vs SnapMC on a tetrahedral hierarchy
![Imgur](https://i.imgur.com/aVQ7ukk.png)
SnapMC showing level of detail 18 levels deep
[Imgur](https://i.imgur.com/Pboybbe.png)

## Benchmarks on an FX-8350 (including all resets and OpenGL timings)
**MC**: Z Torus (256^3)
```
Running MC on chunk.
--dim: 255
--indexed: true
--pem: false
-Reset chunk...done (451 ms)
-Label grid...done (125 ms)
-Label edges...done (252 ms)
-Polygonize...done (189 ms)
-Create VAO...done (0 ms)
Complete in 1017 ms. 93416 verts, 186832 prims (0 snapped).
```

**SnapMC**: Z Torus (256^3)
```
Running MC on chunk.
--dim: 255
--indexed: true
--pem: true
-Reset chunk...done (458 ms)
-Label grid...done (171 ms)
-Label edges...Snapping vertices...done (387 ms)
-Polygonize...done (272 ms)
-Create VAO...done (0 ms)
Complete in 1288 ms. 93416 verts, 110808 prims (118112 snapped).
```

## To-Do
- [x] Indexed marching cubes
- [x] SnapMC
- [x] Tetrahedral hierarchy
- [ ] Sharp feature support
- [ ] Multithreaded extraction
- [ ] GPU offloading
- [ ] Realtime modification

## References
- Lorensen, William E., and Harvey E. Cline. "Marching cubes: A high resolution 3D surface construction algorithm." In ACM siggraph computer graphics, vol. 21, no. 4, pp. 163-169. ACM, 1987.
- Raman, Sundaresan, and Rephael Wenger. "Quality isosurface mesh generation using an extended marching cubes lookup table." In Computer Graphics Forum, vol. 27, no. 3, pp. 791-798. Blackwell Publishing Ltd, 2008.
- Gregorski, Benjamin, Mark Duchaineau, Peter Lindstrom, Valerio Pascucci, and Kenneth I. Joy. "Interactive view-dependent rendering of large isosurfaces." In Proceedings of the conference on Visualization'02, pp. 475-484. IEEE Computer Society, 2002.
- Scholz, Manuel, Jan Bender, and Carsten Dachsbacher. "Level of Detail for Real-Time Volumetric Terrain Rendering." In VMV, pp. 211-218. 2013.
- Scholz, Manuel, Jan Bender, and Carsten Dachsbacher. "Real‐Time Isosurface Extraction With View‐Dependent Level of Detail and Applications." In Computer Graphics Forum, vol. 34, no. 1, pp. 103-115. 2015.
