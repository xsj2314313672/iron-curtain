// Fill out your copyright notice in the Description page of Project Settings.

#include "Shooter/PVPWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

APVPWeapon::APVPWeapon()
{
	// 武器本身不需要 Tick：单发靠事件，全自动由角色在 Tick 里反复调 Fire()，内部自己限速
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
	SetRootComponent(WeaponMesh);

	// 武器只负责视觉表现，不参与碰撞查询（命中判定走射线）
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetCanEverAffectNavigation(false);
}

void APVPWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 第一人称武器：只有自己看得到，并使用 FirstPerson 渲染参数，避免贴脸穿模
	if (bFirstPersonOnly)
	{
		WeaponMesh->SetOnlyOwnerSee(true);
		WeaponMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	}

	// 开局补满弹药，并且让第一发不用等冷却
	CurrentAmmo = MaxAmmo;
	LastFireTime = -GetFireInterval();
}

bool APVPWeapon::Fire()
{
	if (!CanFire())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	LastFireTime = World ? World->GetTimeSeconds() : 0.0f;

	if (!bInfiniteAmmo)
	{
		--CurrentAmmo;
	}

	PerformTraceShot();

	// 蓝图钩子：枪口火光、音效、后坐力、开火动画
	OnFired();

	return true;
}

void APVPWeapon::Reload()
{
	// 最简单的一版：直接补满。换弹时间、动画和音效交给蓝图或后续实现
	CurrentAmmo = MaxAmmo;
}

bool APVPWeapon::CanFire() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (!bInfiniteAmmo && CurrentAmmo <= 0)
	{
		return false;
	}

	return (World->GetTimeSeconds() - LastFireTime) >= GetFireInterval();
}

float APVPWeapon::GetFireInterval() const
{
	// 600 发 / 分钟 = 0.1 秒一发；除零保护
	return 60.0f / FMath::Max(RoundsPerMinute, 1.0f);
}

void APVPWeapon::SetOwningPawn(APawn* InPawn)
{
	OwningPawn = InPawn;

	// 伤害归属、只给 Owner 看（bFirstPersonOnly）都依赖这两个
	SetOwner(InPawn);
	SetInstigator(InPawn);
}

FTransform APVPWeapon::GetMuzzleTransform() const
{
	if (WeaponMesh && MuzzleSocketName != NAME_None && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh->GetSocketTransform(MuzzleSocketName, RTS_World);
	}

	return WeaponMesh ? WeaponMesh->GetComponentTransform() : GetActorTransform();
}

void APVPWeapon::GetTraceStartAndDirection(FVector& OutStartLocation, FVector& OutDirection) const
{
	// 没有持有人时只能从枪口打
	if (bUseMuzzleAsTraceStart || !OwningPawn)
	{
		const FTransform MuzzleTransform = GetMuzzleTransform();
		OutStartLocation = MuzzleTransform.GetLocation();
		OutDirection = MuzzleTransform.GetRotation().Vector();
		return;
	}

	// 从持有人视线发射：第一人称下弹道才和准星一致
	OutStartLocation = OwningPawn->GetPawnViewLocation();

	if (const AController* OwningController = OwningPawn->GetController())
	{
		OutDirection = OwningController->GetControlRotation().Vector();
	}
	else
	{
		OutDirection = OwningPawn->GetBaseAimRotation().Vector();
	}
}

void APVPWeapon::PerformTraceShot()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector TraceStart;
	FVector TraceDirection;
	GetTraceStartAndDirection(TraceStart, TraceDirection);

	const FVector TraceEnd = TraceStart + TraceDirection * TraceDistance;

	// 不打自己、也不打持有人（否则射线一出枪就撞在自己身上）
	FCollisionQueryParams QueryParams(FName(TEXT("PVPWeaponTrace")), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(this);
	if (OwningPawn)
	{
		QueryParams.AddIgnoredActor(OwningPawn);
	}
	else if (const AActor* OwnerActor = GetOwner())
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannel, QueryParams);

	if (AActor* HitActor = Hit.GetActor())
	{
		AController* InstigatorController = OwningPawn ? OwningPawn->GetController() : GetInstigatorController();

		// 伤害还没接 GAS，走引擎原生 Damage；以后换成 GameplayEffect 只改这几行
		UGameplayStatics::ApplyPointDamage(HitActor, BaseDamage, TraceDirection, Hit, InstigatorController, this, DamageTypeClass);

		OnWeaponHit(Hit);
	}

	if (bDrawDebugTrace)
	{
		const FColor TraceColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(World, TraceStart, TraceEnd, TraceColor, false, 1.0f, 0, 1.5f);

		if (bHit)
		{
			DrawDebugPoint(World, Hit.ImpactPoint, 8.0f, TraceColor, false, 1.0f);
		}
	}
}
