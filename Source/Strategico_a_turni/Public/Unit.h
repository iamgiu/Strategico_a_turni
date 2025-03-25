// Fill out your copyright notice in the Description page of Project Settings.
// Create a general class for the units so that they follow the given rules and can thus add new classes if necessary

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Unit.generated.h"

UCLASS()
class STRATEGICO_A_TURNI_API AUnit : public APawn
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AUnit();

    // Coordinate dell'unità sulla griglia
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 GridX;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 GridY;

    // Indica se l'unità appartiene al giocatore o all'AI
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
    bool bIsPlayerUnit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit State")
    bool bHasAttackedThisTurn = false;

    // Reference to the material interface for the piece when is normal
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* BaseMaterial;

    // Reference to material interface for the piece when is selected
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* SelectedMaterial;

    // Team color materials
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* BlueMaterial;  // Material for player units

    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* RedMaterial;   // Material for AI units

    UFUNCTION(BlueprintCallable, Category = "Visual")
    void UpdateTeamColor();

    UFUNCTION(BlueprintCallable, Category = "Visual")
    void SetPlayerUnit(bool bIsPlayer);

    // Visual feedback when unit is selected
    UFUNCTION(BlueprintCallable, Category = "Visual")
    void ShowSelected();

    UFUNCTION(BlueprintCallable, Category = "Visual")
    void UnshowSelected();

    // Function for Damage Taken by the Unit
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void DamageTaken(int32 Damage);

    // Function to verify that the unit is alive
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsAlive() const;

    // Function for the movement of the unit
    UFUNCTION(BlueprintCallable, Category = "Movement")
    virtual bool Move(int32 NewGridX, int32 NewGridY);

    // Function for the Attack of the unit
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual bool Attack(AUnit* Target);

    // Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    int32 Movement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    FString TypeofAttack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    int32 RangeAttack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    int32 MinDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    int32 MaxDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    int32 Hp;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Visual")
    bool bIsSelected;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bHasMovedThisTurn;

    UPROPERTY(BlueprintReadOnly, Category = "Unit Info")
    FString UnitTypeDisplayName;

    // Calculate damage with random range between min and max
    UFUNCTION(BlueprintCallable, Category = "Combat")
    int32 CalculateDamage() const;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Reference to the static mesh component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* StaticMeshComponent;

    // Position on the grid
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
    FVector2D UnitGridPosition;

    // Check if target is within attack range
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsTargetInRange(const AUnit* Target) const;

public:
    // Called every frame
    // virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    // virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};