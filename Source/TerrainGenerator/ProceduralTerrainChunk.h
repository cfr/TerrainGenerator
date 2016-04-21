// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProceduralMeshComponent.h"
#include "ProceduralTerrainChunk.generated.h"

class AProceduralTerrain;
class UTerrainGrid;

UCLASS(hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), Blueprintable)
class TERRAINGENERATOR_API UProceduralTerrainChunk : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:
	UProceduralTerrainChunk(const FObjectInitializer &ObjectInitializer);

	FIntVector ChunkLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Procedural Terrain")
	bool IsUpdating;

	UPROPERTY(BlueprintReadOnly, Category = "Procedural Terrain")
	bool HasChanges;

	UPROPERTY(BlueprintReadOnly, Category = "Procedural Terrain")
	bool HasCollisoon;

	UPROPERTY(BlueprintReadWrite, Category = "Procedural Terrain")
	FTransform TerrainTransform;

	UPROPERTY(BlueprintReadWrite, Category = "Procedural Terrain")
	AProceduralTerrain *Terrain;

	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	void RegenerateVoxelData();

	virtual void BeginDestroy() override;
private:


};