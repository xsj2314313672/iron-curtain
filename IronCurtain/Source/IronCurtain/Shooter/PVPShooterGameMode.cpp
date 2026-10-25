// Fill out your copyright notice in the Description page of Project Settings.

#include "Shooter/PVPShooterGameMode.h"
#include "Shooter/PVPShooterCharacter.h"
#include "Shooter/PVPShooterPlayerController.h"

APVPShooterGameMode::APVPShooterGameMode()
{
	// 射击玩法专用的玩家控制器
	PlayerControllerClass = APVPShooterPlayerController::StaticClass();

	// 射击玩法专用的角色
	DefaultPawnClass = APVPShooterCharacter::StaticClass();
}
