// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PVPGameMode.h"
#include "PVPShooterGameMode.generated.h"

/**
 * 射击玩法的 PvP GameMode。
 *
 * 在基础 PvP 模式之上，把默认角色和玩家控制器换成射击版本；
 * 回合、计分等射击专属规则后续在这里扩展。
 */
UCLASS()
class IRONCURTAIN_API APVPShooterGameMode : public APVPGameMode
{
	GENERATED_BODY()

public:

	/** Constructor */
	APVPShooterGameMode();
};
