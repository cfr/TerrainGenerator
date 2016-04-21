// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ProceduralTerrainChunk.h"
#include "ProceduralTerrain.generated.h"

class FProceduralTerrainWorker;
class UTerrainGrid;

/**
 * Base class for procedural terrains
 */
UCLASS(BlueprintType, Blueprintable)
class TERRAINGENERATOR_API AProceduralTerrain : public AActor
{
	GENERATED_BODY()
public:
	
	// Worker Thread for Terrain Marching
	FProceduralTerrainWorker *ProceduralTerrainWorker;

	// Default Scene Root (Chunks will be attached to this component)
	class USceneComponent* SceneRoot;

	// Stores the Terrain Chunks
	TMap<FIntVector, class UProceduralTerrainChunk*> ProceduralTerrainChunks;

	UTerrainGrid* TerrainGrid;
public:

	// Constructor
	AProceduralTerrain(const FObjectInitializer& PCIP);

	/*
	*	PROPERTIES
	*/

	// Material used to apply to chunks
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Procedural Terrain")
	UMaterialInterface *TerrainMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Procedural Terrain")
	float SurfaceCrossOverValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Procedural Terrain")
	FVector ChunkScale;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Procedural Terrain")
	FVector ChunkSize;

	/*
	*	FUNCTIONS
	*/
	// Methods to manage Chunks
	
	// Adds a new chunk to the Terrain. returns true if the addition was successful
	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	bool CreateChunk(int32 x, int32 y, int32 z);

	// Updates a chunk in the Terrain. Returns true if the update was successful. (Regenerates triangles from Voxel data)
	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	bool UpdateChunk(int32 x, int32 y, int32 z);

	// Deletes a chunk in the Terrain. returns true if the removal was successful
	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	bool DestroyChunk(int32 x, int32 y, int32 z);
	
	// Returns a chunk from the Terrain, Returns 0 (NULL) if the chunk was not found in the TMap
	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Procedural Terrain")
	UProceduralTerrainChunk *GetChunk(int32 x, int32 y, int32 z);
	
	// Updates The density for a given voxel. (Can create new Chunks if needed)
	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	bool SetVoxel(int32 x, int32 y, int32 z, float NewDensity, bool CreateIfNotExists);

	// Retrieves the density for a given voxel.
	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	float GetVoxel(int32 x, int32 y, int32 z);

	// Checks for changes in the chunks and updates them one by one. returns true if the terrain was updated
	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	bool UpdateTerrain();

	void DestoryWorkerThread();

	virtual void Tick(float fDeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	void SetDefaultIsoValue(float NewValue);
	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	float GetDefaultIsoValue();
	
	//UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	//void OnChunkGenerated() // Todo make an implementable event when a chunk is created
	// Required to kill the ProceduralTerrainnWorker when the game / PIE ends
	virtual void BeginDestroy() override;
private:
	UProceduralTerrainChunk *CreateProceduralTerrainChunk();
};
