#include "TerrainGenerator.h"
#include "ProceduralTerrainWorker.h"
#include "TerrainGrid.h"
#include "MarchingCubesLookupTables.h"
#include "ProceduralMeshComponent.h"

int32 FProceduralTerrainWorker::ThreadCount = 0;

FProceduralTerrainWorker::FProceduralTerrainWorker()
	: StopTaskCounter(0),
	Thread(0),
	bIsRunning(false)
{

}

bool FProceduralTerrainWorker::Start()
{
	if (FPlatformProcess::SupportsMultithreading())
	{
		Thread = FRunnableThread::Create(this, TEXT("ProceduralTerrainWorker"), 0, TPri_AboveNormal); //windows default = 8mb for thread, could specify more
		return true;
	}
	else{
		return false;
	}
}

bool FProceduralTerrainWorker::Init()
{
	++FProceduralTerrainWorker::ThreadCount;

	return true;
}

uint32 FProceduralTerrainWorker::Run()
{
	UE_LOG(LogTemp, Warning, TEXT("Terrain Worker Starting Up!"));
	bIsRunning = true;
	while (StopTaskCounter.GetValue() == 0)
	{
		if (!QueuedTasks.IsEmpty())
		{
			FTerrainWorkerTask WorkerTask;
			QueuedTasks.Dequeue(WorkerTask);

			if (WorkOnTask(&WorkerTask))
			{
				FinishgedTasks.Enqueue(WorkerTask);
			}
			
			
		}
		FPlatformProcess::Sleep(0.03);
	}
	bIsRunning = false;
	UE_LOG(LogTemp, Warning, TEXT("Terrain Worker Exiting!"));
	return 0;
}

void FProceduralTerrainWorker::Exit()
{
	--FProceduralTerrainWorker::ThreadCount;
}
 
void FProceduralTerrainWorker::EnsureCompletion()
{
	if (!Thread)
		return;

	Stop();

	Thread->WaitForCompletion();
	while (bIsRunning)
	{
		FPlatformProcess::Sleep(1.00);
	}
}

void FProceduralTerrainWorker::Stop()
{
	if (!Thread)
		return;

	StopTaskCounter.Increment();
}




void FProceduralTerrainWorker::Shutdown()
{
	if (!Thread)
		return;

	Thread->Kill(true);
}
 


FProceduralTerrainWorker::~FProceduralTerrainWorker()
{
	delete Thread;
	Thread = NULL;
}


bool FProceduralTerrainWorker::WorkOnTask(FTerrainWorkerTask *WorkerTask)
{
	PolygonizeToTriangles(
		WorkerTask->TerrainGrid,
		WorkerTask->SurfaceCrossValue,
		WorkerTask->ChunkStartSize,
		WorkerTask->ChunkEndSize,
		WorkerTask->ChunkLocation,
		WorkerTask->ChunkScale,
		WorkerTask->TerrainTransform,

		WorkerTask->Vertices,
		WorkerTask->Indices,
		WorkerTask->Normals,
		WorkerTask->UVs,
		WorkerTask->VertexColors,
		WorkerTask->Tangents
	);

	return true;
}

int32 FProceduralTerrainWorker::PolygonizeToTriangles(
	UTerrainGrid *TerrainGrid,
	float p_fSurfaceCrossValue,
	FIntVector ChunkStartSize,
	FIntVector ChunkEndSize,
	FVector &ChunkLocation,
	FVector &ChunkScale,
	FTransform &TerrainTransform,

	TArray<FVector> &Vertices,
	TArray<int32> &Indices,
	TArray<FVector> &Normals,
	TArray<FVector2D> &UVs,
	TArray<FColor> &VertexColors,
	TArray<FProcMeshTangent> &Tangents
	)
{
	// TODO?
	float PosX = ChunkLocation.X;
	float PosY = ChunkLocation.Y;
	float PosZ = ChunkLocation.Z;
	TArray<FVector> Positions;

	int NumTriangles = 0;
	for (int32 x = ChunkStartSize.X; x < ChunkEndSize.X; ++x)
	{
		for (int32 y = ChunkStartSize.Y; y < ChunkEndSize.Y; ++y)
		{
			for (int32 z = ChunkStartSize.Z; z < ChunkEndSize.Z; ++z)
			{

				// Get each points of a cube.
				float p0 = TerrainGrid->GetVoxel(x, y, z);
				float p1 = TerrainGrid->GetVoxel(x + 1, y, z);
				float p2 = TerrainGrid->GetVoxel(x, y + 1, z);
				float p3 = TerrainGrid->GetVoxel(x + 1, y + 1, z);
				float p4 = TerrainGrid->GetVoxel(x, y, z + 1);
				float p5 = TerrainGrid->GetVoxel(x + 1, y, z + 1);
				float p6 = TerrainGrid->GetVoxel(x, y + 1, z + 1);
				float p7 = TerrainGrid->GetVoxel(x + 1, y + 1, z + 1);

				/*
				Determine the index into the edge table which
				tells us which vertices are inside of the surface
				*/
				int crossBitMap = 0;

				if (p0 < p_fSurfaceCrossValue) crossBitMap |= 1;
				if (p1 < p_fSurfaceCrossValue) crossBitMap |= 2;

				if (p2 < p_fSurfaceCrossValue) crossBitMap |= 8;
				if (p3 < p_fSurfaceCrossValue) crossBitMap |= 4;

				if (p4 < p_fSurfaceCrossValue) crossBitMap |= 16;
				if (p5 < p_fSurfaceCrossValue) crossBitMap |= 32;

				if (p6 < p_fSurfaceCrossValue) crossBitMap |= 128;
				if (p7 < p_fSurfaceCrossValue) crossBitMap |= 64;


				/* Cube is entirely in/out of the surface */
				int edgeBits = edgeTable[crossBitMap];
				if (edgeBits == 0)
					continue;

				float interpolatedCrossingPoint = 0.0f;
				FVector interpolatedValues[12];

				if ((edgeBits & 1) > 0)
				{

					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p0) / (p1 - p0);
					interpolatedValues[0] = FMath::Lerp(FVector(PosX + x, PosY + y, PosZ + z), FVector(PosX + x + 1, PosY + y, PosZ + z), interpolatedCrossingPoint);
				}
				if ((edgeBits & 2) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p1) / (p3 - p1);
					interpolatedValues[1] = FMath::Lerp(FVector(PosX + x + 1, PosY + y, PosZ + z), FVector(PosX + x + 1, PosY + y + 1, PosZ + z), interpolatedCrossingPoint);
				}
				if ((edgeBits & 4) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p2) / (p3 - p2);
					interpolatedValues[2] = FMath::Lerp(FVector(PosX + x, PosY + y + 1, PosZ + z), FVector(PosX + x + 1, PosY + y + 1, PosZ + z), interpolatedCrossingPoint);
				}
				if ((edgeBits & 8) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p0) / (p2 - p0);
					interpolatedValues[3] = FMath::Lerp(FVector(PosX + x, PosY + y, PosZ + z), FVector(PosX + x, PosY + y + 1, PosZ + z), interpolatedCrossingPoint);
				}

				//Top four edges
				if ((edgeBits & 16) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p4) / (p5 - p4);
					interpolatedValues[4] = FMath::Lerp(FVector(PosX + x, PosY + y, PosZ + z + 1), FVector(PosX + x + 1, PosY + y, PosZ + z + 1), interpolatedCrossingPoint);
				}
				if ((edgeBits & 32) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p5) / (p7 - p5);
					interpolatedValues[5] = FMath::Lerp(FVector(PosX + x + 1, PosY + y, PosZ + z + 1), FVector(PosX + x + 1, PosY + y + 1, PosZ + z + 1), interpolatedCrossingPoint);
				}
				if ((edgeBits & 64) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p6) / (p7 - p6);
					interpolatedValues[6] = FMath::Lerp(FVector(PosX + x, PosY + y + 1, PosZ + z + 1), FVector(PosX + x + 1, PosY + y + 1, PosZ + z + 1), interpolatedCrossingPoint);
				}
				if ((edgeBits & 128) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p4) / (p6 - p4);
					interpolatedValues[7] = FMath::Lerp(FVector(PosX + x, PosY + y, PosZ + z + 1), FVector(PosX + x, PosY + y + 1, PosZ + z + 1), interpolatedCrossingPoint);
				}

				//Side four edges
				if ((edgeBits & 256) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p0) / (p4 - p0);
					interpolatedValues[8] = FMath::Lerp(FVector(PosX + x, PosY + y, PosZ + z), FVector(PosX + x, PosY + y, PosZ + z + 1), interpolatedCrossingPoint);
				}
				if ((edgeBits & 512) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p1) / (p5 - p1);
					interpolatedValues[9] = FMath::Lerp(FVector(PosX + x + 1, PosY + y, PosZ + z), FVector(PosX + x + 1, PosY + y, PosZ + z + 1), interpolatedCrossingPoint);
				}
				if ((edgeBits & 1024) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p3) / (p7 - p3);
					interpolatedValues[10] = FMath::Lerp(FVector(PosX + x + 1, PosY + y + 1, PosZ + z), FVector(PosX + x + 1, PosY + y + 1, PosZ + z + 1), interpolatedCrossingPoint);
				}
				if ((edgeBits & 2048) > 0)
				{
					interpolatedCrossingPoint = (p_fSurfaceCrossValue - p2) / (p6 - p2);
					interpolatedValues[11] = FMath::Lerp(FVector(PosX + x, PosY + y + 1, PosZ + z), FVector(PosX + x, PosY + y + 1, PosZ + z + 1), interpolatedCrossingPoint);
				}

				crossBitMap <<= 4;

				int triangleIndex = 0;
				while (triTable[crossBitMap + triangleIndex] != -1)
				{
					// For each triangle in the look up table, create a triangle and add it to the list.
					int index1 = triTable[crossBitMap + triangleIndex];
					int index2 = triTable[crossBitMap + triangleIndex + 1];
					int index3 = triTable[crossBitMap + triangleIndex + 2];

					FDynamicMeshVertex Vertex0;
					Vertex0.Position = TerrainTransform.TransformVector(ChunkScale * interpolatedValues[index1]);
					FDynamicMeshVertex Vertex1;
					Vertex1.Position = TerrainTransform.TransformVector(ChunkScale * interpolatedValues[index2]);
					FDynamicMeshVertex Vertex2;
					Vertex2.Position = TerrainTransform.TransformVector(ChunkScale * interpolatedValues[index3]);

					Vertex0.TextureCoordinate.X = Vertex0.Position.X / 100.0f;
					Vertex0.TextureCoordinate.Y = Vertex0.Position.Y / 100.0f;
					
					Vertex1.TextureCoordinate.X = Vertex1.Position.X / 100.0f;
					Vertex1.TextureCoordinate.Y = Vertex1.Position.Y / 100.0f;

					Vertex2.TextureCoordinate.X = Vertex2.Position.X / 100.0f;
					Vertex2.TextureCoordinate.Y = Vertex2.Position.Y / 100.0f;

					// Fill Index buffer And Vertex buffer with the generated vertices.
					int32 VIndex0 = Positions.Find(Vertex0.Position);
					if (VIndex0 < 0)
					{
						VIndex0 = Positions.Add(Vertex0.Position);
						Indices.Add(VIndex0);
						Vertices.Add(Vertex0.Position);
						UVs.Add(FVector2D(Vertex0.TextureCoordinate));
					}
					else {
						Indices.Add(VIndex0);
					}

					int32 VIndex1 = Positions.Find(Vertex1.Position);
					if (VIndex1 < 0)
					{
						VIndex1 = Positions.Add(Vertex1.Position);
						Indices.Add(VIndex1);
						Vertices.Add(Vertex1.Position);
						UVs.Add(FVector2D(Vertex1.TextureCoordinate));
					}
					else {
						Indices.Add(VIndex1);
					}

					int32 VIndex2 = Positions.Find(Vertex2.Position);
					if (VIndex2 < 0)
					{
						VIndex2 = Positions.Add(Vertex2.Position);
						Indices.Add(VIndex2);
						Vertices.Add(Vertex2.Position);
						UVs.Add(FVector2D(Vertex2.TextureCoordinate));
					}
					else {
						Indices.Add(VIndex2);
					}

					++NumTriangles;
					triangleIndex += 3;

				}



			}
		}
	}



	VertexColors.SetNum(Vertices.Num(), false);
	Normals.SetNum(Vertices.Num(), false);
	Tangents.SetNum(Vertices.Num(), false);

	for (int32 Index = 0; Index < Indices.Num(); Index += 3)
	{
		const FVector Edge21 = Vertices[Indices[Index + 1]] - Vertices[Indices[Index + 2]];
		const FVector Edge20 = Vertices[Indices[Index + 0]] - Vertices[Indices[Index + 2]];
		FVector TriNormal = (Edge21 ^ Edge20).GetSafeNormal();

		FVector FaceTangentX = Edge20.GetSafeNormal();
		FVector FaceTangentY = (FaceTangentX ^ TriNormal).GetSafeNormal();
		FVector FaceTangentZ = TriNormal;

		// Use Gram-Schmidt orthogonalization to make sure X is orth with Z
		FaceTangentX -= FaceTangentZ * (FaceTangentZ | FaceTangentX);
		FaceTangentX.Normalize();

		// See if we need to flip TangentY when generating from cross product
		const bool bFlipBitangent = ((FaceTangentZ ^ FaceTangentX) | FaceTangentY) < 0.f;
		TriNormal.Normalize();
		FaceTangentX.Normalize();
		FProcMeshTangent tangent = FProcMeshTangent(FaceTangentX, bFlipBitangent);
		for (int32 i = 0; i < 3; i++)
		{
			Normals[Indices[Index + i]] = TriNormal;
			Tangents[Indices[Index + i]] = tangent;
			VertexColors[Indices[Index + i]] = FColor(1, 1, 1, 1);
		}
	}
	//UE_LOG(LogTemp, Warning, TEXT("NumTriangles: %d => %d,%d -> %d,%d"), NumTriangles, ChunkStartSize.X, ChunkStartSize.Y, ChunkEndSize.X, ChunkEndSize.Y);
	return NumTriangles;
}