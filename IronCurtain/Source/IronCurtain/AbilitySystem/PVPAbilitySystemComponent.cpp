// Fill out your copyright notice in the Description page of Project Settings.


#include "PVPAbilitySystemComponent.h"


// Sets default values for this component's properties
UPVPAbilitySystemComponent::UPVPAbilitySystemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

void UPVPAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
		return;
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
		}
	}
}

void UPVPAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
		return;
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.Remove(AbilitySpec.Handle);
		}
	}
}

void UPVPAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();
	
	for (const FGameplayAbilitySpecHandle& Handle:InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle))
		{
			if (!Spec->Ability)continue;
			Spec->InputPressed = true;
			if (Spec->IsActive())
			{
				AbilitySpecInputPressed(*Spec);
			}
			else
			{
				AbilitiesToActivate.AddUnique(Spec->Handle);
			}
		}
	}
	
	for (const FGameplayAbilitySpecHandle& Handle:AbilitiesToActivate)
	{
		TryActivateAbility(Handle);
	}
	
	for (const FGameplayAbilitySpecHandle& Handle:InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle))
		{
			if (!Spec->Ability)continue;
			Spec->InputPressed = false;
			if (Spec->IsActive())
			{
				AbilitySpecInputReleased(*Spec);
			}
		}
	}
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UPVPAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UPVPAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	// UE 5.5 起 FGameplayAbilitySpec::ActivationInfo 已废弃（只对废弃的 NonInstanced 有意义）。
	// 输入派发本身完全可以交给 Super：它会按实例逐个调用 InputPressed
	// （InstancedPerActor 走主实例，InstancedPerExecution 走全部实例），
	// 所以这里不再手动转发一次，避免同一个实例被调用两次。
	Super::AbilitySpecInputPressed(Spec);

	// 以后若要在这里广播「输入按下」的预测事件，请取实例自己的 ActivationInfo：
	//   if (const UGameplayAbility* Instance = Spec.GetPrimaryInstance())
	//   {
	//       const FGameplayAbilityActivationInfo ActivationInfo = Instance->GetCurrentActivationInfo();
	//   }
}

void UPVPAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	// 同上：Super 已经按实例转发 InputReleased。
	Super::AbilitySpecInputReleased(Spec);
}




