// Fill out your copyright notice in the Description page of Project Settings.

#include "PVPPlayerController.h"
#include "PVPCameraManager.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "IronCurtain.h"

APVPPlayerController::APVPPlayerController()
{
	// 用本项目的相机管理器替代引擎默认的 APlayerCameraManager
	PlayerCameraManagerClass = APVPCameraManager::StaticClass();

	// PvP 全程锁定在游戏输入模式，不显示系统鼠标
	bShowMouseCursor = false;

	// 默认输入映射：蓝图子类里可以整体覆盖这个数组
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultMappingContextAsset(TEXT("/Game/IronCurtain/Input/InputMappingContext"));
	if (DefaultMappingContextAsset.Succeeded())
	{
		DefaultMappingContexts.Add(DefaultMappingContextAsset.Object);
	}
	else
	{
		UE_LOG(LogIronCurtain, Error, TEXT("没有找到默认输入映射 /Game/IronCurtain/Input/InputMappingContext，按键将不会生效。"));
	}
}

void APVPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 专用服务器上不存在本地玩家，不需要处理输入模式
	if (!IsLocalPlayerController())
	{
		return;
	}

	// 占用角色后保持 GameOnly，避免 UI 抢走视角输入
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void APVPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!InputSubsystem)
	{
		UE_LOG(LogIronCurtain, Error, TEXT("'%s' 没有取到 EnhancedInput 子系统，输入映射未注册。"), *GetNameSafe(this));
		return;
	}

	for (const TObjectPtr<UInputMappingContext>& MappingContext : DefaultMappingContexts)
	{
		if (MappingContext)
		{
			InputSubsystem->AddMappingContext(MappingContext, InputMappingPriority);
		}
	}

	if (DefaultMappingContexts.Num() == 0)
	{
		// 最常见的就是这里：没有任何 IMC，角色的 Action 绑定再正确也不会触发
		UE_LOG(LogIronCurtain, Error, TEXT("'%s' 没有配置任何输入映射上下文（DefaultMappingContexts 为空），按键不会生效。"), *GetNameSafe(this));
	}
}

APVPCameraManager* APVPPlayerController::GetPVPCameraManager() const
{
	return Cast<APVPCameraManager>(PlayerCameraManager);
}
