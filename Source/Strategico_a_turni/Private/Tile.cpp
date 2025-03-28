// Fill out your copyright notice in the Description page of Project Settings.


#include "Tile.h"

/*
 * Constructor - sets default values for tile properties
 * Initializes components and state variables
 */
ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create components
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));

	// Set up component hierarchy
	SetRootComponent(Scene);
	StaticMeshComponent->SetupAttachment(Scene);

	// Initialize tile state
	Status = ETileStatus::EMPTY;
	PlayerOwner = -1;
	TileGridPosition = FVector2D(0, 0);
	bIsObstacle = false;
	bIsOccupied = false;
}

/*
 * Sets the player owner and status of the tile
 * @param TileOwner - Player ID that owns the tile
 * @param TileStatus - New status to set for the tile
 */
void ATile::SetTileStatus(const int32 TileOwner, const ETileStatus TileStatus)
{
	PlayerOwner = TileOwner;
	Status = TileStatus;
}

/*
 * Gets the current status of the tile
 * @return The tile's current status
 */
ETileStatus ATile::GetTileStatus()
{
	return ETileStatus();
}

/*
 * Gets the current owner of the tile
 * @return Player ID of the owner
 */
int32 ATile::GetOwner()
{
	return PlayerOwner;
}

/*
 * Sets the grid position of the tile
 * @param InX - X coordinate in the grid
 * @param InY - Y coordinate in the grid
 */
void ATile::SetGridPosition(const double InX, const double InY)
{
	TileGridPosition.Set(InX, InY);
}

/*
 * Gets the grid position of the tile
 * @return Vector2D containing the coordinates
 */
FVector2D ATile::GetGridPosition()
{
	return FVector2D();
}

/*
 * Called when the game starts or when spawned
 * Initializes the tile
 */
void ATile::BeginPlay()
{
	Super::BeginPlay();
	
}

