#pragma once
#include "Mat4.h"
#include "Vec4.h"
#include "Vec3.h"
#include <vector>

class Camera
{
	Mat4 view;
	Mat4 projection;

	// light binning
	float zstep;
	float xstep;
	float ystep;

	// organized by z-step
	std::vector<Vec3> cornerPointList;

	bool exponential;

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

public:
	// view frustum
	float farPlane;
	float nearPlane;
	float fov;
	float aspect;
	Vec3 eye;
	Vec3 up;
	Vec3 center;

	// Storing each point that makes up 
	void SubdivideFrustum(std::vector<Vec4> &points, int zSlices, float sliceStep, float xSlices, float ySlices);

	void Camera::ConstructDebugLineList(const std::vector<Vec4> &frustumGrid, std::vector<Vec4> &lineOutput, int xSlices, int ySlices, int zSlices);
}; 