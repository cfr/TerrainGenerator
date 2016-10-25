// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainGenerator.h"
#include "ProceduralTerrain.h"
#include "ProceduralTerrainChunk.h"
#include "ProceduralTerrainWorker.h"

#include "TerrainGrid.h"

AProceduralTerrain::AProceduralTerrain(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	ChunkScale(100.0f, 100.0f, 100.0f),
	ChunkSize(32, 32, 32)
{
	// Create Root Component
	SceneRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, "SceneRoot");
	RootComponent = SceneRoot;

	PrimaryActorTick.bCanEverTick = true; // Enable ticking to this component
	
	// Initialize default value for properties
	TerrainMaterial = NULL;

	// Spawn the Worker thread
	ProceduralTerrainWorker = new FProceduralTerrainWorker();
	ProceduralTerrainWorker->Start();

	TerrainGrid = new UTerrainGrid();
	SurfaceCrossOverValue = 0.0f;
	SetDefaultIsoValue(0.0f);
}

void AProceduralTerrain::DestoryWorkerThread()
{
	if (!ProceduralTerrainWorker)
		return;

	// Destroy the Worker Thread
	ProceduralTerrainWorker->EnsureCompletion();
	ProceduralTerrainWorker->Shutdown();
}

void AProceduralTerrain::SetDefaultIsoValue(float NewValue)
{
	TerrainGrid->DefaultIsoValue = NewValue;
}

float AProceduralTerrain::GetDefaultIsoValue()
{
	return TerrainGrid->DefaultIsoValue;
}

void AProceduralTerrain::BeginDestroy()
{
	DestoryWorkerThread();


	delete ProceduralTerrainWorker;
	ProceduralTerrainWorker = 0;

	Super::BeginDestroy();
}


bool AProceduralTerrain::CreateChunk(int32 x, int32 y, int32 z)
{
	// Make sure we don't create duplicated chunks
	UProceduralTerrainChunk **FoundChunk = ProceduralTerrainChunks.Find(FIntVector(x, y, z));
	if (FoundChunk != 0)
	{
		// Don't create chunk;
		return false;
	}

	// Create a new Terrain Chunk
	UProceduralTerrainChunk *TerrainChunk = CreateProceduralTerrainChunk();
	TerrainChunk->ChunkLocation = FIntVector(x, y, z);
//	TerrainChunk->SetWorldLocation(NewTansform.GetLocation());
	// Mark TerrainChunk As Should be updated
	ProceduralTerrainChunks.Add(FIntVector(x, y, z), TerrainChunk);

	UpdateChunk(x, y, z);
	

	return true;
}

bool AProceduralTerrain::UpdateChunk(int32 x, int32 y, int32 z)
{
	UProceduralTerrainChunk **FoundChunk = ProceduralTerrainChunks.Find(FIntVector(x, y, z));
	if (!FoundChunk)
	{
		return false;
	}

	UProceduralTerrainChunk *TerrainChunk = *FoundChunk;
	if (TerrainChunk->IsUpdating)
		return false;

	TerrainChunk->IsUpdating = true; // Because we going to work with this in a different thread
	TerrainChunk->HasChanges = false; // So we know if we have to update the chunk or not

	FTerrainWorkerTask WorkerTask;
	WorkerTask.ChunkIdentifier = TerrainChunk->ChunkLocation;
	WorkerTask.TerrainGrid = this->TerrainGrid;
    WorkerTask.ChunkLocation = FVector::ZeroVector;//FVector(TerrainChunk->ChunkLocation);

	WorkerTask.SurfaceCrossValue = SurfaceCrossOverValue;
	WorkerTask.ChunkScale = ChunkScale;

	WorkerTask.ChunkStartSize = FIntVector(x * ChunkSize.X, y * ChunkSize.Y, z * ChunkSize.Z);
	WorkerTask.ChunkEndSize = FIntVector((x + 1) * ChunkSize.X, (y + 1) * ChunkSize.Y, (z + 1) * ChunkSize.Z);
	
	ProceduralTerrainWorker->QueuedTasks.Enqueue(WorkerTask);
	return true;
}

bool AProceduralTerrain::DestroyChunk(int32 x, int32 y, int32 z)
{
	UProceduralTerrainChunk **FoundChunk = ProceduralTerrainChunks.Find(FIntVector(x, y, z));
	if (!FoundChunk)
	{
		return false;
	}

	UProceduralTerrainChunk *TerrainChunk = *FoundChunk;
	TerrainChunk->UnregisterComponent();
	TerrainChunk->DestroyComponent();
	ProceduralTerrainChunks.Remove(FIntVector(x, y, z));

	return true;
}

UProceduralTerrainChunk * AProceduralTerrain::GetChunk(int32 x, int32 y, int32 z)
{
	UProceduralTerrainChunk **FoundChunk = ProceduralTerrainChunks.Find(FIntVector(x, y, z));
	if (!FoundChunk)
	{
		return 0;
	}
	return *FoundChunk;
}

UProceduralTerrainChunk * AProceduralTerrain::CreateProceduralTerrainChunk()
{
	// Generate different names for our component to suppress warnings (May not be prefect :P)
	FString ComponentName;
	int32 ID = ProceduralTerrainChunks.Num();
	ComponentName.Append(TEXT("TerrainMeshComponent"));
	ComponentName.AppendInt(ID);
	FName name;
	name.AppendString(ComponentName);

	// Create our TerrainMeshComponent 
	UProceduralTerrainChunk *TerrainChunk = NewObject<UProceduralTerrainChunk>(this, name);
	TerrainChunk->RegisterComponent();

	// Apply a material if we have any
	if (!TerrainMaterial)
		TerrainMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	TerrainChunk->SetMaterial(0, TerrainMaterial);

	TerrainChunk->Terrain = this;

	return TerrainChunk;
}

bool AProceduralTerrain::UpdateTerrain()
{
	bool ChangesHappened = false;
	for (auto ChunkIt = ProceduralTerrainChunks.CreateIterator(); ChunkIt; ++ChunkIt)
	{
		FIntVector ChunkLocation = ChunkIt.Key();

		if (ChunkIt.Value()->HasChanges)
		{
			UpdateChunk(ChunkLocation.X, ChunkLocation.Y, ChunkLocation.Z);


			ChangesHappened = true;
		}
	}
	return ChangesHappened;
}

bool AProceduralTerrain::SetVoxel(int32 x, int32 y, int32 z, float NewDensity, bool CreateIfNotExists)
{
	TerrainGrid->SetVoxel(x, y, z, NewDensity);

	// Find the Chunk Location
	
	int32 ChunkX = FMath::FloorToInt((float)x / ChunkSize.X);
	int32 ChunkY = FMath::FloorToInt((float)y / ChunkSize.Y);
	int32 ChunkZ = FMath::FloorToInt((float)z / ChunkSize.Z);
	
	if (FMath::Fmod(x, ChunkSize.X) == 0.0f)
	{
		UpdateChunk(ChunkX - 1, ChunkY, ChunkZ);
		UpdateChunk(ChunkX + 1, ChunkY, ChunkZ);
	}

	if (FMath::Fmod(y, ChunkSize.Y) == 0.0f)
	{
		UpdateChunk(ChunkX, ChunkY - 1 , ChunkZ);
		UpdateChunk(ChunkX, ChunkY + 1, ChunkZ);
	}

	if (FMath::Fmod(z, ChunkSize.Z) == 0.0f)
	{
		UpdateChunk(ChunkX, ChunkY, ChunkZ + 1);
		UpdateChunk(ChunkX , ChunkY, ChunkZ - 1);
	}
	// Create if its not exists
	UProceduralTerrainChunk *TerrainChunk = GetChunk(ChunkX, ChunkY, ChunkZ);
	if (!TerrainChunk)
	{
		if (CreateIfNotExists)
		{
			CreateChunk(ChunkX, ChunkY, ChunkZ);
		}

	/*	for (int32 x = -1; x < 1; ++x)
		{
			for (int32 y = -1; y < 1; ++y)
			{
				for (int32 z = -1; z < 1; ++z)
				{
					UpdateChunk(ChunkX + x, ChunkY + y, ChunkZ + z);
				}
			}
		}*/
		return false;
	}

	// Update The voxel inside the chunk if it exists
	TerrainChunk->HasChanges = true;

	// Ask for update
	TerrainChunk->RegenerateVoxelData();
	
	return true;
}

float AProceduralTerrain::GetVoxel(int32 x, int32 y, int32 z)
{
	return TerrainGrid->GetVoxel(x, y, z);
}


void AProceduralTerrain::Tick(float fDeltaTime)
{
	Super::Tick(fDeltaTime);
	if (!ProceduralTerrainWorker)
	{
		return;
	}

	if (ProceduralTerrainWorker->FinishgedTasks.IsEmpty())
		return;

	FTerrainWorkerTask FinishedTask;

	if (ProceduralTerrainWorker->FinishgedTasks.Dequeue(FinishedTask))
	{
		UProceduralTerrainChunk **FoundChunk = ProceduralTerrainChunks.Find(FinishedTask.ChunkIdentifier);
		if (!FoundChunk)
		{
			return;
		}

		UProceduralTerrainChunk *TerrainChunk = *FoundChunk;
		TArray<FVector2D> UVs;
		TArray<FColor> VertexColors;
		TerrainChunk->ClearAllMeshSections();
		TerrainChunk->CreateMeshSection(0, FinishedTask.Vertices, FinishedTask.Indices, FinishedTask.Normals, FinishedTask.UVs, FinishedTask.VertexColors, FinishedTask.Tangents, true);
		TerrainChunk->IsUpdating = false;
	}

}
