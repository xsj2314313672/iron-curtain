// Fill out your copyright notice in the Description page of Project Settings.


#include "PVPHealthComponent.h"
#include "PVPAbilitySystemComponent.h"
#include "Attributes/PVPHealthSet.h"
#include "IronCurtain.h"

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
}

void UPVPHealthComponent::UninitializeFromAbilitySystem()
{
}

float UPVPHealthComponent::GetHealth() const
{
	return 0.f;
}

float UPVPHealthComponent::GetMaxHealth() const
{
	return 0.f;
}

float UPVPHealthComponent::GetHealthNormalized() const
{
	return 0.f;
}

void UPVPHealthComponent::StartDeath()
{
}

void UPVPHealthComponent::FinishDeath()
{
}

void UPVPHealthComponent::HandleHealthChanged(AActor* EffectInstigator, AActor* EffectCauser,
	const FGameplayEffectSpec* EffectSpec, float Magnitude, float OldValue, float NewValue)
{
}

void UPVPHealthComponent::HandleOutOfHealth(AActor* EffectInstigator, AActor* EffectCauser,
	const FGameplayEffectSpec* EffectSpec, float Magnitude, float OldValue, float NewValue)
{
}


