// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tile.h"
#include "GameFramework/Actor.h"
#include "GridManager.generated.h"

class AUnit;

UCLASS()
class STRATEGICO_A_TURNI_API AGridManager : public AActor
{

	GENERATED_BODY()

public:
	// keeps track of tiles
	UPROPERTY(Transient)
	TArray<ATile*> TileArray;

	//given a position returns a tile
	UPROPERTY(Transient)
	TMap<FVector2D, ATile*> TileMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float NextCellPositionMultiplier;

	static const int32 NOT_ASSIGNED = -1;

	// size of field
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Size;

	// TSubclassOf template class that provides UClass type safety
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ATile> TileClass;

	// tile padding percentage
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CellPadding;

	// tile size
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TileSize;

	// Sets default values for this actor's properties
	AGridManager();

	// Called when an instance of this class is placed (in editor) or spawned
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// remove all signs from the field
	//UFUNCTION(BlueprintCallable)
	//void ResetField();

	// generate an empty game field
	void GenerateField();

	// return a (x,y) position given a hit (click) on a field tile
	FVector2D GetPosition(const FHitResult& Hit);

	// return the array of tile pointers
	TArray<ATile*>& GetTileArray();

	// return a relative position given (x,y) position
	FVector GetRelativeLocationByXYPosition(const int32 InX, const int32 InY) const;

	// return (x,y) position given a relative position
	FVector2D GetXYPositionByRelativeLocation(const FVector& Location) const;


	// checking if is a valid field position
	inline bool IsValidPosition(const FVector2D Position) const;

	// get a line given a begin and end positions
	TArray<int32> GetLine(const FVector2D Begin, const FVector2D End) const;

	// check if a line contains all equal elements
	bool AllEqual(const TArray<int32>& Array) const;

	// Verifica se una cella è occupata
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool IsCellOccupied(int32 GridX, int32 GridY);

	// Occupa una cella con un'unità
	UFUNCTION(BlueprintCallable, Category = "Grid")
	void OccupyCell(int32 GridX, int32 GridY, AUnit* Unit);

	// Ottieni la posizione del mondo dalle coordinate della griglia
	UFUNCTION(BlueprintCallable, Category = "Grid")
	FVector GetWorldLocationFromGrid(int32 GridX, int32 GridY);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void HighlightCell(int32 GridX, int32 GridY, bool bHighlight);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ClearAllHighlights();

	UPROPERTY(EditDefaultsOnly, Category = "Materials")
	UMaterialInterface* DefaultTileMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Materials")
	UMaterialInterface* HighlightMaterial;

	// Private member to track highlighted tiles
	UPROPERTY()
	TArray<ATile*> HighlightedTiles;

	UPROPERTY(EditDefaultsOnly, Category = "Materials")
	UMaterialInterface* PathMaterial;  // Material for highlighting the path

	UPROPERTY()
	TArray<ATile*> PathTiles;  // Track path tiles separately from movement range

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void HighlightPath(TArray<FVector2D> PathPoints, bool bClearPrevious = true);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ClearPathHighlights();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	void DebugMaterials();

	UPROPERTY(EditDefaultsOnly, Category = "Materials")
	UMaterialInterface* ObstacleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ObstaclePercentage;

	void GenerateObstacles();
	void VerifyGridConnectivity();
	bool RemoveObstacleToCreatePath(int32 UnreachableX, int32 UnreachableY);

	//public:	
	//	// Called every frame
	//	virtual void Tick(float DeltaTime) override;

};