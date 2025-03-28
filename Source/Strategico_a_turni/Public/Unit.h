// Fill out your copyright notice in the Description page of Project Settings.
// Create a general class for the units so that they follow the given rules and can thus add new classes if necessary

/*
 * Base Unit class for all game units (Sniper, Brawler, etc.)
 * Provides common functionality for movement, combat, and visualization
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Unit.generated.h"

UCLASS()
class STRATEGICO_A_TURNI_API AUnit : public APawn
{
    GENERATED_BODY()

public:

    // Constructor - initializes default values for unit properties
    AUnit();

    // Grid coordinates for the unit
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 GridX;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 GridY;

    // Indicates if the unit belongs to the player (true) or AI (false)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
    bool bIsPlayerUnit;

    // Indicates if the unit has already attacked this turn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit State")
    bool bHasAttackedThisTurn = false;

    // Material for the unit's normal state
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* BaseMaterial;

    // Material to use when the unit is selected
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* SelectedMaterial;

    // Team color materials
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* BlueMaterial;  // Material for player units

    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* RedMaterial;   // Material for AI units

    /*
     * Updates the unit's material based on its team
     */
    UFUNCTION(BlueprintCallable, Category = "Visual")
    void UpdateTeamColor();

    /*
     * Sets whether the unit belongs to the player or AI
     * @param bIsPlayer - True if unit belongs to player, false if AI
     */
    UFUNCTION(BlueprintCallable, Category = "Visual")
    void SetPlayerUnit(bool bIsPlayer);

    /*
     * Applies selected visual state to the unit
     */
    UFUNCTION(BlueprintCallable, Category = "Visual")
    void ShowSelected();

    /*
     * Removes selected visual state from the unit
     */
    UFUNCTION(BlueprintCallable, Category = "Visual")
    void UnshowSelected();

    /*
     * Applies damage to the unit
     * @param Damage - Amount of damage to apply
     */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void DamageTaken(int32 Damage);

    /*
     * Checks if the unit is still alive
     * @return True if unit has HP remaining, false if dead
     */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsAlive() const;

    /*
     * Moves the unit to a new grid position
     * @param NewGridX - Target X coordinate
     * @param NewGridY - Target Y coordinate
     * @return True if move was successful, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    virtual bool Move(int32 NewGridX, int32 NewGridY);

    /*
     * Performs an attack against another unit
     * @param Target - Unit to attack
     * @return True if attack was successful, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual bool Attack(AUnit* Target);

    // Unit stats - Override in child classes
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

    // Unit state tracking
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Visual")
    bool bIsSelected;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Gameplay")
    bool bHasMovedThisTurn;

    // Display name for UI
    UPROPERTY(BlueprintReadOnly, Category = "Unit Info")
    FString UnitTypeDisplayName;

    /*
     * Calculates random damage value between min and max damage
     * @return Damage amount
     */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    int32 CalculateDamage() const;

    /*
     * Static method to check if two units would destroy each other
     * Used for detecting potential draw scenarios
     */
    static bool CheckMutualDestruction(AUnit* Attacker, AUnit* Target);

    /*
     * Checks if a target unit is within attack range
     * @param Target - Unit to check range to
     * @return True if target is in range, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool IsTargetInRange(const AUnit* Target) const;

protected:


    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Reference to the static mesh component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* StaticMeshComponent;

    // Position on the grid (redundant with GridX/GridY)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
    FVector2D UnitGridPosition;
};