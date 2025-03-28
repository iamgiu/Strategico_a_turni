// Fill out your copyright notice in the Description page of Project Settings.

/*
 * Tile class representing a single cell in the game grid
 * Handles grid position, occupancy state, and owner information
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaT_Enums.h"
#include "Tile.generated.h"

UCLASS()
class STRATEGICO_A_TURNI_API ATile : public AActor
{
	GENERATED_BODY()
	
public:	
	
	// Constructor - initializes default properties for the tile
	ATile();

	/**
	 * Sets the player owner and status of the tile
	 * @param TileOwner - Player ID that owns the tile
	 * @param TileStatus - New status to set for the tile
	 */
	void SetTileStatus(const int32 TileOwner, const ETileStatus TileStatus);

	/*
	 * Gets the current status of the tile
	 * @return The tile's current status
	 */
	ETileStatus GetTileStatus();

	/*
	 * Gets the current owner of the tile
	 * @return Player ID of the owner
	 */
	int32 GetOwner();

	/*
	 * Sets the grid position of the tile
	 * @param InX - X coordinate in the grid
	 * @param InY - Y coordinate in the grid
	 */
	void SetGridPosition(const double InX, const double InY);

	/*
	 * Gets the grid position of the tile
	 * @return Vector2D containing the coordinates
	 */
	FVector2D GetGridPosition();

	// Flag indicating if the tile is occupied by a unit
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bIsOccupied;

	// Reference to the unit currently occupying this tile
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Grid")
	class AUnit* OccupyingUnit;

	// X coordinate of the tile in the grid
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 GridX;

	// Y coordinate of the tile in the grid
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 GridY;

	// Static mesh component for visual representation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	// Flag indicating if the tile is an obstacle
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bIsObstacle;

protected:

	/*
	 * Called when the game starts or when spawned
	 */
	virtual void BeginPlay() override;

	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* Scene;

	// Current status of the tile
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ETileStatus Status;

	// Player ID of the current owner
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PlayerOwner;

	// Grid position of the tile (redundant with GridX/GridY)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector2D TileGridPosition;

};