// Fill out your copyright notice in the Description page of Project Settings.

#include "Shooter/PVPShooterPlayerController.h"
#include "Shooter/PVPShooterCharacter.h"
#include "IronCurtain.h"

APVPShooterPlayerController::APVPShooterPlayerController()
{
	// 相机管理器与输入映射沿用 APVPPlayerController，这里不再重复指定
}

void APVPShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledShooterCharacter = Cast<APVPShooterCharacter>(InPawn);

	if (!ControlledShooterCharacter)
	{
		// 观察者、载具等非射击角色时属于正常情况，所以只记 Warning 不中断流程
		UE_LOG(LogIronCurtain, Warning, TEXT("'%s' 占用的 Pawn '%s' 不是 APVPShooterCharacter，射击相关功能不可用。"),
			*GetNameSafe(this),
			*GetNameSafe(InPawn));
	}
}

void APVPShooterPlayerController::OnUnPossess()
{
	ControlledShooterCharacter = nullptr;

	Super::OnUnPossess();
}
