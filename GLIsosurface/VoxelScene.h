#pragma once

#include <gl/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <stdio.h>

struct VoxelScene
{

};

void VoxelScene_init(struct VoxelScene* out);
void VoxelScene_run();
