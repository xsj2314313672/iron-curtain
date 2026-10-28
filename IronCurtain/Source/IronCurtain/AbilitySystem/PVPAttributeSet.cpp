// Fill out your copyright notice in the Description page of Project Settings.


#include "PVPAttributeSet.h"
#include "AbilitySystem/PVPAbilitySystemComponent.h"


UPVPAttributeSet::UPVPAttributeSet()
{
}

UWorld* UPVPAttributeSet::GetWorld() const
{
	// 属性集不是 Actor，没有默认世界上下文，要从 OwningActor 取
	const UObject* OwningActor = GetOwningAbilitySystemComponent();
	return OwningActor ? OwningActor->GetWorld() : nullptr;
}

UPVPAbilitySystemComponent* UPVPAttributeSet::GetPVPAbilitySystemComponent() const
{
	return Cast<UPVPAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
