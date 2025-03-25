// Fill out your copyright notice in the Description page of Project Settings.

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
	// Sets default values for this actor's properties
	ATile();

	// set the player owner and the status of a tile
	void SetTileStatus(const int32 TileOwner, const ETileStatus TileStatus);

	// get the tile status
	ETileStatus GetTileStatus();

	// get the tile owner
	int32 GetOwner();

	//set the (x,y) position
	void SetGridPosition(const double InX, const double InY);

	//get the (x,y) position 
	FVector2D GetGridPosition();

	// Flag per indicare se il tile è occupato
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bIsOccupied;

	// Riferimento all'unità che occupa questo tile
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Grid")
	class AUnit* OccupyingUnit;

	// Coordinate del tile nella griglia
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 GridX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 GridY;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bIsObstacle;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* Scene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ETileStatus Status;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PlayerOwner;

	//(x,y) position of the tile
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector2D TileGridPosition;


//public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

};