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

    //Reference to the material interface for the piece when is normal
    //UPROPERTY(EditAnywhere, Category = "Materials")
    //UMaterialInterface* BaseMaterial;

    // Reference to  material interface for the piece when is selected
    //UPROPERTY(EditAnywhere, Category = "Materials")
    //UMaterialInterface* SelectedMaterial;

    //void ShowSelected();

    //void UnshowSelected();

protected:

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

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

    //Function for the movement of the unit
    //void Move(FVector Destination);

    //Function for the Attack of the unit
    //void Attack(AUnit* Target);

    //Reference to the static mesh component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components");
    UStaticMeshComponent* StaticMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit")
    FVector2D UnitGridPosition;

public:
    // Called every frame
//	virtual void Tick(float DeltaTime) override;

    //Function for Damage Taken by the Unit
    void DamageTaken(int32 Damage);

    //Function to verify that the unit is alive
    bool IsAlive() const;

// Called to bind functionality to input
    //virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
