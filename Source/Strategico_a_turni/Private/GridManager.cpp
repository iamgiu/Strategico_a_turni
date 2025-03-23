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
	for (int32 IndexX = 0; IndexX < Size; IndexX++)
	{
		for (int32 IndexY = 0; IndexY < Size; IndexY++)
		{
			FVector Location = AGridManager::GetRelativeLocationByXYPosition(IndexX, IndexY);
			ATile* Obj = GetWorld()->SpawnActor<ATile>(TileClass, Location, FRotator::ZeroRotator);
			const float TileScale = TileSize / 100.0f;
			const float Zscaling = 0.01f;
			Obj->SetActorScale3D(FVector(TileScale, TileScale, Zscaling));

			// This is the critical part - setting the grid position
			Obj->SetGridPosition(IndexX, IndexY);
			// You should also be setting the GridX and GridY properties directly
			Obj->GridX = IndexX;
			Obj->GridY = IndexY;

			TileArray.Add(Obj);
			TileMap.Add(FVector2D(IndexX, IndexY), Obj);
		}
	}
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
	// Verifica se le coordinate sono valide
	if (!IsValidPosition(FVector2D(GridX, GridY)))
	{
		return true; // Considera le posizioni non valide come occupate
	}

	// Trova il tile corrispondente
	FVector2D Position(GridX, GridY);
	if (TileMap.Contains(Position))
	{
		ATile* Tile = TileMap[Position];
		if (Tile)
		{
			// Verifica se il tile è occupato
			return Tile->bIsOccupied;
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
    // Clear any existing path only if specified
    if (bClearPrevious)
    {
        ClearPathHighlights();
    }

    // Debug info
    UE_LOG(LogTemp, Warning, TEXT("===== HIGHLIGHT PATH ====="));
    UE_LOG(LogTemp, Warning, TEXT("Highlighting path with %d points"), PathPoints.Num());
    UE_LOG(LogTemp, Warning, TEXT("PathMaterial is %s"), PathMaterial ? TEXT("VALID") : TEXT("NULL"));

    // Early return if no path points
    if (PathPoints.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No path points to highlight!"));
        return;
    }

    // Debug: Log all path points
    for (int32 i = 0; i < PathPoints.Num(); i++)
    {
        UE_LOG(LogTemp, Warning, TEXT("Path point %d: (%f, %f)"), 
            i, PathPoints[i].X, PathPoints[i].Y);
    }

    // Highlight each point in the path
    for (int32 i = 0; i < PathPoints.Num(); i++)
    {
        const FVector2D& Point = PathPoints[i];
        int32 GridX = FMath::FloorToInt(Point.X);
        int32 GridY = FMath::FloorToInt(Point.Y);

        // Skip invalid positions
        if (!IsValidPosition(FVector2D(GridX, GridY)))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid position (%d, %d), skipping"), GridX, GridY);
            continue;
        }

        // Skip the first point (unit's current position) if it's not the only point
        if (i == 0 && PathPoints.Num() > 1)
        {
            UE_LOG(LogTemp, Warning, TEXT("Skipping first point (%d, %d)"), GridX, GridY);
            continue;
        }

        // Get the tile at this position
        ATile* Tile = nullptr;
        if (TileMap.Contains(FVector2D(GridX, GridY)))
        {
            Tile = TileMap[FVector2D(GridX, GridY)];
        }

        if (Tile)
        {
            // IMPORTANT: Make sure we have a valid path material
            if (PathMaterial)
            {
                UE_LOG(LogTemp, Warning, TEXT("Applying path material to tile at (%d, %d)"), GridX, GridY);
                Tile->StaticMeshComponent->SetMaterial(0, PathMaterial);
                PathTiles.Add(Tile);
            }
            else if (HighlightMaterial)
            {
                UE_LOG(LogTemp, Warning, TEXT("No path material, using highlight material at (%d, %d)"), GridX, GridY);
                Tile->StaticMeshComponent->SetMaterial(0, HighlightMaterial);
                PathTiles.Add(Tile);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("No path or highlight material available!"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No tile found at position (%d, %d)"), GridX, GridY);
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
	// Reset all path-highlighted tiles to their original material
	for (ATile* Tile : PathTiles)
	{
		if (Tile && DefaultTileMaterial)
		{
			Tile->StaticMeshComponent->SetMaterial(0, DefaultTileMaterial);
		}
	}

	PathTiles.Empty();
}

/*FVector AGridManager::GetWorldLocationFromGrid(int32 GridX, int32 GridY)
{
	// Usa NextCellPositionMultiplier per mantenere coerenza con il resto del codice
	FVector WorldLocation = GetRelativeLocationByXYPosition(GridX-1, GridY-1);

	// Aggiungi offset verticale per rendere l'unità visibile sopra il tile
	WorldLocation.Z += 50.0f;

	UE_LOG(LogTemp, Warning, TEXT("Posizione mondo calcolata: X=%f, Y=%f, Z=%f"),
		WorldLocation.X, WorldLocation.Y, WorldLocation.Z);

	return WorldLocation;
}*/

// Called every frame
//void AGameField::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}
