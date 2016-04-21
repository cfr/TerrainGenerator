
#include "TerrainGenerator.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralTerrain.h"
#include "ProceduralTerrainChunk.h"
#include "Runtime/Launch/Resources/Version.h"

//////////////////////////////////////////////////////////////////////////
//		Procedural Terrain Chunk Class
//////////////////////////////////////////////////////////////////////////

UProceduralTerrainChunk::UProceduralTerrainChunk(const FObjectInitializer& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = false;
	
	HasCollisoon = true;
	IsUpdating = false;
	HasChanges = false;
}

void UProceduralTerrainChunk::RegenerateVoxelData()
{
	Terrain->UpdateChunk(ChunkLocation.X, ChunkLocation.Y, ChunkLocation.Z);
}

void UProceduralTerrainChunk::BeginDestroy()
{
	Super::BeginDestroy();
}