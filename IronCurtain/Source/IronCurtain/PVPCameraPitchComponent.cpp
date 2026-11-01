// Fill out your copyright notice in the Description page of Project Settings.

#include "PVPCameraPitchComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace PVPCameraPitchConstants
{
	/** 第一人称镜头俯仰的硬性安全边界，超出这个范围画面会翻面 */
	constexpr float PitchHardMin = -89.0f;
	constexpr float PitchHardMax = 89.0f;

	/** 小于这个差值就不用再去写控制旋转了（避免每帧无意义的 SetControlRotation） */
	constexpr float PitchTolerance = 0.01f;
}

UPVPCameraPitchComponent::UPVPCameraPitchComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// 只影响本地玩家的镜头，不需要复制
	SetIsReplicatedByDefault(false);
}

void UPVPCameraPitchComponent::BeginPlay()
{
	Super::BeginPlay();

	NormalizePitchLimits();
	ApplyPitchLimits();
	ClampControlRotationPitch();
}

void UPVPCameraPitchComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnforceEveryTick)
	{
		return;
	}

	// 相机管理器可能被重建（换关卡、重新占用 Pawn、切换 ViewTarget），这里顺手对齐一次；
	// 之后再兜底夹一次控制旋转，防止别的系统绕过 AddControllerPitchInput 直接写 ControlRotation。
	ApplyPitchLimits();
	ClampControlRotationPitch();
}

void UPVPCameraPitchComponent::SetPitchLimits(float InPitchMin, float InPitchMax)
{
	PitchMin = InPitchMin;
	PitchMax = InPitchMax;
	NormalizePitchLimits();

	// 立即生效，不用等下一帧
	ApplyPitchLimits();
	ClampControlRotationPitch();
}

void UPVPCameraPitchComponent::GetPitchLimits(float& OutPitchMin, float& OutPitchMax) const
{
	OutPitchMin = PitchMin;
	OutPitchMax = PitchMax;
}

void UPVPCameraPitchComponent::ApplyPitchLimits()
{
	APlayerCameraManager* CameraManager = FindOwningCameraManager();
	if (!CameraManager)
	{
		// Pawn 还没被控制器占用（或相机管理器还没生成）时先跳过，Tick 会再来一次
		return;
	}

	// 每次同步前先校验，蓝图里运行时改过 PitchMin / PitchMax 也不会写出非法值
	NormalizePitchLimits();

	CameraManager->ViewPitchMin = PitchMin;
	CameraManager->ViewPitchMax = PitchMax;
}

void UPVPCameraPitchComponent::ClampControlRotationPitch()
{
	APlayerController* PlayerController = FindOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	const FRotator ControlRotation = PlayerController->GetControlRotation();

	// 控制旋转的 Pitch 是 0 ~ 360 存储的（LimitViewPitch 会做 ClampAxis），
	// 所以必须先 NormalizeAxis 成 -180 ~ 180，否则低头 20 度会被读成 340、再被夹成抬头。
	const float CurrentPitch = FRotator::NormalizeAxis(ControlRotation.Pitch);
	const float ClampedPitch = FMath::Clamp(CurrentPitch, PitchMin, PitchMax);

	if (FMath::IsNearlyEqual(CurrentPitch, ClampedPitch, PVPCameraPitchConstants::PitchTolerance))
	{
		return;
	}

	FRotator ClampedRotation = ControlRotation;
	ClampedRotation.Pitch = ClampedPitch;
	PlayerController->SetControlRotation(ClampedRotation);
}

void UPVPCameraPitchComponent::NormalizePitchLimits()
{
	// 下限只能朝下看，上限只能朝上看，并且下限不能高过上限
	PitchMin = FMath::Clamp(PitchMin, PVPCameraPitchConstants::PitchHardMin, 0.0f);
	PitchMax = FMath::Clamp(FMath::Max(PitchMax, PitchMin), 0.0f, PVPCameraPitchConstants::PitchHardMax);
}

APlayerController* UPVPCameraPitchComponent::FindOwningPlayerController() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// 挂在 PlayerController 上（推荐）
	if (APlayerController* PlayerController = Cast<APlayerController>(Owner))
	{
		return PlayerController;
	}

	// 挂在 Pawn / Character 上
	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}

	// 挂在相机管理器上
	if (APlayerCameraManager* CameraManager = Cast<APlayerCameraManager>(Owner))
	{
		return CameraManager->GetOwningPlayerController();
	}

	return nullptr;
}

APlayerCameraManager* UPVPCameraPitchComponent::FindOwningCameraManager() const
{
	// 直接挂在相机管理器上
	if (APlayerCameraManager* CameraManager = Cast<APlayerCameraManager>(GetOwner()))
	{
		return CameraManager;
	}

	// 挂在 PlayerController / Pawn 上时，通过控制器取
	if (APlayerController* PlayerController = FindOwningPlayerController())
	{
		return PlayerController->PlayerCameraManager;
	}

	return nullptr;
}
