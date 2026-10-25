// Fill out your copyright notice in the Description page of Project Settings.

#include "PVPGameMode.h"
#include "PVPCharacter.h"
#include "PVPPlayerController.h"
#include "IronCurtain.h"

APVPGameMode::APVPGameMode()
{
	// 玩家控制器：负责相机管理器与输入映射
	PlayerControllerClass = APVPPlayerController::StaticClass();

	// 默认角色：没有玩法扩展时使用基础第一人称角色
	DefaultPawnClass = APVPCharacter::StaticClass();
}

void APVPGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 方便在日志里确认当前地图到底跑的是哪个 GameMode / 控制器 / 角色
	UE_LOG(LogIronCurtain, Log, TEXT("PvP GameMode 已激活：PlayerController=%s，DefaultPawn=%s"),
		*GetNameSafe(PlayerControllerClass),
		*GetNameSafe(DefaultPawnClass));
}
