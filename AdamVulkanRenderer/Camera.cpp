#include "stdafx.h"
#include "Camera.h"

#define PI (3.141592653589793)

void Camera::SubdivideFrustum(std::vector<Vec4> &pointList, int zSlices, float sliceStep, float xSlices, float ySlices)
{
	for (int i = 1; i <= zSlices; ++i)
	{
		float z = sliceStep * i;
		float halfWidth = z * tan(((fov*aspect) / 2.0f)); // assumes radians
		float halfHeight = z * tan((fov / 2.0f));

		float xinterval = 2.0f * halfWidth / xSlices;
		float yinterval = 2.0f * halfHeight / ySlices;

		for (int j = 0; j < xSlices; ++j)
		{
			for (int k = 0; k < ySlices; ++k)
			{
				float x = -halfWidth + j * xinterval;
				float y = -halfHeight + k * yinterval;
				Vec4 frustumPoint = Vec4(x, y, -z, 1);
				pointList.push_back(frustumPoint);
			}
		}
	}
}

// This function is a little overkill because it creates a line segment between each mini-frusta.
// Could get away with just a cross section of the entire frustum along the different axes.
// However, this allows me to possibly highlight individual mini-frusta in the future.
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
		for (int y = 0; y < ySlices; ++y)
		{
			lineOutput.push_back(frustumGrid[z * xSlices * ySlices + y]);
			int x = 1;
			for (; x < xSlices - 1; ++x)
			{
				lineOutput.push_back(frustumGrid[z * xSlices * ySlices + y + xSlices * x]);
				lineOutput.push_back(frustumGrid[z * xSlices * ySlices + y + xSlices * x]);
			}
			lineOutput.push_back(frustumGrid[z * xSlices * ySlices + y + xSlices * x]);
		}
	}

	for (int z = 0; z < zSlices-1; ++z)
	{
		for (int y = 0; y < ySlices; ++y)
		{
			lineOutput.push_back(frustumGrid[z * xSlices * ySlices + y]);
			int x = 1;
			for (; x < xSlices - 1; ++x)
			{
				lineOutput.push_back(frustumGrid[z * xSlices * ySlices + y + xSlices * x]);
				lineOutput.push_back(frustumGrid[z * xSlices * ySlices + y + xSlices * x]);
			}
			lineOutput.push_back(frustumGrid[z * xSlices * ySlices + y + xSlices * x]);
		}
	}

	for (int x = 0; x < xSlices; ++x)
	{
		for (int y = 0; y < ySlices; ++y)
		{
			lineOutput.push_back(Vec4(0, 0, 0, 1));
			int z = 0;
			for (; z < zSlices - 1; ++z)
			{
				lineOutput.push_back(frustumGrid[(z * xSlices * ySlices) + x * xSlices + y]);
				lineOutput.push_back(frustumGrid[(z * xSlices * ySlices) + x * xSlices + y]);
			}
			lineOutput.push_back(frustumGrid[(z * xSlices * ySlices) + x * xSlices + y]);
		}
	}
}