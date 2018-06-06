#include "stdafx.h"
#include <math.h>
#include "Camera.h"

#define PI (3.141592653589793)

void Camera::SubdivideFrustum(std::vector<Vec4> &pointList, int zSlices, float sliceStep, float xSlices, float ySlices)
{
	for (int i = 1; i <= zSlices; ++i)
	{
		float z = sliceStep * i;
		float halfWidth = z * tan(((fov*aspect) / 2.0f)); // assumes radians
		float halfHeight = z * tan((fov / 2.0f));

		float xstep = 2.0f * halfWidth / xSlices;
		float ystep = 2.0f * halfHeight / ySlices;

		for (int j = 0; j < xSlices; ++j)
		{
			for (int k = 0; k < ySlices; ++k)
			{
				float x = -halfWidth + j * xstep;
				float y = -halfHeight + k * ystep;
				Vec4 frustumPoint = Vec4(x, y, -z, 1);
				pointList.push_back(frustumPoint);
			}
		}
	}
}

void Camera::ConstructDebugLineList(const std::vector<Vec4> &frustumGrid, std::vector<Vec4> &lineOutput, int xSlices, int ySlices, int zSlices)
{
	for (int z = 0; z < zSlices; ++z)
	{
		for (int x = 0; x < xSlices; ++x)
		{
			lineOutput.push_back(frustumGrid[z * xSlices * ySlices + x * xSlices]);
			int y = 1;
			for (; y < ySlices-1; ++y)
			{
				lineOutput.push_back(frustumGrid[z * xSlices * ySlices + x * xSlices + y]);
				lineOutput.push_back(frustumGrid[z * xSlices * ySlices + x * xSlices + y]);
			}
			lineOutput.push_back(frustumGrid[z * xSlices * ySlices + x * xSlices + y]);
		}
	}

	for (int z = 0; z < zSlices; ++z)
	{
		int pointsProcessed = 0;
		for (int x = 0; x < xSlices; ++x)
		{
			lineOutput.push_back(frustumGrid[z * xSlices * ySlices + x * xSlices]);
			++pointsProcessed;
			int y = 1;
			for (; y < ySlices - 1; ++y)
			{
				lineOutput.push_back(frustumGrid[z * xSlices * ySlices + x * xSlices + y]);
				lineOutput.push_back(frustumGrid[z * xSlices * ySlices + x * xSlices + y]);
				++pointsProcessed;
			}
			lineOutput.push_back(frustumGrid[z * xSlices * ySlices + x * xSlices + y]);
			++pointsProcessed;
		}
	}
}