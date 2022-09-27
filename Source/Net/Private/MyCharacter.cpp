// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"

//#include "AudioMixerDevice.h"
//#include "EditorTutorial.h"
//#include "ParticleHelper.h"
//#include "Evaluation/IMovieSceneEvaluationHook.h"
//#include "AIController.h"
#include "KataCharacter.h"
#include "Components/DecalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
//#include "Math/UnrealMathNeon.h"
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

 #pragma region Component
	//构造函数里面初始化.h文件的声明的变量，如 相机组件和第一人称手臂组件
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	if(PlayerCamera)//判断指针是否有效
	{
		PlayerCamera->SetupAttachment(RootComponent);//将摄像机组件添加到根组件
		PlayerCamera->bUsePawnControlRotation = true;//摄像机的旋转（绕pitch）
	}
	FPArmMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmMesh"));
	if(FPArmMesh)
	{
		FPArmMesh->SetupAttachment(PlayerCamera);
		FPArmMesh->SetOnlyOwnerSee(true);//设置第一人称的手臂模型只能自己能看到
	}
	Mesh->SetOwnerNoSee(true);//这个是继承Character的Mesh，相当于第三人称的Mesh，设置它不能被自己看见

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);//设置碰撞，外部的碰撞由胶囊体提供，所以设置QueryOnly
	Mesh->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
#pragma endregion 
}

void AMyCharacter::DelayPlayControlCallBack()
{
	FPSPlayerController = Cast<AMyPlayerController>(GetController());
	if(FPSPlayerController)
	{
		FPSPlayerController->CreatePlayerUI();
	}
	else
	{
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;
		ActionInfo.ExecutionFunction = TEXT("DelayPlayControlCallBack");
		ActionInfo.UUID = FMath::Rand();
		ActionInfo.Linkage = 0;
		UKismetSystemLibrary::Delay(this,0.5,ActionInfo);
		
	}
}


// Called when the game starts or when spawned
void AMyCharacter:: BeginPlay()
{
	Super::BeginPlay();
	Health = 100;
	IsFireing = false;
	IsReloading = false;
	IsAiming = false;
	OnTakePointDamage.AddDynamic(this,&AMyCharacter::OnHit);//事件发生就会调用OnHit事件，回调函数  类似于球形碰撞体，当碰撞时就回调 详见WeaponServer

	//播放动画赋予动画蓝图初始值
	ClientArmsAnimBP = FPArmMesh->GetAnimInstance();
	ServerBodyAnimBP = Mesh->GetAnimInstance();
	
	FPSPlayerController = Cast<AMyPlayerController>(GetController());
	if(FPSPlayerController)
	{
		FPSPlayerController->CreatePlayerUI();
	}
	else
	{
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;
		ActionInfo.ExecutionFunction = TEXT("DelayPlayControlCallBack");
		ActionInfo.UUID = FMath::Rand();
		ActionInfo.Linkage = 0;
		UKismetSystemLibrary::Delay(this,0.5,ActionInfo);
		
	}
	
		StartWithKindofWeapon();
	
	
}



void AMyCharacter:: GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const//重载父类函数  让子弹数也能replicate到客户端
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);//调用父类的方法，避免崩溃  有的会用到
	DOREPLIFETIME_CONDITION(AMyCharacter,IsFireing,COND_None);
	DOREPLIFETIME_CONDITION(AMyCharacter,IsReloading,COND_None);
	DOREPLIFETIME_CONDITION(AMyCharacter,ActiveWeapon,COND_None);
	DOREPLIFETIME_CONDITION(AMyCharacter,IsAiming,COND_None);
	DOREPLIFETIME_CONDITION(AMyCharacter,ServerPrimaryWeapon,COND_None);

}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAction(TEXT("LowSpeedWalk"),IE_Pressed,this,&AMyCharacter::LowSpeedWalkAction);
	InputComponent->BindAction(TEXT("LowSpeedWalk"),IE_Released,this,&AMyCharacter::NormalSpeedWalkAction);

	InputComponent->BindAction(TEXT("Jump"),IE_Pressed,this,&AMyCharacter::JumpAction);
	InputComponent->BindAction(TEXT("Jump"),IE_Released,this,&AMyCharacter::StopJumpAction);

	InputComponent->BindAction(TEXT("Fire"),IE_Pressed,this,&AMyCharacter::FireAction);
	InputComponent->BindAction(TEXT("Fire"),IE_Released,this,&AMyCharacter::StopFireAction);

	InputComponent->BindAction(TEXT("Aiming"),IE_Pressed,this,&AMyCharacter::AimingAction);
	InputComponent->BindAction(TEXT("Aiming"),IE_Released,this,&AMyCharacter::StopAimingAction);

	InputComponent->BindAction(TEXT("Throw"),IE_Pressed,this,&AMyCharacter::ThrowingAction);
	

	InputComponent->BindAction(TEXT("Reload"),IE_Pressed,this,&AMyCharacter::ReloadAction);
	
	InputComponent->BindAxis(TEXT("MoveForward"),this,&AMyCharacter::MoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"),this,&AMyCharacter::MoveRight);

	InputComponent->BindAxis(TEXT("Turn"),this,&AMyCharacter::AddControllerYawInput);
	InputComponent->BindAxis(TEXT("LookUp"),this,&AMyCharacter::AddControllerPitchInput);
	
}


#pragma  region NetWorking
void AMyCharacter::ServerLowSpeedWalkAction_Implementation()
{
	CharacterMovement->MaxWalkSpeed = 300;
}

bool AMyCharacter::ServerLowSpeedWalkAction_Validate()
{
	return true;
}

void AMyCharacter::ServerNormalSpeedWalkAction_Implementation()
{
	CharacterMovement->MaxWalkSpeed = 600;
}

bool AMyCharacter::ServerNormalSpeedWalkAction_Validate()
{
	return true;
}

void AMyCharacter:: ServerFireRifleAction_Implementation(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	if(ServerPrimaryWeapon)
	{
		//多播（只在服务器调用，谁调谁多播）
		ServerPrimaryWeapon->MultiShootingEffect();//枪口闪光以及声音

		ServerPrimaryWeapon->ClipCurrentAmmo -= 1;
		ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);//RPC在客户端执行，更新弹药UI

		//多播身体射击蒙太奇（让其他人看到你在射击）
		MultiShooting();
		IsFireing = true;
		
		RifeLineTrace(CameraLocation, CameraRotation, IsMoving);
	}
	

	
}

bool AMyCharacter::ServerFireRifleAction_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	return true;
}

void AMyCharacter::ServerFireSniperAction_Implementation(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	if(ServerPrimaryWeapon)
	{

		
		//多播（只在服务器调用，谁调谁多播）
		ServerPrimaryWeapon->MultiShootingEffect();//枪口闪光以及声音

		ServerPrimaryWeapon->ClipCurrentAmmo -= 1;
		ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);//RPC在客户端执行，更新弹药UI

		//多播身体射击蒙太奇（让其他人看到你在射击）
		MultiShooting();
		IsFireing = true;
		
		SniperLineTrace(CameraLocation, CameraRotation, IsMoving);
	}
	if(ClientPrimaryWeapon)
	{
		//开一个计时器来判断射击动画是否播完，再把IsFireing 改为Fasle
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;
		ActionInfo.ExecutionFunction = TEXT("DelaySniperShootCallBack");
		ActionInfo.UUID = FMath::Rand();
		ActionInfo.Linkage = 0;
		UKismetSystemLibrary::Delay(this,ClientPrimaryWeapon->ClientArmsFireAnimMontage->GetPlayLength(),ActionInfo);
	}
}

bool AMyCharacter::ServerFireSniperAction_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	return true;
}

void AMyCharacter::ServerReloadPrimary_Implementation()
{
	//客户端播放手臂动画  服务端身体多播动画  数据更新  UI
	if(ServerPrimaryWeapon)
	{
		if(ServerPrimaryWeapon->GunCurrentAmmo > 0 && ServerPrimaryWeapon->ClipCurrentAmmo < ServerPrimaryWeapon->MAXClipAmmo)
		{
			ClientReload();
			MultiReloadAction();
			IsReloading = true;
			if(ClientPrimaryWeapon)
			{
				FLatentActionInfo ActionInfo;
				ActionInfo.CallbackTarget = this;
				ActionInfo.ExecutionFunction = TEXT("DelayPlayArmReloadCallBack");
				ActionInfo.UUID = FMath::Rand();
				ActionInfo.Linkage = 0;
				UKismetSystemLibrary::Delay(this,ClientPrimaryWeapon->ClientArmsReloadAnimMontage->GetPlayLength(),ActionInfo);
			}
		}
	}
	
	// UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Reload")));
	
}

bool AMyCharacter::ServerReloadPrimary_Validate()
{
	return true;
}

void AMyCharacter::ServerStopFiring_Implementation()
{
	IsFireing = false;
}

bool AMyCharacter::ServerStopFiring_Validate()
{
	return true;
}

void AMyCharacter::ServerSetAiming_Implementation(bool AimingState)
{
	IsAiming = AimingState;
}

bool AMyCharacter::ServerSetAiming_Validate(bool AimingState)
{
	return true;
}

void AMyCharacter::ServerThrowWeapon_Implementation()
{
	if(ServerPrimaryWeapon)
	{
		ServerPrimaryWeapon->Destroy();
		ServerPrimaryWeapon = nullptr;
		// ActiveWeapon = EWeaponType::None;
		ClientThrowWeapon();
	}
}

bool AMyCharacter::ServerThrowWeapon_Validate()
{
	return true;
}

void AMyCharacter::ClientThrowWeapon_Implementation()
{
	if(ClientPrimaryWeapon)
	{
		ClientPrimaryWeapon->Destroy();
		ClientPrimaryWeapon = nullptr;
		ClientUpdateAmmoUI(0,0);
		
	}
}



void AMyCharacter::ClientUpdateAmmoUI_Implementation(int32 ClipCurrentAmmo, int32 GunCurrentAmmo)
{
	if(FPSPlayerController)
	{
		FPSPlayerController->UpdateAmmoUI(ClipCurrentAmmo,GunCurrentAmmo);
	}
}

void AMyCharacter::ClientFire_Implementation()
{
	//枪体动画
	AWeaponBaseClient* CurrentClientWeapon = GetCurrentClientFPArmsWeaponActor();
	if(CurrentClientWeapon)
	{
		CurrentClientWeapon->PlayShootAnimation();
		//手臂动画
		UAnimMontage* ClientArmFireMontage = CurrentClientWeapon->ClientArmsFireAnimMontage;
		ClientArmsAnimBP->Montage_SetPlayRate(ClientArmFireMontage,1);
		ClientArmsAnimBP->Montage_Play(ClientArmFireMontage);

		//播放声音
		CurrentClientWeapon->DisaplayWeaponEffect();

		//镜头抖动
		FPSPlayerController->PlayerCameraShake(CurrentClientWeapon->CameraShakeClass);

		//十字线扩散
		FPSPlayerController->DoCrosshairRecoil();
	}

	
}

void AMyCharacter::ClientEquipFPArmsPrimary_Implementation()
{
	UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Client FpArms")));
	if(ServerPrimaryWeapon)
	{
		UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("有Server primary Weapon")));
		if(ClientPrimaryWeapon)
		{
			UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("有Client primary Weapon")));
		}
		else
		{
			// ActiveWeapon = ServerPrimaryWeapon->KindofWeapon;
			UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("没有Client primary Weapon")));
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner = this;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ClientPrimaryWeapon = GetWorld()->SpawnActor<AWeaponBaseClient>(ServerPrimaryWeapon->ClientWeaponBaseBpClass,
				GetActorTransform(),
				SpawnInfo);
			FName WeaponSocketName = TEXT("WeaponSocket");
			UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Active Weapon = %d"),ActiveWeapon));
			if(ActiveWeapon == EWeaponType::Sniper)
			{
				WeaponSocketName = TEXT("AWP_Socket");
			}
			if(ActiveWeapon == EWeaponType::M4A1)
			{
				
				WeaponSocketName = TEXT("M4A1_Socket");
				UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Equip M4A1")));
			}
			ClientPrimaryWeapon->K2_AttachToComponent(FPArmMesh,WeaponSocketName,EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,
				true);
			ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);
			//改变手臂动画
			if(ClientPrimaryWeapon)
			{
				UpdateFPArmsBlendPose(ClientPrimaryWeapon->BlendIndex);	
			}
			
		}
	}
}


void AMyCharacter::MultiShooting_Implementation()
{
	if(ServerBodyAnimBP)
	{
		if(ServerPrimaryWeapon)
		{
			ServerBodyAnimBP->Montage_Play(ServerPrimaryWeapon->ServerTPBodysShootAnimMontage);
		}
	}
}

bool AMyCharacter::MultiShooting_Validate()
{
	return true;
}

void AMyCharacter::MultiReloadAction_Implementation()
{

	AWeaponBaseServer* ServerCurrentWeapon = GetCurrentServerTPWeaponActor();
	if(ServerBodyAnimBP)
	{
		if(ServerCurrentWeapon)
		{
			UAnimMontage* ServerAimMontage = ServerCurrentWeapon->ServerTPBodysReloadAnimMontage;
			ServerBodyAnimBP->Montage_Play(ServerAimMontage);
			
		}
	}
}

bool AMyCharacter::MultiReloadAction_Validate()
{
	return true;
}

void AMyCharacter::MultiDeathAmim_Implementation()
{
	if(ServerBodyAnimBP)
	{
		
		ServerBodyAnimBP->Montage_Play(ServerTpBodysDeathAnimMontage);
			
		
	}
}

bool AMyCharacter::MultiDeathAmim_Validate()
{
	return true;
}

void AMyCharacter::MultiSpawnBulletDecal_Implementation(FVector Location, FRotator Rotation)
{
	if(ServerPrimaryWeapon)
	{
		UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(),ServerPrimaryWeapon->BulletDecalMaterial,FVector(8,8,8),
			Location,Rotation,10);
		if(Decal)
		{
			Decal->SetFadeScreenSize(0.001);
		}
	}
}

bool AMyCharacter::MultiSpawnBulletDecal_Validate(FVector Location, FRotator Rotation)
{
	return true;
}

void AMyCharacter::ClientEndAiming_Implementation()
{
	if(ClientPrimaryWeapon)
	{
		ClientPrimaryWeapon->SetActorHiddenInGame(false);
		if(FPArmMesh)
		{
			FPArmMesh->SetHiddenInGame(false);
		}
		if(PlayerCamera)
		{
			PlayerCamera->SetFieldOfView(90);//把镜头设置回默认的 90；		
		}
	}
	if(WidgetScope)
	{
		WidgetScope->RemoveFromParent();	
	}
}

void AMyCharacter::ClientAiming_Implementation()
{
	if(ClientPrimaryWeapon)
	{
		ClientPrimaryWeapon->SetActorHiddenInGame(true);
		if(FPArmMesh)
		{
			FPArmMesh->SetHiddenInGame(true);
		}
		if(PlayerCamera)
		{
			PlayerCamera->SetFieldOfView(ClientPrimaryWeapon->AimDistance);		
		}
	}
	WidgetScope = CreateWidget<UUserWidget>(GetWorld(),SniperScopeBPClass);
	if(WidgetScope)
	{
		WidgetScope->AddToViewport();	
	}
	
}

void AMyCharacter::ClientRecoil_Implementation()
{
	if(ServerPrimaryWeapon)
	{
		UCurveFloat* VerticalRecoilCurve = ServerPrimaryWeapon->VerticalRecoilCurve;
		UCurveFloat* HorizontalRecoilCurve = ServerPrimaryWeapon->HorizontalRecoilCurve;
		RecoilXCoordPerShoot += 0.1;
		if(VerticalRecoilCurve)
		{
			NewVerticalRecoilAmount = VerticalRecoilCurve->GetFloatValue(RecoilXCoordPerShoot);
			VerticalRecoilAmount = NewVerticalRecoilAmount - OldVerticalRecoilAmount;
			if(HorizontalRecoilCurve)
			{
				NewHorizontalRecoilAmount = HorizontalRecoilCurve->GetFloatValue(RecoilXCoordPerShoot);
				HorizontalRecoilAmount = NewHorizontalRecoilAmount - OldHorizontalRecoilAmount;
				if(FPSPlayerController)
				{
					FRotator ControllerRotator = FPSPlayerController->GetControlRotation();
					FPSPlayerController->SetControlRotation(FRotator(ControllerRotator.Pitch + VerticalRecoilAmount,ControllerRotator.Yaw + HorizontalRecoilAmount,ControllerRotator.Roll));
				}
				OldVerticalRecoilAmount = NewVerticalRecoilAmount;OldHorizontalRecoilAmount = NewHorizontalRecoilAmount;
			}
		}
			
	}
	
}

void AMyCharacter::ClientDeathMatchDeath_Implementation()
{
	AWeaponBaseClient* ClientCurrentWeapon = GetCurrentClientFPArmsWeaponActor();
	
	if(ClientCurrentWeapon)
	{
		ClientCurrentWeapon->Destroy();
	}

	if(FPArmMesh)
	{
		FPArmMesh->DestroyComponent();
	}
}

void AMyCharacter::ClientReload_Implementation()
{
	AWeaponBaseClient* ClientCurrentWeapon = GetCurrentClientFPArmsWeaponActor();
	if(ClientCurrentWeapon)
	{
		UAnimMontage* ClientAimMontage = ClientCurrentWeapon->ClientArmsReloadAnimMontage;
		ClientArmsAnimBP->Montage_Play(ClientAimMontage);
		ClientCurrentWeapon->PlayReloadAnimation();
	}
	
}

void AMyCharacter::ClientUpdateHealthUi_Implementation(float NewHealth)
{
	if(FPSPlayerController)
	{
		FPSPlayerController->UpdateHealthUI(NewHealth);
	}
	
}

#pragma endregion

#pragma region InputEvent
void AMyCharacter::MoveRight(float AxisValue)
{
	AddMovementInput(GetActorRightVector(),AxisValue,false);
}

void AMyCharacter::MoveForward(float AxisValue)
{
	AddMovementInput(GetActorForwardVector(),AxisValue,false);
}

void AMyCharacter::JumpAction()
{
	 Jump();
}

void AMyCharacter::StopJumpAction()
{
	StopJumping();
}

void AMyCharacter::LowSpeedWalkAction()
{
	CharacterMovement->MaxWalkSpeed = 300;
	ServerLowSpeedWalkAction();
}

void AMyCharacter::NormalSpeedWalkAction()
{
	CharacterMovement->MaxWalkSpeed = 600;
	ServerNormalSpeedWalkAction();
}

void AMyCharacter::FireAction()
{
	
		switch (ActiveWeapon)
		{
		case EWeaponType::Ak47:
			{
				FireWeaponPrimary();
			}
			break;
		case EWeaponType::M4A1:
			{
				FireWeaponPrimary();
			}
			break;
		case EWeaponType::Sniper:
			{
				FireWeaponSniper();
			}
			break;
		default:
			{
				
			}
			break;
		}
	
	
}

void AMyCharacter::StopFireAction()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::Ak47:
		{
			StopFireWeaponPrimary();
		}
		break;
	case EWeaponType::M4A1:
		{
			StopFireWeaponPrimary();
		}
		break;
	case EWeaponType::Sniper:
		{
			StopFireWeaponSniper();
		}
		break;
	}
}

void AMyCharacter::AimingAction()
{
	//贴瞄准镜的UI，关闭枪体的可见性，摄像头距离拉远  客户端RPC
	//改变IsAiming 的bool值  服务端RPC
	if(ActiveWeapon == EWeaponType::Sniper && !IsReloading)
	{

		ServerSetAiming(true);//将IsAiming 设为true  IsAiming 是Replicate的
		ClientAiming();
	}
}

void AMyCharacter::StopAimingAction()
{
	//删除瞄准镜的UI，开启枪体的可见性，摄像头距离拉近
	if(ActiveWeapon == EWeaponType::Sniper)
	{
		ServerSetAiming(false);
		ClientEndAiming();
	}
}

void AMyCharacter::ThrowingAction()
{
	if(ServerPrimaryWeapon)
	{
		ServerThrowWeapon();
	}
}

//换弹
void AMyCharacter::ReloadAction()
{
	if(!IsReloading)//如果正在播放动画 就不播放了  避免鬼畜
	{
		if(!IsFireing)
		{
			switch (ActiveWeapon)
			{
			case EWeaponType::Ak47:
				{
					ServerReloadPrimary();
				}
				break;
			case EWeaponType::M4A1:
				{
					ServerReloadPrimary();
				}
				break;
			case EWeaponType::Sniper:
				{
					if(!IsAiming)
					{
						ServerReloadPrimary();	
					}
				}
				break;
			}
		}
	}
}


#pragma endregion  


#pragma region Weapon

void AMyCharacter::DelayPlayArmReloadCallBack()
{
	int32 GunCurrentAmmo = ServerPrimaryWeapon->GunCurrentAmmo;
	int32 ClipCurrentAmmo = ServerPrimaryWeapon->ClipCurrentAmmo;
	int32 const MAXClipAmmo = ServerPrimaryWeapon->MAXClipAmmo;
	if(MAXClipAmmo - ClipCurrentAmmo >= GunCurrentAmmo)
	{
		ClipCurrentAmmo += GunCurrentAmmo;
		GunCurrentAmmo = 0;
		
	}
	else
	{
		GunCurrentAmmo -= MAXClipAmmo - ClipCurrentAmmo;
		ClipCurrentAmmo = MAXClipAmmo;
		
	}
	IsReloading = false;
	ServerPrimaryWeapon->GunCurrentAmmo = GunCurrentAmmo;
	ServerPrimaryWeapon->ClipCurrentAmmo = ClipCurrentAmmo;
	ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);

	// UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Delay")));
}

void AMyCharacter:: EquipPrimary(AWeaponBaseServer* WeaponBaseServer)
{
	 if(ServerPrimaryWeapon)
	 {
	 }
	 else
	 {
	 	ServerPrimaryWeapon = WeaponBaseServer;
	 	ServerPrimaryWeapon->SetOwner(this);
	 	ServerPrimaryWeapon->K2_AttachToComponent(Mesh,TEXT("hand_rSocket"),EAttachmentRule::SnapToTarget,
			 EAttachmentRule::SnapToTarget,
			 EAttachmentRule::SnapToTarget,
			 true );
	 	ActiveWeapon = ServerPrimaryWeapon->KindofWeapon;
	 	UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("ServerPrimary Weapon = %d"),ServerPrimaryWeapon->KindofWeapon));
	 	UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Active Weapon = %d"),ActiveWeapon));
	 	ClientEquipFPArmsPrimary();
	 }
	 
	
	 
}


void AMyCharacter::StartWithKindofWeapon()
{
	if(HasAuthority())
	{
		PurchaseWeapon(TestStartWeapon);
	}
}

void AMyCharacter::PurchaseWeapon(EWeaponType WeaponType)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	switch (WeaponType)
	{
	case EWeaponType::Ak47:
		{
			AWeaponBaseServer* ServerWeapon;
			// if(!ServerPrimaryWeapon)
			
				UClass* BluePrintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/BluePrint/Weapon/Ak47/Server_Ak47_BP.Server_Ak47_BP_C'"));
				ServerWeapon = GetWorld()->SpawnActor<AWeaponBaseServer>(BluePrintVar,
					GetActorTransform(),
					SpawnInfo);
				IsHaveServerWeaon = true;
			
			// else
			// {
			// 	ServerWeapon = ServerPrimaryWeapon;
			// }
				ServerWeapon->EquipWeapon();//服务器调用装备武器这个函数，动态生成第三人称武器（baseonserver）,没碰撞，调用EquipPrimary装备武器，因为你第三人称武器是replicate，客户端也会复制一份，有碰撞，就走碰撞装备武器
				ActiveWeapon = EWeaponType::Ak47;
				EquipPrimary(ServerWeapon);
			
			
		}
		break;
	case EWeaponType::M4A1:
		{
			AWeaponBaseServer* ServerWeapon;
			
			
			UClass* BluePrintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/BluePrint/Weapon/M4A1/Server_M4A1_BP.Server_M4A1_BP_C'"));
			ServerWeapon = GetWorld()->SpawnActor<AWeaponBaseServer>(BluePrintVar,
				GetActorTransform(),
				SpawnInfo);
			IsHaveServerWeaon = true;
			ServerWeapon->EquipWeapon();//服务器调用装备武器这个函数，动态生成第三人称武器（baseonserver）,没碰撞，调用EquipPrimary装备武器，因为你第三人称武器是replicate，客户端也会复制一份，有碰撞，就走碰撞装备武器
			ActiveWeapon = EWeaponType::M4A1;
			EquipPrimary(ServerWeapon);
			
		}
		break;
	case EWeaponType::Sniper:
		{
			AWeaponBaseServer* ServerWeapon;
			UClass* BluePrintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/BluePrint/Weapon/Sniper/Server_Sniper_BP.Server_Sniper_BP_C'"));
			ServerWeapon = GetWorld()->SpawnActor<AWeaponBaseServer>(BluePrintVar,
				GetActorTransform(),
				SpawnInfo);
			IsHaveServerWeaon = true;
			ServerWeapon->EquipWeapon();//服务器调用装备武器这个函数，动态生成第三人称武器（baseonserver）,没碰撞，调用EquipPrimary装备武器，因为你第三人称武器是replicate，客户端也会复制一份，有碰撞，就走碰撞装备武器
			ActiveWeapon = EWeaponType::Sniper;
			EquipPrimary(ServerWeapon);
		}
		break;
		default:
			{
				 
			}
	}
}

AWeaponBaseClient* AMyCharacter::GetCurrentClientFPArmsWeaponActor()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::Ak47:
		{
			return ClientPrimaryWeapon;
		}
	
	case EWeaponType::M4A1:
		{
			return ClientPrimaryWeapon;
		}
	case EWeaponType::Sniper:
		{
			return ClientPrimaryWeapon;
		}
	}
	return nullptr;
}

AWeaponBaseServer* AMyCharacter::GetCurrentServerTPWeaponActor()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::Ak47:
		{
			return ServerPrimaryWeapon;
		}
	case EWeaponType::M4A1:
		{
			return ServerPrimaryWeapon;
		}
	case EWeaponType::Sniper:
		{
			return ServerPrimaryWeapon;
		}
	}
	return nullptr;
}
#pragma endregion 

#pragma region Fire
void AMyCharacter::AutomaticFire()
{
	if(ServerPrimaryWeapon->ClipCurrentAmmo > 0 && !IsReloading)
	{
	
		if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
		{
			ServerFireRifleAction(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		}
		else
		{
			ServerFireRifleAction(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);
		}
		ClientFire();
		ClientRecoil();
	}
	else
	{
		StopFireWeaponPrimary();
	}
}

void AMyCharacter::ResetRecoil()
{
	NewVerticalRecoilAmount = 0;
	OldVerticalRecoilAmount = 0;
	VerticalRecoilAmount = 0;
	RecoilXCoordPerShoot = 0;
	NewHorizontalRecoilAmount = 0;
	OldHorizontalRecoilAmount = 0;
	HorizontalRecoilAmount = 0;
	
}

void AMyCharacter::FireWeaponPrimary()
{
	//服务端（减少弹药，射线检测（三种），伤害应用，单孔生成）

	
	//客户端：（枪体动画播放，手臂动画，播放声音，应用屏幕抖动，后坐力，枪口的闪光效果）
	//联机系统开发
	if(ServerPrimaryWeapon)
	{ 
		if (ServerPrimaryWeapon->ClipCurrentAmmo > 0 && !IsReloading)
		{
			if (UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
			{
				ServerFireRifleAction(PlayerCamera->GetComponentLocation(), PlayerCamera->GetComponentRotation(), true);
			}
			else
			{
				ServerFireRifleAction(PlayerCamera->GetComponentLocation(), PlayerCamera->GetComponentRotation(), false);
			}

			ClientFire();
			ClientRecoil();
			//开启计时器
			//按住开火键后走这个逻辑，rate是每rate秒后执行回调函数这个逻辑，松手就停止开火，就停止计时器。（所以按住不松手，rate越小射速越快）
			if (ServerPrimaryWeapon->IsAutomatic)
			{
				GetWorldTimerManager().SetTimer(AutomaticFireTimerHandle, this, &AMyCharacter::AutomaticFire, ServerPrimaryWeapon->AutomaticFireRate, true);
			}
		}
		
	}
	

}

void AMyCharacter::StopFireWeaponPrimary()
{
	//IsFireing = false;需要在服务器上实现
	ServerStopFiring();
	//关闭计时器
	GetWorldTimerManager().ClearTimer(AutomaticFireTimerHandle);
	//重置相关后坐力
	ResetRecoil();
}



void AMyCharacter::RifeLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);

	FHitResult HitResult;//传入传出参数，告诉碰撞的结果
	if(ServerPrimaryWeapon)
	{
		if(IsMoving)
		{
			FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulluetDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			EndLocation = FVector(Vector.X + RandomX,Vector.Y + RandomY,Vector.Z + RandomZ);
		}
		else
		{
			EndLocation = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulluetDistance;
		}
		
	}
	bool HitSuccess =  UKismetSystemLibrary::LineTraceSingle(GetWorld(),CameraLocation,EndLocation,ETraceTypeQuery::TraceTypeQuery1,false,IgnoreArray,
		EDrawDebugTrace::None,HitResult,true,FLinearColor::Red,FLinearColor::Green,3.f
		);
	//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Health : %s"), *HitResult.Actor->GetName()));
	if(HitSuccess)
	{
		AMyCharacter* HitCharactor = Cast<AMyCharacter>(HitResult.Actor);
		AKataCharacter* HitKata = Cast<AKataCharacter>(HitResult.Actor);
		if(HitCharactor)
		{
			// UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hit Name : %s"), *HitResult.Actor->GetName()));
			DamegePlayer(HitResult.PhysMaterial.Get(),HitResult.Actor.Get(),CameraLocation,HitResult);
		}
		
		else if(HitKata)
		{
			DamegePlayer(HitResult.PhysMaterial.Get(),HitResult.Actor.Get(),CameraLocation,HitResult);
		}
		else
		{
			FRotator XRotator =  UKismetMathLibrary::MakeRotFromX(HitResult.Normal);//这个变换可以让你的物体的前向向量等于法线向量（Normal）
			MultiSpawnBulletDecal(HitResult.Location,XRotator);
		}
		//打到玩家应用伤害
		//打到其他生成弹孔
	}
}

void AMyCharacter::FireWeaponSniper()
{
	//服务端（减少弹药，射线检测（三种），伤害应用，单孔生成）

	
	//客户端：（枪体动画播放，手臂动画，播放声音，应用屏幕抖动，后坐力，枪口的闪光效果）
	//联机系统开发
	if(ServerPrimaryWeapon->ClipCurrentAmmo > 0 && !IsReloading && !IsFireing)
	{
		if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f)
		{
			ServerFireSniperAction(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		}
		else
		{
			ServerFireSniperAction(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);
		}
		
		ClientFire();
		//开启计时器
		//按住开火键后走这个逻辑，rate是每rate秒后执行回调函数这个逻辑，松手就停止开火，就停止计时器。（所以按住不松手，rate越小射速越快）
		
		
		
	}
}

void AMyCharacter::StopFireWeaponSniper()
{
}

void AMyCharacter::SniperLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);

	FHitResult HitResult;//传入传出参数，告诉碰撞的结果
	if(ServerPrimaryWeapon)
	{
		//是否开镜导致不同的射线检测
		if(IsAiming)
		{
			if(IsMoving)
			{
				FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulluetDistance;
				float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
				float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
				float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
				EndLocation = FVector(Vector.X + RandomX,Vector.Y + RandomY,Vector.Z + RandomZ);
			}
			else
			{
				EndLocation = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulluetDistance;
			}
			ClientEndAiming();
		}
		else
		{
			FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulluetDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			EndLocation = FVector(Vector.X + RandomX,Vector.Y + RandomY,Vector.Z + RandomZ);
		}
		
	}
	bool HitSuccess =  UKismetSystemLibrary::LineTraceSingle(GetWorld(),CameraLocation,EndLocation,ETraceTypeQuery::TraceTypeQuery1,false,IgnoreArray,
		EDrawDebugTrace::None,HitResult,true,FLinearColor::Red,FLinearColor::Green,3.f
		);
	//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Health : %s"), *HitResult.Actor->GetName()));
	if(HitSuccess)
	{
		AMyCharacter* HitCharactor = Cast<AMyCharacter>(HitResult.Actor);
		AKataCharacter* HitKata = Cast<AKataCharacter>(HitResult.Actor);
		if(HitCharactor)
		{
			// UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hit Name : %s"), *HitResult.Actor->GetName()));
			DamegePlayer(HitResult.PhysMaterial.Get(),HitResult.Actor.Get(),CameraLocation,HitResult);
		}
		
		else if(HitKata)
		{
			DamegePlayer(HitResult.PhysMaterial.Get(),HitResult.Actor.Get(),CameraLocation,HitResult);
		}
		else
		{
			FRotator XRotator =  UKismetMathLibrary::MakeRotFromX(HitResult.Normal);//这个变换可以让你的物体的前向向量等于法线向量（Normal）
			MultiSpawnBulletDecal(HitResult.Location,XRotator);
		}
		//打到玩家应用伤害
		//打到其他生成弹孔
	}
}

void AMyCharacter::DelaySniperShootCallBack()
{
	IsFireing = false;
}

void AMyCharacter::DamegePlayer(UPhysicalMaterial* PhysicalMaterial,AActor* DamegeActor,FVector& HitDirection,FHitResult& HitInfo)
{
	float Damege = 0;
	if(ServerPrimaryWeapon)
	{
		switch (PhysicalMaterial->SurfaceType)
		{
		case EPhysicalSurface::SurfaceType1:
			{
				//头
				Damege = ServerPrimaryWeapon->BaseDmage * 4;
			}
			break;
		case EPhysicalSurface::SurfaceType2:
			{
				//Body
				Damege = ServerPrimaryWeapon->BaseDmage * 1;
			}
			break;
		case EPhysicalSurface::SurfaceType3:
			{
				//Arm
				Damege = ServerPrimaryWeapon->BaseDmage * 0.8;
			}
			break;
		case EPhysicalSurface::SurfaceType4:
			{
				//Leg
				Damege = ServerPrimaryWeapon->BaseDmage * 0.5;
			}
			break;
		}
		//五个位置不同伤害
	
		UGameplayStatics::ApplyPointDamage(DamegeActor,Damege,HitDirection,HitInfo,
			GetController(),this,UDamageType::StaticClass());//打了别人调这个发通知
		OnTakePointDamage;//给接受伤害的添加回调，自己被打的时候，就会被调用
	}
	}
	

void AMyCharacter::OnHit(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType,
	AActor* DamageCauser)
{
	Health -= Damage;
	UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Health : %f"), Health));
	ClientUpdateHealthUi(Health);
	//更新生命UI 步骤：1、客户端RPC（服务器调用在客户端执行的RPC） 2、1的函数实现 ：调用玩家控制器的更新UI函数 3 2的函数在蓝图实现
	if(Health <= 0)
	{
		//死亡
		DeathMatchDeath(DamageCauser);
		MultiDeathAmim();
	}
	// UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Health : %f"), Health));
}

void AMyCharacter::DeathMatchDeath(AActor* Damager)
{
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPWeaponActor();
	AWeaponBaseClient* CurrentClientWeapon = GetCurrentClientFPArmsWeaponActor();
	if(CurrentServerWeapon)
	{
		CurrentServerWeapon->Destroy();
	}
	if(CurrentClientWeapon)
	{
		CurrentClientWeapon->Destroy();
	}
	ClientDeathMatchDeath();
	
	AMyPlayerController* MultiPlaterController = Cast<AMyPlayerController>(GetController());
	if(MultiPlaterController)
	{
		MultiPlaterController->DeathMatchDeath(Damager);
	}
	
}

#pragma endregion


