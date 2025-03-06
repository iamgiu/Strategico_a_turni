// Fill out your copyright notice in the Description page of Project Settings.


#include "GridManager.h"
#include "Unit.h"
#include "SaT_GameMode.h"

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
	GenerateField();

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
			Obj->SetGridPosition(IndexX, IndexY);
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
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("x=%f,y=%f"), XPos, YPos));
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

// Ottieni la posizione del mondo dalle coordinate della griglia
FVector AGridManager::GetWorldLocationFromGrid(int32 GridX, int32 GridY)
{
	// Calcola la posizione relativa 
	FVector RelativeLocation = GetRelativeLocationByXYPosition(GridX, GridY);

	// Converti la posizione relativa in una posizione globale
	FVector WorldLocation = GetActorTransform().TransformPosition(RelativeLocation);

	// Aggiungi un offset verticale per posizionare l'unità sulla superficie del tile
	// Presuppone che l'origine dell'unità sia alla sua base
	WorldLocation.Z += TileSize * 0.5f;

	return WorldLocation;
}

// Called every frame
//void AGameField::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

