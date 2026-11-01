// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PVPPlayerController.generated.h"

class APVPCameraManager;
class UInputMappingContext;
class UPVPCameraPitchComponent;

/**
 * PvP 模式的基础玩家控制器。
 *
 * 职责：
 *  - 把 PlayerCameraManagerClass 指到本项目的 APVPCameraManager；
 *  - 给本地玩家注册增强输入（Enhanced Input）映射上下文；
 *  - 保证本地玩家始终处于 GameOnly 输入模式。
 *
 * 玩法相关的输入 / UI 交给子类（例如 Shooter 下的 APVPShooterPlayerController）。
 */
UCLASS()
class IRONCURTAIN_API APVPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	/** Constructor */
	APVPPlayerController();

	/** 取当前正在使用的 PvP 相机管理器，可能为空 */
	UFUNCTION(BlueprintPure, Category = "PVP|Camera")
	APVPCameraManager* GetPVPCameraManager() const;

	/** 取相机管理组件（俯仰范围由它统一管理），一定有效 */
	UFUNCTION(BlueprintPure, Category = "PVP|Camera")
	UPVPCameraPitchComponent* GetCameraPitchComponent() const;

protected:

	/** 相机管理组件：统一管理第一人称镜头的俯仰范围（默认 -65 ~ 65） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PVP|Camera")
	UPVPCameraPitchComponent* CameraPitchComponent;

	/** Gameplay 初始化 */
	virtual void BeginPlay() override;

	/** 注册输入映射 */
	virtual void SetupInputComponent() override;

	/** 本地玩家要注册的输入映射上下文 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PVP|Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	/** 输入映射上下文的优先级，数值越大优先级越高 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PVP|Input", meta = (ClampMin = "0"))
	int32 InputMappingPriority = 0;
};
