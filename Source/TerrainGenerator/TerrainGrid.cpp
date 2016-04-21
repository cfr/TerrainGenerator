#include "TerrainGenerator.h"
#include "TerrainGrid.h"


UTerrainGrid::UTerrainGrid()
{
}

float UTerrainGrid::GetVoxel(int32 x, int32 y, int32 z)
{
	ReadWriteLock.Lock();
	if (TerrainGrid.Contains(FIntVector(x, y, z)))
	{
		FVoxel **Voxel = TerrainGrid.Find(FIntVector(x, y, z));
		ReadWriteLock.Unlock();
		return (*Voxel)->Voxel;
	};
	ReadWriteLock.Unlock();
	return DefaultIsoValue;
}

void UTerrainGrid::SetVoxel(int32 x, int32 y, int32 z, float fValue)
{
	ReadWriteLock.Lock();
	FVoxel **Voxel = TerrainGrid.Find(FIntVector(x, y, z));
	if (Voxel != NULL)
	{
		delete (*Voxel);
	}
	
	TerrainGrid.Add(FIntVector(x, y, z), new FVoxel(fValue));
	ReadWriteLock.Unlock();
}