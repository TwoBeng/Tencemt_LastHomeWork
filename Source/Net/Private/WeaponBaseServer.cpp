// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBaseServer.h"

#include "MyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AWeaponBaseServer::AWeaponBaseServer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	if(SphereCollision)
	{
		SphereCollision->SetupAttachment(RootComponent);
	}
	
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);//设置碰撞查询  且物体
	WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);

	WeaponMesh->SetOwnerNoSee(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetSimulatePhysics(true);

	SphereCollision->OnComponentBeginOverlap.AddDynamic(this,&AWeaponBaseServer::OnOtherBeginOverlap);//事件发生就会调用OnOtherBeginOverlap这个方法

	// SetReplicates(true);//在服务器生成后会复制一份给客户端。
	bReplicates = true;
}


void AWeaponBaseServer::OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMyCharacter* TPCharacter = Cast<AMyCharacter>(OtherActor);
	if(TPCharacter)//如果转变类型成功
	{
		//玩家逻辑
		//ServerPickWeapon(OtherActor);
		EquipWeapon();
		TPCharacter->EquipPrimary(this);
	}
}

void AWeaponBaseServer::EquipWeapon()
{
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetSimulatePhysics(false);
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeaponBaseServer::MultiShootingEffect_Implementation()
{
	if(GetOwner() != UGameplayStatics::GetPlayerPawn(GetWorld(),0))
	{
		FName AttachPointName = TEXT("Fire_FX_Slot");
		if(KindofWeapon == EWeaponType::M4A1)
		{
			AttachPointName = TEXT("MuzzleSocket");
		}
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlash,WeaponMesh,AttachPointName,
		FVector::ZeroVector,FRotator::ZeroRotator,FVector::OneVector,
		EAttachLocation::KeepRelativeOffset,true,
		EPSCPoolMethod::None,true);

		UGameplayStatics::PlaySoundAtLocation(GetWorld(),FireSound,GetActorLocation());
	}
}

bool AWeaponBaseServer::MultiShootingEffect_Validate()
{
	return true;
}

void AWeaponBaseServer::ServerPickWeapon_Implementation(AActor* TPCharacter)
{
	AMyCharacter* MyCharacter = Cast<AMyCharacter>(TPCharacter);
	if(MyCharacter)//如果转变类型成功
		{
		//玩家逻辑
		EquipWeapon();
		MyCharacter->EquipPrimary(this);
		}
	
}

bool AWeaponBaseServer::ServerPickWeapon_Validate(AActor* TPCharacter)
{
	return true;
}

// Called when the game starts or when spawned
void AWeaponBaseServer::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponBaseServer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void AWeaponBaseServer:: GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const//重载父类函数  让子弹数也能replicate到客户端
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);//调用父类的方法，避免崩溃  有的会用到
	DOREPLIFETIME_CONDITION(AWeaponBaseServer,ClipCurrentAmmo,COND_None);
}

