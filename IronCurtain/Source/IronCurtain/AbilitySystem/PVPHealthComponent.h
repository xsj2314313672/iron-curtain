// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PVPHealthComponent.generated.h"

class UPVPAbilitySystemComponent;
class UPVPHealthSet;
struct FGameplayEffectSpec;

UENUM(BlueprintType)
enum class EPVPDeathState:uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPVPHealthChangedDelegate,float,OldValue,float,NewValue,AActor*,Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPVPDeathDelegate,AActor*,OwningActor,AActor*,Killer);

UCLASS(ClassGroup=(IronCurtain), meta=(BlueprintSpawnableComponent))
class IRONCURTAIN_API UPVPHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPVPHealthComponent();
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	void InitializeWithAbilitySystem(UPVPAbilitySystemComponent* InASC);
	void UninitializeFromAbilitySystem();
	
	UFUNCTION(BlueprintPure, Category = "PVP|Health") float GetHealth() const;
	UFUNCTION(BlueprintPure, Category = "PVP|Health") float GetMaxHealth() const;
	UFUNCTION(BlueprintPure, Category = "PVP|Health") float GetHealthNormalized() const;
	UFUNCTION(BlueprintPure, Category = "PVP|Health") EPVPDeathState GetDeathState() const { return DeathState; }
	UFUNCTION(BlueprintPure, Category = "PVP|Health") bool IsDeadOrDying() const { return DeathState != EPVPDeathState::NotDead; }
	/** 开始死亡流程（可被蓝图/作弊命令直接调用） */
	UFUNCTION(BlueprintCallable, Category = "PVP|Health") void StartDeath();
	/** 结束死亡流程（StartDeath 后 DeathFinishDelay 秒自动调用；也可被蓝图/作弊命令手动提前调用） */
	UFUNCTION(BlueprintCallable, Category = "PVP|Health") void FinishDeath();
	
	/** Dying → Dead 的固定延迟（秒）。默认 3 秒，可在角色蓝图的组件面板里调整；配 0 则立即结算 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PVP|Death", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DeathFinishDelay = 3.0f;
	
	UPROPERTY(BlueprintAssignable, Category = "PVP|Health")
	FPVPHealthChangedDelegate OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "PVP|Health")
	FPVPDeathDelegate OnDeathStarted;

	UPROPERTY(BlueprintAssignable, Category = "PVP|Health")
	FPVPDeathDelegate OnDeathFinished;
	
protected:
	void HandleHealthChanged(AActor* EffectInstigator,AActor* EffectCauser,const FGameplayEffectSpec* EffectSpec,float Magnitude,float OldValue,float NewValue);
	void HandleOutOfHealth(AActor* EffectInstigator,AActor* EffectCauser,const FGameplayEffectSpec* EffectSpec,float Magnitude,float OldValue,float NewValue);
	
	/** 改状态的唯一入口：写 DeathState + 立刻把标签刷成一致 */
	void SetDeathStateInternal(EPVPDeathState NewState);
	/** 把 ASC 上的 Dying / Dead 刷成与 InState 严格一致（一次写死两个 count，天然互斥）；幂等 */
	void ApplyDeathTags(EPVPDeathState InState);
	/** 排"Dying → Dead"的定时器 */
	void ScheduleFinishDeath();
	/** 清"Dying → Dead"的定时器（EndPlay / Uninitialize / FinishDeath 都会调） */
	void ClearDeathTimer();

	UPROPERTY()
	TObjectPtr<UPVPAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<const UPVPHealthSet> HealthSet;
	
	UPROPERTY()
	TObjectPtr<AActor> LastKiller;
	
	/** 死亡状态（单机唯一真源）：Dying / Dead 两个 loose tag 只是它的投影 */
	EPVPDeathState DeathState = EPVPDeathState::NotDead;
	
	/** Dying → Dead 定时器句柄 */
	FTimerHandle DeathFinishTimerHandle;
	
};
