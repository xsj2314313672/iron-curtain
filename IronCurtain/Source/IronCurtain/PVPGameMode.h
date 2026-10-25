// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PVPGameMode.generated.h"

/**
 * PvP 模式的基础 GameMode。
 *
 * 只负责把玩家控制器和默认角色指到本项目的 C++ 基类；
 * 具体的玩法规则（回合、计分、复活）放到派生类里实现，例如 Shooter/PVPShooterGameMode。
 */
UCLASS()
class IRONCURTAIN_API APVPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	APVPGameMode();

protected:

	/** Gameplay 初始化 */
	virtual void BeginPlay() override;
};
