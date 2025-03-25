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

	ObstaclePercentage = 0.1f;

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

	// Debug materials before generating field
	DebugMaterials();

	// Generate the field
	GenerateField();

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
}

//void AGridManager::ResetField()
//{
//	for (ATile* Obj : TileArray)
//	{
//		Obj->SetTileStatus(NOT_ASSIGNED, ETileStatus::EMPTY);
//	}
//
	// send broadcast event to registered objects 
//	OnResetEvent.Broadcast();

//	A_GameMode* GameMode = Cast<ATTT_GameMode>(GetWorld()->GetAuthGameMode());
//	GameMode->IsGameOver = false;
//	GameMode->MoveCounter = 0;
//	GameMode->ChoosePlayerAndStartGame();
//}

void AGridManager::GenerateField()
{
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
	// Verify if coordinates are valid
	if (!IsValidPosition(FVector2D(GridX, GridY)))
	{
		return true; // Consider invalid positions as occupied
	}

	// Find the tile at the specified position
	FVector2D Position(GridX, GridY);
	if (TileMap.Contains(Position))
	{
		ATile* Tile = TileMap[Position];
		if (Tile)
		{
			// Check if the tile is occupied or is an obstacle
			return Tile->bIsOccupied || Tile->bIsObstacle;
		}
	}

	return false;
}

// Occupa una cella con un'unità
void AGridManager::OccupyCell(int32 GridX, int32 GridY, AUnit* Unit)
{
	// Verifica se le coordinate sono valide
	if (!IsValidPosition(FVector2D(GridX, GridY)))
	{
		return;
	}

	// Trova il tile corrispondente
	FVector2D Position(GridX, GridY);
	if (TileMap.Contains(Position))
	{
		ATile* Tile = TileMap[Position];
		if (Tile)
		{
			// Segna il tile come occupato
			Tile->bIsOccupied = true;

			// Memorizza il riferimento all'unità (aggiungi questa proprietà a ATile)
			Tile->OccupyingUnit = Unit;

			// Aggiorna anche l'unità con la sua posizione sulla griglia (aggiungi queste proprietà a AUnit)
			if (Unit)
			{
				Unit->GridX = GridX;
				Unit->GridY = GridY;

				// Posiziona l'unità nel mondo
				FVector WorldLocation = GetWorldLocationFromGrid(GridX, GridY);
				Unit->SetActorLocation(WorldLocation);
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

void AGridManager::HighlightPath(TArray<FVector2D> PathPoints, bool bClearPrevious)
{
	// Extensive debug logging
	UE_LOG(LogTemp, Warning, TEXT("===== HIGHLIGHT PATH DEBUG ====="));
	UE_LOG(LogTemp, Warning, TEXT("Highlighting path with %d points"), PathPoints.Num());

	// Clear previous path highlights if specified
	if (bClearPrevious)
	{
		ClearPathHighlights();
	}

	// Early return if no path points
	if (PathPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No path points to highlight!"));
		return;
	}

	// Forcefully load the path material if not already loaded
	if (!PathMaterial)
	{
		PathMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/Materials/M_Path"));

		if (!PathMaterial)
		{
			UE_LOG(LogTemp, Error, TEXT("CRITICAL: Could not load path material from /Game/Materials/M_Path"));
			return;
		}
	}

	// Verify material is valid
	if (!PathMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("Path material is NULL after loading attempt!"));
		return;
	}

	// Clear and reset path tiles tracking
	PathTiles.Empty();

	// Highlight each point in the path
	for (int32 i = 0; i < PathPoints.Num(); i++)
	{
		const FVector2D& Point = PathPoints[i];
		int32 GridX = FMath::FloorToInt(Point.X);
		int32 GridY = FMath::FloorToInt(Point.Y);

		// Skip invalid positions
		if (!IsValidPosition(FVector2D(GridX, GridY)))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid path position (%d, %d), skipping"), GridX, GridY);
			continue;
		}

		// Skip the first point (unit's current position) if it's not the only point
		if (i == 0 && PathPoints.Num() > 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("Skipping first point (%d, %d)"), GridX, GridY);
			continue;
		}

		// Get the tile at this position
		FVector2D Position(GridX, GridY);
		ATile* Tile = nullptr;
		if (TileMap.Contains(Position))
		{
			Tile = TileMap[Position];
		}

		if (Tile && Tile->StaticMeshComponent)
		{
			// Create a dynamic material instance for more precise control
			UMaterialInstanceDynamic* DynamicPathMaterial = UMaterialInstanceDynamic::Create(PathMaterial, this);

			if (DynamicPathMaterial)
			{
				// Apply the dynamic material instance
				Tile->StaticMeshComponent->SetMaterial(0, DynamicPathMaterial);

				UE_LOG(LogTemp, Warning, TEXT("Applied dynamic path material to tile at (%d, %d)"), GridX, GridY);

				PathTiles.Add(Tile);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create dynamic material instance for tile at (%d, %d)"), GridX, GridY);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid tile or static mesh component at (%d, %d)"), GridX, GridY);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Total path tiles highlighted: %d"), PathTiles.Num());
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

		// Skip positions around edges (leaving room for unit placement)
		if (RandomX < 0 || RandomX >= Size - 1 || RandomY < 0 || RandomY >= Size - 1)
			continue;

		// Get the tile at this position
		ATile* Tile = TileMap[FVector2D(RandomX, RandomY)];
		if (!Tile)
			continue;

		// Set as obstacle
		Tile->bIsObstacle = true;

		// Apply obstacle material
		if (ObstacleMaterial)
		{
			Tile->StaticMeshComponent->SetMaterial(0, ObstacleMaterial);
		}

		// Also mark as occupied so units can't move here
		Tile->bIsOccupied = true;

		PlacedObstacles++;
	}

	// Verify connectivity to ensure all non-obstacle cells can be reached
	VerifyGridConnectivity();
}

void AGridManager::VerifyGridConnectivity()
{
	// Start with all cells marked as unreachable
	TArray<bool> Visited;
	Visited.SetNumZeroed(Size * Size);

	// Find first non-obstacle cell to start from
	int32 StartX = 0, StartY = 0;
	bool bFoundStart = false;

	for (int32 Y = 0; Y < Size && !bFoundStart; Y++)
	{
		for (int32 X = 0; X < Size && !bFoundStart; X++)
		{
			ATile* Tile = TileMap[FVector2D(X, Y)];
			if (Tile && !Tile->bIsObstacle)
			{
				StartX = X;
				StartY = Y;
				bFoundStart = true;
			}
		}
	}

	// Run BFS from the start position
	TQueue<FVector2D> Queue;
	Queue.Enqueue(FVector2D(StartX, StartY));
	Visited[StartY * Size + StartX] = true;

	// Direction vectors (4 directions)
	int32 dx[] = { 1, -1, 0, 0 };
	int32 dy[] = { 0, 0, 1, -1 };

	while (!Queue.IsEmpty())
	{
		FVector2D Current;
		Queue.Dequeue(Current);

		// Try all four directions
		for (int32 i = 0; i < 4; i++)
		{
			int32 NewX = FMath::FloorToInt(Current.X) + dx[i];
			int32 NewY = FMath::FloorToInt(Current.Y) + dy[i];

			// Skip if out of bounds
			if (NewX < 0 || NewX >= Size || NewY < 0 || NewY >= Size)
				continue;

			int32 NewIndex = NewY * Size + NewX;

			// Skip if already visited
			if (Visited[NewIndex])
				continue;

			// Skip if it's an obstacle
			ATile* Tile = TileMap[FVector2D(NewX, NewY)];
			if (Tile && !Tile->bIsObstacle)
			{
				Visited[NewIndex] = true;
				Queue.Enqueue(FVector2D(NewX, NewY));
			}
		}
	}

	// Check for any unreachable cells and fix them
	int32 FixedObstacles = 0;
	for (int32 Y = 0; Y < Size; Y++)
	{
		for (int32 X = 0; X < Size; X++)
		{
			int32 Index = Y * Size + X;
			ATile* Tile = TileMap[FVector2D(X, Y)];

			if (Tile && !Tile->bIsObstacle && !Visited[Index])
			{
				// This is a non-obstacle cell that couldn't be reached
				// Find the nearest obstacle and remove it to create a path
				if (RemoveObstacleToCreatePath(X, Y))
				{
					FixedObstacles++;
				}
			}
		}
	}

	if (FixedObstacles > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Fixed %d obstacles to ensure grid connectivity"), FixedObstacles);
	}
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

// Called every frame
//void AGameField::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}
