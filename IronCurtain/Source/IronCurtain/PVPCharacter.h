// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PVPCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USkeletalMeshComponent;
struct FInputActionValue;


UCLASS()
class IRONCURTAIN_API APVPCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APVPCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/**
	 * 客户端接过控制权后调用。用来把本地玩家自己的头部几何从第一人称里去掉。
	 * 这是引擎里唯一“确定已经本地占有”的可靠时机，比 BeginPlay 稳。
	 */
	virtual void PawnClientRestart() override;

	/**
	 * 第一人称可见性：本地玩家隐藏 head 骨骼，其他玩家照常显示。
	 * 幂等，可反复调用（LOD 切换会重建 BoneVisibilityStates，所以用一个低频定时器自愈）。
	 */
	void UpdateFirstPersonHeadVisibility();

	/**
	 * 配置“只投影不显示”的全身副本。
	 * @param bLocalOwner 当前这具是不是本地玩家自己操控的
	 */
	void ConfigureShadowBody(bool bLocalOwner);

	FTimerHandle HeadVisibilityTimer;

public:	

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronCurtain|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> CameraComponent;

	/**
	 * 只投射阴影的全身副本（和主网格用同一份 SkeletalMesh）。
	 *
	 * 由来：本地玩家隐藏 head 骨骼后，影子会变成“无头”。因为骨骼可见性是在蒙皮阶段
	 * 生效的，阴影通道用的是同一批骨骼变换，所以头会一起从影子里消失。
	 *
	 * 这一具永远不参与主渲染（bRenderInMainPass = false），只进阴影通道；骨骼由主网格
	 * 通过 LeaderPose 驱动，等于零动画开销。屏幕上依然没有头，影子里头是完整的。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IronCurtain|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> ShadowBody;
	
protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "IronCurtain|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "IronCurtain|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "IronCurtain|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "IronCurtain|Input")
	TObjectPtr<UInputAction> JumpAction;


};
