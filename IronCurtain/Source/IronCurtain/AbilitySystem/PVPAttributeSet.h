// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "PVPAttributeSet.generated.h"

class UPVPAbilitySystemComponent;
struct FGameplayEffectSpec;

/**
 * 属性变化事件（照搬 Lyra 的 FLyraAttributeEvent）
 * EffectInstigator : 发起者（开枪的人）
 * EffectCauser     : 直接来源（枪）
 * EffectSpec       : 完整 GE 上下文。用指针而不是引用，因为有些属性变化
 *                    不是 GE 引起的（直接 SetBaseValue），那时它是 nullptr
 * EffectMagnitude  : 未夹取的原始数值
 * OldValue/NewValue: 变化前后
 */
DECLARE_MULTICAST_DELEGATE_SixParams(FPVPAttributeEvent,
	AActor*,
	AActor*,
	const FGameplayEffectSpec*,
	float,
	float,
	float);

/**
 * 
 */
UCLASS()
class IRONCURTAIN_API UPVPAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UPVPAttributeSet();
	
	virtual UWorld* GetWorld() const override;
	
	UPVPAbilitySystemComponent* GetPVPAbilitySystemComponent() const;
};
