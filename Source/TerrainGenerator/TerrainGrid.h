#pragma once

struct FVoxel
{
	float Voxel;
	FVoxel(float fvoxel)
	{
		Voxel = fvoxel;
	}
};


class TERRAINGENERATOR_API UTerrainGrid
{
	FCriticalSection ReadWriteLock;
public:
	UTerrainGrid();

	float DefaultIsoValue;

	TMap<FIntVector, FVoxel*> TerrainGrid;
	
	float GetVoxel(int32 x, int32 y, int32 z);
	void SetVoxel(int32 x, int32 y, int32 z, float fValue);

};
