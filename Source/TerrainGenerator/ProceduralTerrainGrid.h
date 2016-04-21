#pragma once

#include "TerrainGrid.h"
#include "ProceduralTerrainGrid.generated.h"

/**
* Utility class for Editing The Terrain Grid from blueprints
*/

UCLASS()
class TERRAINGENERATOR_API UProceduralTerrainGrid : public UObject
{
	GENERATED_BODY()
public:
	UTerrainGrid *TerrainGrid;

	UProceduralTerrainGrid(const FObjectInitializer& PCIP);

	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	void SetVoxel(int32 x, int32 y, int32 z, float fDensity);

	UFUNCTION(BlueprintCallable, Category = "Procedural Terrain")
	float GetVoxel(int32 x, int32 y, int32 z);

	virtual void BeginDestroy() override;

	UTerrainGrid *GetDataProvider()
	{
		return TerrainGrid;
	}
};