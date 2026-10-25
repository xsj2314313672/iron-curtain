// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "PVPCameraManager.generated.h"

/**
 * PvP 模式统一的玩家相机管理器。
 *
 * 把第一人称视角的俯仰限制收在这里，角色蓝图只需要开启 bUsePawnControlRotation，
 * 不用每个角色各写一份限制逻辑。
 */
UCLASS(meta = (DisplayName = "PVP Camera Manager"))
class IRONCURTAIN_API APVPCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:

	/** Constructor */
	APVPCameraManager();

	/**
	 * 调整俯仰限制（单位：度）。
	 * 会先夹到 -89 ~ 89 的安全范围内，避免仰角过大触发万向锁导致画面翻转。
	 */
	UFUNCTION(BlueprintCallable, Category = "PVP|Camera")
	void SetPitchLimits(float InPitchMin, float InPitchMax);

	/** 取当前生效的俯仰限制（单位：度） */
	UFUNCTION(BlueprintPure, Category = "PVP|Camera")
	void GetPitchLimits(float& OutPitchMin, float& OutPitchMax) const;
};
