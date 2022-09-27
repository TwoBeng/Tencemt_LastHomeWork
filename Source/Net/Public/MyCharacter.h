// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyPlayerController.h"
#include "WeaponBaseServer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "MyCharacter.generated.h"

UCLASS()
class NET_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter();

	UFUNCTION()
	void DelayPlayControlCallBack();
	
	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTpBodysDeathAnimMontage;

#pragma region Component
private:
	UPROPERTY(Category=character,VisibleAnywhere,BlueprintReadWrite,meta=(AllowPrivateAccess = "true"))
	float Health;
	

	
	UPROPERTY(Category=character,VisibleAnywhere,BlueprintReadOnly,meta=(AllowPrivateAccess = "true"))
	UCameraComponent* PlayerCamera;

	UPROPERTY(Category=character,VisibleAnywhere,BlueprintReadWrite,meta=(AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FPArmMesh;

	UPROPERTY(Category=character,BlueprintReadOnly,meta=(AllowPrivateAccess = "true"))
	UAnimInstance* ClientArmsAnimBP;//创建手臂的类，游戏begin令他为手臂，然后用它调用播放蒙太奇
	UPROPERTY(Category=character,BlueprintReadOnly,meta=(AllowPrivateAccess = "true"))
	UAnimInstance* ServerBodyAnimBP;

	UPROPERTY(BlueprintReadOnly,meta=(AllowPrivateAccess = "true"))//防止被当作野指针回收
	AMyPlayerController* FPSPlayerController;//创建一个蓝图创建的自己控制器

	//Reload
	UPROPERTY(Replicated)
	bool IsFireing;
	UPROPERTY(Replicated)
	bool IsReloading;

	UFUNCTION()
	void DelayPlayArmReloadCallBack();
	
#pragma endregion 

#pragma region Weapon

public:
	UFUNCTION(BlueprintCallable)
	void EquipPrimary(AWeaponBaseServer* WeaponBaseServer);

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool IsHaveServerWeaon = false;

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateFPArmsBlendPose(int NewIndex);

	UPROPERTY(EditAnywhere,Replicated)
	EWeaponType ActiveWeapon;
private:
	
	UPROPERTY(Replicated,meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerPrimaryWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClient* ClientPrimaryWeapon;

	UPROPERTY(EditAnywhere,meta=(AllowPrivateAccess = "true"))
	bool IsAi;

	UPROPERTY(EditAnywhere,meta=(AllowPrivateAccess = "true"))
	EWeaponType TestStartWeapon;
	

	void StartWithKindofWeapon();

	void PurchaseWeapon(EWeaponType WeaponType);

	AWeaponBaseClient* GetCurrentClientFPArmsWeaponActor();
	AWeaponBaseServer* GetCurrentServerTPWeaponActor();
	
#pragma endregion
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#pragma region  InputEvent
	void MoveRight(float AxisValue);
	void MoveForward(float AxisValue);

	void JumpAction();
	void StopJumpAction();

	void LowSpeedWalkAction();
	void NormalSpeedWalkAction();
    
	void FireAction();
	void StopFireAction();

	void AimingAction();
	void StopAimingAction();

	void ThrowingAction();
	
	void ReloadAction();

#pragma endregion 
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

#pragma region Fire
public:
	//计时器
	FTimerHandle AutomaticFireTimerHandle;
	void AutomaticFire();

	//后坐力
	float NewVerticalRecoilAmount;
	float OldVerticalRecoilAmount;
	float VerticalRecoilAmount;
	float RecoilXCoordPerShoot;
	void ResetRecoil();
	float NewHorizontalRecoilAmount;
	float OldHorizontalRecoilAmount;
	float HorizontalRecoilAmount;
	
	
	//步枪的射击相关
	void FireWeaponPrimary();
	void StopFireWeaponPrimary();
	//服务器射线检测
	void RifeLineTrace(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);

	//狙击枪的射击相关
	void FireWeaponSniper();
	void StopFireWeaponSniper();
	UPROPERTY(Replicated)
	bool IsAiming;
	//服务器射线检测
	void SniperLineTrace(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	UFUNCTION()
	void DelaySniperShootCallBack();

	UPROPERTY(VisibleAnywhere,Category = "SniperUI")
	UUserWidget* WidgetScope;

	UPROPERTY(EditAnywhere,Category = "SniperUI")
	TSubclassOf<UUserWidget> SniperScopeBPClass;
	

	void DamegePlayer(UPhysicalMaterial* PhysicalMaterial,AActor* DamegeActor,FVector& HitDirection,FHitResult& HitInfo);

	UFUNCTION(BlueprintCallable)
	void OnHit(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);

	void DeathMatchDeath(AActor* Damager);
#pragma  endregion 
	
public:
#pragma region Networking

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerLowSpeedWalkAction();//调用的名字
	void ServerLowSpeedWalkAction_Implementation(); //方法体实现
	bool ServerLowSpeedWalkAction_Validate();

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerNormalSpeedWalkAction();//调用的名字
	void ServerNormalSpeedWalkAction_Implementation(); //方法体实现
	bool ServerNormalSpeedWalkAction_Validate();

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFireRifleAction(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);//调用的名字
	void ServerFireRifleAction_Implementation(FVector CameraLocation,FRotator CameraRotation,bool IsMoving); //方法体实现
	bool ServerFireRifleAction_Validate(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFireSniperAction(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);//调用的名字
	void ServerFireSniperAction_Implementation(FVector CameraLocation,FRotator CameraRotation,bool IsMoving); //方法体实现
	bool ServerFireSniperAction_Validate(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerReloadPrimary();//调用的名字
	void ServerReloadPrimary_Implementation(); //方法体实现
	bool ServerReloadPrimary_Validate();

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerStopFiring();//调用的名字
	void ServerStopFiring_Implementation(); //方法体实现
	bool ServerStopFiring_Validate();

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerSetAiming(bool AimingState);//调用的名字
	void ServerSetAiming_Implementation(bool AimingState); //方法体实现
	bool ServerSetAiming_Validate(bool AimingState);

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerThrowWeapon();//调用的名字
	void ServerThrowWeapon_Implementation(); //方法体实现
	bool ServerThrowWeapon_Validate();
	
	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiShooting();//调用的名字
	void MultiShooting_Implementation(); //方法体实现
	bool MultiShooting_Validate();

	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiReloadAction();//调用的名字
	void MultiReloadAction_Implementation(); //方法体实现
	bool MultiReloadAction_Validate();

	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiDeathAmim();//调用的名字
	void MultiDeathAmim_Implementation(); //方法体实现
	bool MultiDeathAmim_Validate();

	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiSpawnBulletDecal(FVector Location, FRotator Rotation);//调用的名字
	void MultiSpawnBulletDecal_Implementation(FVector Location, FRotator Rotation); //方法体实现
	bool MultiSpawnBulletDecal_Validate(FVector Location, FRotator Rotation);

	UFUNCTION(Client,Reliable)
	void ClientThrowWeapon();//调用的名字
	
	
	
	UFUNCTION(Client,Reliable)
	void ClientEquipFPArmsPrimary();

	UFUNCTION(Client,Reliable)
	void ClientFire();

	UFUNCTION(Client,Reliable)
	void ClientUpdateAmmoUI(int32 ClipCurrentAmmo,int32 GunCurrentAmmo);

	UFUNCTION(Client,Reliable)
	void ClientUpdateHealthUi(float NewHealth);

	UFUNCTION(Client,Reliable)
	void ClientReload();

	UFUNCTION(Client,Reliable)
	void ClientDeathMatchDeath();

	UFUNCTION(Client,Reliable)
	void ClientRecoil();

	UFUNCTION(Client,Reliable)
	void ClientAiming();

	UFUNCTION(Client,Reliable)
	void ClientEndAiming();
#pragma endregion 

};



