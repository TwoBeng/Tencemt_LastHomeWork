// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KataCharacter.generated.h"

UCLASS()
class NET_API AKataCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AKataCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// UPROPERTY(Category=character,BlueprintReadOnly,meta=(AllowPrivateAccess = "true"))
	// UAnimInstance* ServerBodyAnimBP;
	//
	// UPROPERTY(EditAnywhere)
	// UAnimMontage* ServerTpBodysDeathAnimMontage;

	
};
