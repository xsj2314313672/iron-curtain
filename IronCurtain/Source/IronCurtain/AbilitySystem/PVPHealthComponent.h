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
	
	void InitializeWithAbilitySystem(UPVPAbilitySystemComponent* InASC);
	void UninitializeFromAbilitySystem();
	
	UFUNCTION(BlueprintPure, Category = "PVP|Health") float GetHealth() const;
	UFUNCTION(BlueprintPure, Category = "PVP|Health") float GetMaxHealth() const;
	UFUNCTION(BlueprintPure, Category = "PVP|Health") float GetHealthNormalized() const;
	UFUNCTION(BlueprintPure, Category = "PVP|Health") EPVPDeathState GetDeathState() const { return DeathState; }
	UFUNCTION(BlueprintPure, Category = "PVP|Health") bool IsDeadOrDying() const { return DeathState != EPVPDeathState::NotDead; }
	/** 开始死亡流程（可被蓝图/作弊命令直接调用） */
	UFUNCTION(BlueprintCallable, Category = "PVP|Health") void StartDeath();
	/** 结束死亡流程 */
	UFUNCTION(BlueprintCallable, Category = "PVP|Health") void FinishDeath();
	UPROPERTY(BlueprintAssignable, Category = "PVP|Health")
	FPVPHealthChangedDelegate OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "PVP|Health")
	FPVPDeathDelegate OnDeathStarted;

	UPROPERTY(BlueprintAssignable, Category = "PVP|Health")
	FPVPDeathDelegate OnDeathFinished;
	
protected:
	void HandleHealthChanged(AActor* EffectInstigator,AActor* EffectCauser,const FGameplayEffectSpec* EffectSpec,float Magnitude,float OldValue,float NewValue);
	void HandleOutOfHealth(AActor* EffectInstigator,AActor* EffectCauser,const FGameplayEffectSpec* EffectSpec,float Magnitude,float OldValue,float NewValue);

	UPROPERTY()
	TObjectPtr<UPVPAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<const UPVPHealthSet> HealthSet;
	
	UPROPERTY()
	TObjectPtr<AActor> LastKiller;
	
	EPVPDeathState DeathState = EPVPDeathState::NotDead;
	
};
