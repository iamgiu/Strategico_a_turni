// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit.h"

// Sets default values
AUnit::AUnit()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
	Super::BeginPlay();

}

//void AUnit::Attack(AUnit* Target)
//{
//	int32 Damage = rand() % (MaxDamage - MinDamage + 1) + MinDamage; //The damage must be a random number between the minimum and maximum damageù

	//il Target subisce il danno comando + testo in cui si scrive quanto danno fa
//}

void AUnit::DamageTaken(int Damage)
{
	Hp -= Damage;

	//testo quanti danni ha preso la nostra unità
}

bool AUnit::IsAlive() const
{
	if (Hp > 0) {
		return true;
	}
	else {
		return false;
	}
}

// Called every frame
//void AUnit::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

// Called to bind functionality to input
//void AUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//}

