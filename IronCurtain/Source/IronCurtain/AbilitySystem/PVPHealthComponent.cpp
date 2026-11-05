// Fill out your copyright notice in the Description page of Project Settings.


#include "PVPHealthComponent.h"
#include "PVPAbilitySystemComponent.h"
#include "Attributes/PVPHealthSet.h"
#include "IronCurtain.h"
#include "PVPGameplayTags.h"

// Sets default values for this component's properties
UPVPHealthComponent::UPVPHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UPVPHealthComponent::InitializeWithAbilitySystem(UPVPAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	if (!Owner || !InASC)
	{
		UE_LOG(LogIronCurtain,Error,TEXT("[PVPHealthComponent]初始化失败或者ASC为空"));
		return;
	}
	if (AbilitySystemComponent)
	{
		UE_LOG(LogIronCurtain,Warning,TEXT("[PVPHealthComponent] %s 已经初始化过一次，跳过"),*GetNameSafe(Owner));
		return;
	}
	AbilitySystemComponent = InASC;
	HealthSet = AbilitySystemComponent->GetSet<UPVPHealthSet>();
	if (!HealthSet)
	{
		UE_LOG(LogIronCurtain,Error,TEXT("[PVPHealthComponent] %s 的ASC上没有UPVPHealthSet"),*GetNameSafe(Owner));
		return;
	}
	
	HealthSet->OnHealthChangeDelegate.AddUObject(this,&UPVPHealthComponent::HandleHealthChanged);
	HealthSet->OnOutOfHealthDelegate.AddUObject(this,&UPVPHealthComponent::HandleOutOfHealth);
	
	AbilitySystemComponent->SetNumericAttributeBase(UPVPHealthSet::GetHealthAttribute(),HealthSet->GetMaxHealth());
	DeathState = EPVPDeathState::NotDead;
	
	UE_LOG(LogIronCurtain,Log,TEXT("[PVPHealthComponent] %s 初始化完成: %.0f/ %.0f"),
		*GetNameSafe(Owner),HealthSet->GetHealth(),HealthSet->GetMaxHealth());
	OnHealthChanged.Broadcast(HealthSet->GetHealth(),HealthSet->GetMaxHealth(),nullptr);
}

void UPVPHealthComponent::UninitializeFromAbilitySystem()
{
	if (HealthSet)
	{
		HealthSet->OnHealthChangeDelegate.RemoveAll(this);
		HealthSet->OnOutOfHealthDelegate.RemoveAll(this);
	}
	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

float UPVPHealthComponent::GetHealth() const
{
	return HealthSet?HealthSet->GetHealth():0.f;
}

float UPVPHealthComponent::GetMaxHealth() const
{
	return HealthSet?HealthSet->GetMaxHealth():0.f;
}

float UPVPHealthComponent::GetHealthNormalized() const
{
	const float Max = GetMaxHealth();
	return Max>0.f?GetHealth()/Max:0.f;
}

void UPVPHealthComponent::StartDeath()
{
	if (DeathState == EPVPDeathState::NotDead)
	{
		return;
	}
	DeathState = EPVPDeathState::DeathStarted;
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(PVPGameplayTags::Status_Death_Dying);
	}
	UE_LOG(LogIronCurtain,Warning,TEXT("[PVPHealthComponent]的[Death] %s 进入死亡流程"),*GetNameSafe(GetOwner()));
	OnDeathStarted.Broadcast(GetOwner(),LastKiller);
}

void UPVPHealthComponent::FinishDeath()
{
	if (DeathState != EPVPDeathState::DeathStarted)
	{
		return;
	}
	DeathState = EPVPDeathState::DeathFinished;
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(PVPGameplayTags::Status_Death_Dead);
	}
	OnDeathFinished.Broadcast(GetOwner(),LastKiller);
}

void UPVPHealthComponent::HandleHealthChanged(AActor* EffectInstigator, AActor* EffectCauser,
	const FGameplayEffectSpec* EffectSpec, float Magnitude, float OldValue, float NewValue)
{
	if (EffectInstigator)
	{
		LastKiller = EffectInstigator;
	}
	OnHealthChanged.Broadcast(OldValue,NewValue,EffectInstigator);
}

void UPVPHealthComponent::HandleOutOfHealth(AActor* EffectInstigator, AActor* EffectCauser,
	const FGameplayEffectSpec* EffectSpec, float Magnitude, float OldValue, float NewValue)
{
	if (DeathState == EPVPDeathState::NotDead)
	{
		StartDeath();
	}
}


