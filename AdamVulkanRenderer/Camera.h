#pragma once
#include "Mat4.h"
#include "Vec3.h"
#include <vector>

class Camera
{
	Mat4 view;
	Mat4 projection;

	float fov;

	// view frustum
	float far;
	float near;
	float width;
	float height;
	float fov;

	// light binning
	float zstep;
	float xstep;
	float ystep;

	// organized by z-step
	std::vector<Vec3> cornerPointList;

	bool exponential;

	// Storing each point that makes up 
	void SubdivideFrustum(std::vector<Vec3> &points) {}

	// x and y step setup a slice
	// z step determines number of slices
	// say we divide 5 x steps and 5 y steps
	// we then have 25 points per slice 
	// v v v v v
	// v v v v v
	// v v v v v
	// v v v v v
	// v v v v v
	// This is first slice of x,y plane enclosed in top left, bottom right, bottom left, and top right of frustum
	// Frustum starts at 0,0,0 for all points, so don't need an entry for that
}; 