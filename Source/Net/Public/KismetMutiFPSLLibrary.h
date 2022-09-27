// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KismetMutiFPSLLibrary.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FDeathMatchPlayerData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	FName PlayerName;

	UPROPERTY(BlueprintReadWrite)
	int PlayerScore;

	UPROPERTY(BlueprintReadWrite)
	int KillNum;
	
	UPROPERTY(BlueprintReadWrite)
	int DeathNum;

	UPROPERTY(BlueprintReadWrite)
	float Health;
	FDeathMatchPlayerData()
	{
		PlayerName = TEXT("");
		PlayerScore = 0;
	}
};

UCLASS()
class NET_API UKismetMutiFPSLLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	UFUNCTION(BlueprintCallable,Category="Sort")
	static void SortValues(UPARAM(ref)TArray<FDeathMatchPlayerData>& Values);//要传入一个引用，因为要反射到蓝图，让蓝图也知道这是个引用，所以前面价格UPARAM的宏
};
