// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PVPCameraPitchComponent.generated.h"

class APlayerCameraManager;
class APlayerController;

/**
 * 专门管理第一人称镜头俯仰（Pitch）的相机管理组件。
 *
 * 引擎里真正生效的俯仰限制是 APlayerCameraManager::ViewPitchMin / ViewPitchMax：
 * APlayerController::UpdateRotation() 每帧都会走 ProcessViewRotation() -> LimitViewPitch()
 * 把控制旋转的 Pitch 夹一次，所以只要把范围写进相机管理器，控制器、鼠标、手柄、
 * 蓝图里的 AddControllerPitchInput 就都会被限制住。
 *
 * 本组件因此做两件事：
 *   1. 把自己配置的俯仰范围同步到所属玩家的相机管理器（默认 -65 ~ 65）；
 *   2. 每帧兜底把控制器当前的 Pitch 夹回范围，其它系统直接 SetControlRotation 也没用。
 *
 * 组件按 Owner 自己找相机管理器：Owner 是 PlayerController 就用它的 PlayerCameraManager，
 * Owner 是 Pawn 就用 Pawn 所属控制器的，Owner 是 APlayerCameraManager 就直接用自己。
 * 推荐挂在 APVPPlayerController 上（每个玩家一份）。
 */
UCLASS(ClassGroup = (PVP), meta = (BlueprintSpawnableComponent, DisplayName = "PVP Camera Pitch Component"))
class IRONCURTAIN_API UPVPCameraPitchComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UPVPCameraPitchComponent();

	/** 俯仰下限（负值 = 低头）。引擎允许到 -89，再大第一人称镜头会翻面 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PVP|Camera", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float PitchMin = -65.0f;

	/** 俯仰上限（正值 = 抬头）。引擎允许到 89 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PVP|Camera", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float PitchMax = 65.0f;

	/** 每帧同步范围并兜底夹控制旋转。关掉后只在 BeginPlay 和调用 SetPitchLimits 时生效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PVP|Camera")
	bool bEnforceEveryTick = true;

	/**
	 * 设置俯仰范围（单位：度），会先夹到合法区间再立即生效：
	 * PitchMin ∈ [-89, 0]、PitchMax ∈ [0, 89]、且 PitchMin <= PitchMax。
	 */
	UFUNCTION(BlueprintCallable, Category = "PVP|Camera")
	void SetPitchLimits(float InPitchMin, float InPitchMax);

	/** 取当前生效的俯仰范围（单位：度） */
	UFUNCTION(BlueprintPure, Category = "PVP|Camera")
	void GetPitchLimits(float& OutPitchMin, float& OutPitchMax) const;

	/** 把当前范围同步到相机管理器（相机管理器被重建、或刚换关时手动补一次） */
	UFUNCTION(BlueprintCallable, Category = "PVP|Camera")
	void ApplyPitchLimits();

	/** 把控制器当前的 Pitch 立刻夹回范围（传送、重生、动画强行转向之后可以用） */
	UFUNCTION(BlueprintCallable, Category = "PVP|Camera")
	void ClampControlRotationPitch();

protected:

	/** Gameplay 初始化：把范围推给相机管理器 */
	virtual void BeginPlay() override;

	/** 每帧兜底：相机管理器对齐 + 控制旋转兜底夹取 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	/** 按 Owner 找到对应的玩家控制器，找不到返回空 */
	APlayerController* FindOwningPlayerController() const;

	/** 按 Owner 找到对应的相机管理器，找不到返回空 */
	APlayerCameraManager* FindOwningCameraManager() const;

	/** 把 PitchMin / PitchMax 夹到安全、合法的区间 */
	void NormalizePitchLimits();
};
