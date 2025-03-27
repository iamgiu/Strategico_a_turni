// Fill out your copyright notice in the Description page of Project Settings.


#include "GridManager.h"
#include "Unit.h"

// Sets default values
AGridManager::AGridManager()
{

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// size of the field (25x25)
	Size = 25;

	// tile dimension
	TileSize = 100.0f;

	// tile padding percentage 
	CellPadding = 0.01f;

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMatAsset(TEXT("/Game/Materials/M_BaseMaterial"));
	if (DefaultMatAsset.Succeeded())
	{
		DefaultTileMaterial = DefaultMatAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterial> HighlightMatAsset(TEXT("/Game/Materials/M_TileSelected"));
	if (HighlightMatAsset.Succeeded())
	{
		HighlightMaterial = HighlightMatAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterial> PathMatAsset(TEXT("/Game/Materials/M_Path"));
	if (PathMatAsset.Succeeded())
	{
		PathMaterial = PathMatAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterial> ObstacleMatAsset(TEXT("/Game/Materials/M_Obstacle"));
	if (ObstacleMatAsset.Succeeded())
	{
		ObstacleMaterial = ObstacleMatAsset.Object;
	}
}

void AGridManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	//normalized tilepadding
	NextCellPositionMultiplier = FMath::RoundToDouble(((TileSize + TileSize * CellPadding) / TileSize) * 100) / 100;
}

// Called when the game starts or when spawned
void AGridManager::BeginPlay()
{
	Super::BeginPlay();

	// Verify path material is set
	if (!PathMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("Path material is not set! Path visualization will not work correctly."));

		// Try to load it from the content browser as a fallback
		PathMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/Materials/M_Path"));

		if (PathMaterial)
		{
			UE_LOG(LogTemp, Warning, TEXT("Successfully loaded path material from content browser."));
		}
	}

	// Debug materials before generating field
	DebugMaterials();

	// Generate the field
	GenerateField();
}

void AGridManager::GenerateField()
{

	UE_LOG(LogTemp, Warning, TEXT("Generating field with %f%% obstacles"), ObstaclePercentage * 100.0f);
	// Clear all existing tiles first
	for (ATile* Tile : TileArray)
	{
		if (Tile)
		{
			Tile->Destroy();
		}
	}
	TileArray.Empty();
	TileMap.Empty();
	HighlightedTiles.Empty();
	PathTiles.Empty();

	// First, generate the basic grid without obstacles
	for (int32 IndexX = 0; IndexX < Size; IndexX++)
	{
		for (int32 IndexY = 0; IndexY < Size; IndexY++)
		{
			FVector Location = AGridManager::GetRelativeLocationByXYPosition(IndexX, IndexY);
			ATile* Obj = GetWorld()->SpawnActor<ATile>(TileClass, Location, FRotator::ZeroRotator);
			const float TileScale = TileSize / 100.0f;
			const float Zscaling = 0.01f;
			Obj->SetActorScale3D(FVector(TileScale, TileScale, Zscaling));

			// Set the grid position
			Obj->SetGridPosition(IndexX, IndexY);
			Obj->GridX = IndexX;
			Obj->GridY = IndexY;

			TileArray.Add(Obj);
			TileMap.Add(FVector2D(IndexX, IndexY), Obj);
		}
	}

	// After generating the basic grid, add obstacles
	GenerateObstacles();
}

FVector2D AGridManager::GetPosition(const FHitResult& Hit)
{
	return Cast<ATile>(Hit.GetActor())->GetGridPosition();
}

TArray<ATile*>& AGridManager::GetTileArray()
{
	return TileArray;
}

FVector AGridManager::GetRelativeLocationByXYPosition(const int32 InX, const int32 InY) const
{
	return TileSize * NextCellPositionMultiplier * FVector(InX, InY, 0);

}

FVector2D AGridManager::GetXYPositionByRelativeLocation(const FVector& Location) const
{
	const double XPos = Location.X / (TileSize * NextCellPositionMultiplier);
	const double YPos = Location.Y / (TileSize * NextCellPositionMultiplier);
	return FVector2D(XPos, YPos);
}

inline bool AGridManager::IsValidPosition(const FVector2D Position) const
{
	return 0 <= Position.X && Position.X < Size && 0 <= Position.Y && Position.Y < Size;
}

TArray<int32> AGridManager::GetLine(const FVector2D Begin, const FVector2D End) const
{
	int32 XSign;
	if (Begin.X == End.X)
	{
		XSign = 0;
	}
	else
	{
		XSign = Begin.X < End.X ? 1 : -1;
	}

	int32 YSign;
	if (Begin.Y == End.Y)
	{
		YSign = 0;
	}
	else
	{
		YSign = Begin.Y < End.Y ? 1 : -1;
	}

	TArray<int32> Line;
	int32 XVal = Begin.X - XSign;
	int32 YVal = Begin.Y - YSign;
	do
	{
		XVal += XSign;
		YVal += YSign;
		Line.Add((TileMap[FVector2D(XVal, YVal)])->GetOwner());
	} while (XVal != End.X || YVal != End.Y);

	return Line;
}

bool AGridManager::AllEqual(const TArray<int32>& Array) const
{
	if (Array.Num() == 0)
	{
		return false;
	}
	const int32 Value = Array[0];

	if (Value == NOT_ASSIGNED)
	{
		return false;
	}

	for (int32 IndexI = 1; IndexI < Array.Num(); IndexI++)
	{
		if (Value != Array[IndexI])
		{
			return false;
		}
	}

	return true;
}

// Verifica se una cella è occupata
bool AGridManager::IsCellOccupied(int32 GridX, int32 GridY)
{
	// Log this check for debugging
	UE_LOG(LogTemp, Display, TEXT("Checking cell occupation at (%d,%d)"), GridX, GridY);

	// Verify if coordinates are valid
	if (!IsValidPosition(FVector2D(GridX, GridY)))
	{
		UE_LOG(LogTemp, Warning, TEXT("Position (%d,%d) is outside grid - considered occupied"), GridX, GridY);
		return true; // Consider invalid positions as occupied
	}

	// Find the tile at the specified position
	FVector2D Position(GridX, GridY);
	if (!TileMap.Contains(Position))
	{
		UE_LOG(LogTemp, Warning, TEXT("No tile found at (%d,%d) - considered occupied"), GridX, GridY);
		return true; // If no tile exists, consider it occupied
	}

	ATile* Tile = TileMap[Position];
	if (!Tile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tile at (%d,%d) is NULL - considered occupied"), GridX, GridY);
		return true; // If tile pointer is null, consider it occupied
	}

	// Check both occupied state and obstacle state
	bool bOccupied = Tile->bIsOccupied;
	bool bObstacle = Tile->bIsObstacle;

	// Debug log the state
	if (bOccupied || bObstacle)
	{
		UE_LOG(LogTemp, Display, TEXT("Cell (%d,%d) is occupied=%s, obstacle=%s"),
			GridX, GridY,
			bOccupied ? TEXT("YES") : TEXT("NO"),
			bObstacle ? TEXT("YES") : TEXT("NO"));
	}

	// Return true if either occupied by a unit OR is an obstacle
	return bOccupied || bObstacle;
}

// Occupa una cella con un'unità
void AGridManager::OccupyCell(int32 GridX, int32 GridY, AUnit* Unit)
{
	// Verify if the coordinates are valid
	if (!IsValidPosition(FVector2D(GridX, GridY)))
	{
		return;
	}

	// Find the tile corresponding to the position
	FVector2D Position(GridX, GridY);
	if (TileMap.Contains(Position))
	{
		ATile* Tile = TileMap[Position];
		if (Tile)
		{
			// If Unit is nullptr, we're freeing the cell
			if (Unit == nullptr)
			{
				// Mark the tile as unoccupied
				Tile->bIsOccupied = false;

				// Clear the reference to the occupying unit
				Tile->OccupyingUnit = nullptr;

				// Log the cell being freed
				UE_LOG(LogTemp, Warning, TEXT("Cell at (%d,%d) has been freed"), GridX, GridY);
			}
			else
			{
				// Mark the tile as occupied
				Tile->bIsOccupied = true;

				// Store the reference to the occupying unit
				Tile->OccupyingUnit = Unit;

				// Update the unit's position on the grid
				Unit->GridX = GridX;
				Unit->GridY = GridY;

				// Position the unit in the world
				FVector WorldLocation = GetWorldLocationFromGrid(GridX, GridY);
				Unit->SetActorLocation(WorldLocation);

				UE_LOG(LogTemp, Warning, TEXT("Unit %s now occupies cell (%d,%d)"),
					*Unit->GetName(), GridX, GridY);
			}
		}
	}
}

FVector AGridManager::GetWorldLocationFromGrid(int32 GridX, int32 GridY)
{
	// Check if the grid coordinates are valid
	if (GridX < 0 || GridX >= Size || GridY < 0 || GridY >= Size) {
		UE_LOG(LogTemp, Error, TEXT("Invalid grid coordinates: X=%d, Y=%d"), GridX, GridY);
		// Return a default position if coordinates are invalid
		return FVector(0, 0, 50.0f);
	}

	// Adjust for the 2-unit offset (observed from your logs)
	// The visual coordinates are offset by +2 in both X and Y
	int32 AdjustedX = GridX;
	int32 AdjustedY = GridY;

	// Calculate position with adjusted coordinates
	FVector WorldLocation = GetRelativeLocationByXYPosition(AdjustedX, AdjustedY);

	// Add Z offset to make the unit visible above the tile
	WorldLocation.Z += 50.0f;

	UE_LOG(LogTemp, Warning, TEXT("World position calculated: X=%f, Y=%f, Z=%f (original coords: %d,%d, adjusted: %d,%d)"),
		WorldLocation.X, WorldLocation.Y, WorldLocation.Z, GridX, GridY, AdjustedX, AdjustedY);

	return WorldLocation;
}

void AGridManager::HighlightCell(int32 GridX, int32 GridY, bool bHighlight)
{
	// Verify if the position is valid
	if (!IsValidPosition(FVector2D(GridX, GridY)))
	{
		return;
	}

	// Get the tile at the specified position
	ATile* Tile = nullptr;
	if (TileMap.Contains(FVector2D(GridX, GridY)))
	{
		Tile = TileMap[FVector2D(GridX, GridY)];
	}

	if (Tile)
	{
		// Apply highlighting material to the tile
		if (bHighlight)
		{
			// Set highlight material - you'll need to create this material in your project
			if (HighlightMaterial)
			{
				Tile->StaticMeshComponent->SetMaterial(0, HighlightMaterial);
				HighlightedTiles.Add(Tile); // Add to tracked list
			}
		}
		else
		{
			// Restore original material
			Tile->StaticMeshComponent->SetMaterial(0, DefaultTileMaterial);
			HighlightedTiles.Remove(Tile); // Remove from tracked list
		}
	}
}

bool AGridManager::HighlightPath(TArray<FVector2D> PathPoints, bool bClearPrevious)
{
	UE_LOG(LogTemp, Warning, TEXT("HighlightPath called with %d points"), PathPoints.Num());

	// Verify path material exists
	if (!PathMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("PathMaterial is NULL! Cannot highlight path."));

		// Attempt to load material as fallback
		PathMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/Materials/M_Path"));

		if (!PathMaterial)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load path material from content browser!"));
			return false;
		}
	}

	if (bClearPrevious)
	{
		ClearPathHighlights();
	}

	// If we don't have at least 2 points, can't draw a path
	if (PathPoints.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not enough points to draw a path"));
		return false;
	}

	// Generate an obstacle-aware orthogonal path
	TArray<FVector2D> FullPath;

	for (int32 i = 0; i < PathPoints.Num() - 1; i++)
	{
		FVector2D Start = PathPoints[i];
		FVector2D End = PathPoints[i + 1];

		int32 x0 = FMath::FloorToInt(Start.X);
		int32 y0 = FMath::FloorToInt(Start.Y);
		int32 x1 = FMath::FloorToInt(End.X);
		int32 y1 = FMath::FloorToInt(End.Y);

		// Check if the entire path is blocked
		bool bPathBlocked = false;

		// First try horizontal movement
		if (x0 != x1)
		{
			int32 step = (x0 < x1) ? 1 : -1;
			for (int32 x = x0; x != x1; x += step)
			{
				if (IsCellOccupied(x, y0))
				{
					UE_LOG(LogTemp, Warning, TEXT("Path blocked horizontally at (%d,%d)"), x, y0);
					bPathBlocked = true;
					break;
				}
			}
		}

		// If horizontal path not blocked, check vertical movement
		if (!bPathBlocked && y0 != y1)
		{
			int32 step = (y0 < y1) ? 1 : -1;
			for (int32 y = y0; y != y1; y += step)
			{
				if (IsCellOccupied(x1, y))
				{
					UE_LOG(LogTemp, Warning, TEXT("Path blocked vertically at (%d,%d)"), x1, y);
					bPathBlocked = true;
					break;
				}
			}
		}

		// If path is not blocked, add path points
		if (!bPathBlocked)
		{
			// First move horizontally
			if (x0 != x1)
			{
				int32 step = (x0 < x1) ? 1 : -1;
				for (int32 x = x0; x != x1; x += step)
				{
					FullPath.Add(FVector2D(x, y0));
				}
			}

			// Then move vertically
			if (y0 != y1)
			{
				int32 step = (y0 < y1) ? 1 : -1;
				for (int32 y = y0; y != y1; y += step)
				{
					FullPath.Add(FVector2D(x1, y));
				}
			}

			// Add the final point
			FullPath.Add(End);
		}
		else
		{
			// If path is blocked, only add start point
			FullPath.Add(Start);
			UE_LOG(LogTemp, Warning, TEXT("Path between (%d,%d) and (%d,%d) is blocked"),
				x0, y0, x1, y1);
		}
	}

	// Highlight the path
	int32 highlightedCount = 0;
	for (const FVector2D& Point : FullPath)
	{
		int32 GridX = FMath::FloorToInt(Point.X);
		int32 GridY = FMath::FloorToInt(Point.Y);

		// Validate grid position
		if (!IsValidPosition(FVector2D(GridX, GridY)))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid path point: (%d, %d)"), GridX, GridY);
			continue;
		}

		// Find the tile
		ATile* Tile = nullptr;
		if (TileMap.Contains(FVector2D(GridX, GridY)))
		{
			Tile = TileMap[FVector2D(GridX, GridY)];
		}

		// Highlight the tile if it exists and is not an obstacle
		if (Tile && !Tile->bIsObstacle)
		{
			// Force material application
			Tile->StaticMeshComponent->SetMaterial(0, PathMaterial);
			Tile->StaticMeshComponent->MarkRenderStateDirty();

			PathTiles.Add(Tile);
			highlightedCount++;

			UE_LOG(LogTemp, Warning, TEXT("Highlighted path tile at (%d, %d)"), GridX, GridY);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Skipping tile at (%d, %d) - Tile is NULL or an obstacle"), GridX, GridY);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Path highlighting complete. Highlighted %d tiles"), highlightedCount);
	return highlightedCount > 0;
}

// Add this debug method to help troubleshoot path materials
void AGridManager::DebugMaterials()
{
    UE_LOG(LogTemp, Warning, TEXT("===== DEBUGGING MATERIALS ====="));
    UE_LOG(LogTemp, Warning, TEXT("DefaultTileMaterial: %s"), 
        DefaultTileMaterial ? *DefaultTileMaterial->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("HighlightMaterial: %s"), 
        HighlightMaterial ? *HighlightMaterial->GetName() : TEXT("NULL"));
    UE_LOG(LogTemp, Warning, TEXT("PathMaterial: %s"), 
        PathMaterial ? *PathMaterial->GetName() : TEXT("NULL"));
}

void AGridManager::ClearAllHighlights()
{
	// Reset all highlighted tiles to their original material
	for (ATile* Tile : HighlightedTiles)
	{
		if (Tile && DefaultTileMaterial)
		{
			Tile->StaticMeshComponent->SetMaterial(0, DefaultTileMaterial);
		}
	}

	HighlightedTiles.Empty();
}

void AGridManager::ClearPathHighlights()
{
	UE_LOG(LogTemp, Warning, TEXT("Clearing path highlights. Total tiles: %d"), PathTiles.Num());

	// Reset all path-highlighted tiles to their original material
	for (ATile* Tile : PathTiles)
	{
		if (Tile && Tile->StaticMeshComponent && DefaultTileMaterial)
		{
			Tile->StaticMeshComponent->SetMaterial(0, DefaultTileMaterial);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unable to reset tile material - Tile, StaticMeshComponent, or DefaultTileMaterial is NULL"));
		}
	}

	// Clear the path tiles array
	PathTiles.Empty();
}

void AGridManager::GenerateObstacles()
{
	// Calculate how many obstacles to generate based on percentage
	int32 TotalCells = Size * Size;
	int32 NumObstacles = FMath::RoundToInt(TotalCells * ObstaclePercentage);

	UE_LOG(LogTemp, Warning, TEXT("Generating %d obstacles (%.1f%% of grid)"),
		NumObstacles, ObstaclePercentage * 100.0f);

	// Keep track of which cells we've already processed
	TArray<bool> ProcessedCells;
	ProcessedCells.SetNumZeroed(TotalCells);

	// Place obstacles randomly
	int32 PlacedObstacles = 0;
	while (PlacedObstacles < NumObstacles)
	{
		// Generate random position
		int32 RandomX = FMath::RandRange(0, Size - 1);
		int32 RandomY = FMath::RandRange(0, Size - 1);
		int32 CellIndex = RandomY * Size + RandomX;

		// Skip if we already processed this cell
		if (ProcessedCells[CellIndex])
			continue;

		ProcessedCells[CellIndex] = true;

		// Get the tile at this position
		ATile* Tile = TileMap[FVector2D(RandomX, RandomY)];
		if (!Tile)
			continue;

		// Set as obstacle
		Tile->bIsObstacle = true;

		// ENSURE MATERIAL IS APPLIED - This is critical!
		if (ObstacleMaterial)
		{
			Tile->StaticMeshComponent->SetMaterial(0, ObstacleMaterial);
			UE_LOG(LogTemp, Warning, TEXT("Applied obstacle material to (%d,%d)"), RandomX, RandomY);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ObstacleMaterial is NULL! Cannot visualize obstacle at (%d,%d)"),
				RandomX, RandomY);
		}

		// Also mark as occupied so units can't move here
		Tile->bIsOccupied = true;

		PlacedObstacles++;
	}

	// Verify all obstacles have correct material
	for (int32 x = 0; x < Size; x++)
	{
		for (int32 y = 0; y < Size; y++)
		{
			FVector2D Position(x, y);
			if (TileMap.Contains(Position))
			{
				ATile* Tile = TileMap[Position];
				if (Tile && Tile->bIsObstacle && ObstacleMaterial)
				{
					// Double-check material is applied
					Tile->StaticMeshComponent->SetMaterial(0, ObstacleMaterial);
				}
			}
		}
	}

	// Call debug function to verify obstacle state
	DebugObstacles();

	EnsureGridConnectivity();
}

void AGridManager::DebugObstacles()
{
	UE_LOG(LogTemp, Warning, TEXT("========== OBSTACLE DEBUG =========="));
	UE_LOG(LogTemp, Warning, TEXT("ObstaclePercentage: %f"), ObstaclePercentage);

	int32 ObstacleCount = 0;
	int32 OccupiedCount = 0;
	int32 BothCount = 0;

	for (int32 x = 0; x < Size; x++)
	{
		for (int32 y = 0; y < Size; y++)
		{
			FVector2D Position(x, y);
			if (TileMap.Contains(Position))
			{
				ATile* Tile = TileMap[Position];
				if (Tile)
				{
					if (Tile->bIsObstacle && Tile->bIsOccupied)
					{
						BothCount++;
						UE_LOG(LogTemp, Warning, TEXT("Tile at (%d,%d) is BOTH obstacle AND occupied"), x, y);
					}
					else if (Tile->bIsObstacle)
					{
						ObstacleCount++;
					}
					else if (Tile->bIsOccupied)
					{
						OccupiedCount++;
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Total obstacles: %d"), ObstacleCount);
	UE_LOG(LogTemp, Warning, TEXT("Total occupied (non-obstacle): %d"), OccupiedCount);
	UE_LOG(LogTemp, Warning, TEXT("Total both obstacle AND occupied: %d"), BothCount);
	UE_LOG(LogTemp, Warning, TEXT("===================================="));
}

bool AGridManager::RemoveObstacleToCreatePath(int32 UnreachableX, int32 UnreachableY)
{
	// Find the nearest reachable non-obstacle cell (we know it exists from our BFS)
	int32 BestX = -1, BestY = -1;
	float BestDistance = FLT_MAX;

	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			ATile* Tile = TileMap[FVector2D(X, Y)];
			if (Tile && !Tile->bIsObstacle)
			{
				float Distance = FVector2D::DistSquared(FVector2D(X, Y), FVector2D(UnreachableX, UnreachableY));
				if (Distance < BestDistance)
				{
					BestDistance = Distance;
					BestX = X;
					BestY = Y;
				}
			}
		}
	}

	if (BestX == -1 || BestY == -1)
		return false;  // No reachable cells found (shouldn't happen)

	// Create a simple path by removing obstacles in a straight line
	int32 DiffX = UnreachableX - BestX;
	int32 DiffY = UnreachableY - BestY;

	// Move horizontally first, then vertically
	int32 StepX = (DiffX > 0) ? 1 : -1;
	if (DiffX != 0) StepX = (DiffX > 0) ? 1 : -1;

	// Remove obstacles horizontally
	for (int32 X = BestX; X != UnreachableX; X += StepX)
	{
		ATile* Tile = TileMap[FVector2D(X, BestY)];
		if (Tile && Tile->bIsObstacle)
		{
			Tile->bIsObstacle = false;
			Tile->bIsOccupied = false;
			if (DefaultTileMaterial)
			{
				Tile->StaticMeshComponent->SetMaterial(0, DefaultTileMaterial);
			}
			return true;  // We only need to remove one obstacle to fix connectivity
		}
	}

	// Remove obstacles vertically
	int32 StepY = (DiffY > 0) ? 1 : -1;
	for (int32 Y = BestY; Y != UnreachableY; Y += StepY)
	{
		ATile* Tile = TileMap[FVector2D(UnreachableX, Y)];
		if (Tile && Tile->bIsObstacle)
		{
			Tile->bIsObstacle = false;
			Tile->bIsOccupied = false;
			if (DefaultTileMaterial)
			{
				Tile->StaticMeshComponent->SetMaterial(0, DefaultTileMaterial);
			}
			return true;  // We only need to remove one obstacle to fix connectivity
		}
	}

	return false;  // No obstacles needed to be removed
}

bool AGridManager::EnsureGridConnectivity()
{
	// Create a copy of the current obstacle configuration
	TArray<bool> ObstacleMap;
	ObstacleMap.SetNumZeroed(Size * Size);

	// Populate obstacle map
	for (int32 x = 0; x < Size; x++)
	{
		for (int32 y = 0; y < Size; y++)
		{
			ObstacleMap[y * Size + x] = TileMap[FVector2D(x, y)]->bIsObstacle;
		}
	}

	// Identify regions (connected groups of cells)
	TArray<int32> RegionMap;
	RegionMap.SetNumZeroed(Size * Size);
	int32 RegionCount = IdentifyRegions(ObstacleMap, RegionMap);

	// If there's only one region, we're done
	if (RegionCount <= 1)
	{
		return true;
	}

	// Find the largest region (presumably the main playable area)
	int32 LargestRegionId = FindLargestRegion(RegionMap, RegionCount);

	// Connect smaller regions to the largest region
	bool bConnectivityImproved = ConnectRegions(ObstacleMap, RegionMap, LargestRegionId);

	// If connectivity was improved, update the grid
	if (bConnectivityImproved)
	{
		// Update obstacle status in tiles
		for (int32 x = 0; x < Size; x++)
		{
			for (int32 y = 0; y < Size; y++)
			{
				int32 Index = y * Size + x;
				TileMap[FVector2D(x, y)]->bIsObstacle = ObstacleMap[Index];

				// Update material if needed
				if (ObstacleMap[Index] && ObstacleMaterial)
				{
					TileMap[FVector2D(x, y)]->StaticMeshComponent->SetMaterial(0, ObstacleMaterial);
				}
				else if (!ObstacleMap[Index] && DefaultTileMaterial)
				{
					TileMap[FVector2D(x, y)]->StaticMeshComponent->SetMaterial(0, DefaultTileMaterial);
				}
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Grid connectivity improved by connecting regions"));
		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("Could not ensure grid connectivity"));
	return false;
}

int32 AGridManager::IdentifyRegions(const TArray<bool>& ObstacleMap, TArray<int32>& RegionMap)
{
	int32 CurrentRegion = 1;
	TArray<bool> Visited;
	Visited.SetNumZeroed(Size * Size);

	for (int32 x = 0; x < Size; x++)
	{
		for (int32 y = 0; y < Size; y++)
		{
			int32 Index = y * Size + x;

			// Skip if already visited or is an obstacle
			if (Visited[Index] || ObstacleMap[Index])
				continue;

			// Flood fill to mark this region
			FloodFillRegion(ObstacleMap, RegionMap, Visited, x, y, CurrentRegion);
			CurrentRegion++;
		}
	}

	return CurrentRegion - 1;
}

void AGridManager::FloodFillRegion(const TArray<bool>& ObstacleMap,
	TArray<int32>& RegionMap,
	TArray<bool>& Visited,
	int32 StartX, int32 StartY,
	int32 RegionId)
{
	// Directions: up, right, down, left
	int32 dx[] = { 0, 1, 0, -1 };
	int32 dy[] = { -1, 0, 1, 0 };

	// Queue for flood fill
	TQueue<FVector2D> Queue;
	Queue.Enqueue(FVector2D(StartX, StartY));

	while (!Queue.IsEmpty())
	{
		FVector2D Current;
		Queue.Dequeue(Current);

		int32 x = FMath::FloorToInt(Current.X);
		int32 y = FMath::FloorToInt(Current.Y);
		int32 Index = y * Size + x;

		// Skip if out of bounds, visited, or an obstacle
		if (x < 0 || x >= Size || y < 0 || y >= Size ||
			Visited[Index] || ObstacleMap[Index])
			continue;

		// Mark as visited and assign region
		Visited[Index] = true;
		RegionMap[Index] = RegionId;

		// Explore adjacent cells
		for (int32 i = 0; i < 4; i++)
		{
			int32 NewX = x + dx[i];
			int32 NewY = y + dy[i];

			// Ensure new coordinates are in bounds
			if (NewX >= 0 && NewX < Size && NewY >= 0 && NewY < Size)
			{
				Queue.Enqueue(FVector2D(NewX, NewY));
			}
		}
	}
}

int32 AGridManager::FindLargestRegion(const TArray<int32>& RegionMap, int32 RegionCount)
{
	// Count cells in each region
	TArray<int32> RegionSizes;
	RegionSizes.SetNumZeroed(RegionCount + 1);

	for (int32 i = 0; i < RegionMap.Num(); i++)
	{
		if (RegionMap[i] > 0)
		{
			RegionSizes[RegionMap[i]]++;
		}
	}

	// Find the region with the most cells
	int32 LargestRegionId = 1;
	for (int32 i = 2; i < RegionSizes.Num(); i++)
	{
		if (RegionSizes[i] > RegionSizes[LargestRegionId])
		{
			LargestRegionId = i;
		}
	}

	return LargestRegionId;
}

bool AGridManager::ConnectRegions(TArray<bool>& ObstacleMap,
	const TArray<int32>& RegionMap,
	int32 LargestRegionId)
{
	bool bConnectivityImproved = false;
	int32 MaxAttempts = Size * Size / 10; // Limit total obstacle removals
	int32 AttemptsCount = 0;

	// Find border cells between regions
	for (int32 x = 1; x < Size - 1; x++)
	{
		for (int32 y = 1; y < Size - 1; y++)
		{
			int32 Index = y * Size + x;

			// Only consider obstacle cells
			if (!ObstacleMap[Index])
				continue;

			// Check adjacent cells
			bool bAdjacentToLargeRegion = false;
			bool bAdjacentToSmallRegion = false;
			int32 SmallRegionId = 0;

			// Check 4-way adjacency
			int32 dx[] = { 0, 1, 0, -1 };
			int32 dy[] = { -1, 0, 1, 0 };

			for (int32 i = 0; i < 4; i++)
			{
				int32 NewX = x + dx[i];
				int32 NewY = y + dy[i];
				int32 AdjIndex = NewY * Size + NewX;

				if (NewX >= 0 && NewX < Size && NewY >= 0 && NewY < Size)
				{
					// Skip if adjacent cell is an obstacle
					if (ObstacleMap[AdjIndex])
						continue;

					if (RegionMap[AdjIndex] == LargestRegionId)
					{
						bAdjacentToLargeRegion = true;
					}
					else if (RegionMap[AdjIndex] > 0 && RegionMap[AdjIndex] != LargestRegionId)
					{
						bAdjacentToSmallRegion = true;
						SmallRegionId = RegionMap[AdjIndex];
					}
				}
			}

			// If this obstacle is a potential connection point
			if (bAdjacentToLargeRegion && bAdjacentToSmallRegion)
			{
				// Remove the obstacle
				ObstacleMap[Index] = false;
				bConnectivityImproved = true;
				AttemptsCount++;

				// Stop if we've made enough connections
				if (AttemptsCount >= MaxAttempts)
				{
					UE_LOG(LogTemp, Warning, TEXT("Reached max attempts to connect regions"));
					return bConnectivityImproved;
				}
			}
		}
	}

	return bConnectivityImproved;
}
