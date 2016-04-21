#include "TerrainGenerator.h"
#include "ProceduralTerrainGrid.h"

void UProceduralTerrainGrid::SetVoxel(int32 x, int32 y, int32 z, float fDensity)
{
	TerrainGrid->SetVoxel(x, y, z, fDensity);
}

float UProceduralTerrainGrid::GetVoxel(int32 x, int32 y, int32 z)
{
	return TerrainGrid->GetVoxel(x, y, z);
}

UProceduralTerrainGrid::UProceduralTerrainGrid(const FObjectInitializer& PCIP)
	: Super(PCIP)
{
	TerrainGrid = new UTerrainGrid();
}


void UProceduralTerrainGrid::BeginDestroy()
{
	if (TerrainGrid!=0)
		delete TerrainGrid;
	Super::BeginDestroy();
}