// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PVPPlayerController.h"
#include "PVPShooterPlayerController.generated.h"

class APVPShooterCharacter;

/**
 * 射击玩法的玩家控制器。
 *
 * 在基础 PvP 控制器（相机管理器 + 输入映射 + 输入模式）之上，
 * 缓存当前占用的射击角色，方便 HUD / 武器系统直接取用。
 */
UCLASS()
class IRONCURTAIN_API APVPShooterPlayerController : public APVPPlayerController
{
	GENERATED_BODY()

public:

	/** Constructor */
	APVPShooterPlayerController();

	/** 当前占用的射击角色，未占用或占用的不是射击角色时为空 */
	UFUNCTION(BlueprintPure, Category = "PVP|Shooter")
	APVPShooterCharacter* GetShooterCharacter() const { return ControlledShooterCharacter; }

protected:

	/** 占用 Pawn 时缓存射击角色 */
	virtual void OnPossess(APawn* InPawn) override;

	/** 解除占用时清空缓存 */
	virtual void OnUnPossess() override;

	/** 缓存的射击角色 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "PVP|Shooter")
	TObjectPtr<APVPShooterCharacter> ControlledShooterCharacter;
};
