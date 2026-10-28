// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/PVPAttributeSet.h"
#include "PVPHealthSet.generated.h"

struct FGameplayEffectModCallbackData;

/**
 * 
 */
UCLASS(BlueprintType)
class IRONCURTAIN_API UPVPHealthSet : public UPVPAttributeSet
{
	GENERATED_BODY()
	
public:
	UPVPHealthSet();
	
	ATTRIBUTE_ACCESSORS_BASIC(UPVPHealthSet,Health);
	ATTRIBUTE_ACCESSORS_BASIC(UPVPHealthSet,MaxHealth);
	ATTRIBUTE_ACCESSORS_BASIC(UPVPHealthSet,Damage);
	
	mutable FPVPAttributeEvent OnHealthChangeDelegate;
	mutable FPVPAttributeEvent OnMaxHealthChangeDelegate;
	mutable FPVPAttributeEvent OnOutOfHealthDelegate;
protected:
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	void ClampAttribute(const FGameplayAttribute& Attribute,float& ClampedValue) const;
private:
	UPROPERTY(BlueprintReadOnly,Category="PVP|Health",meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Health;
	UPROPERTY(BlueprintReadOnly,Category="PVP|Health",meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MaxHealth;
	UPROPERTY(BlueprintReadOnly,Category="PVP|Health",meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Damage;
	
	bool bOutOfHealth = false;
	float HealthBeforeAttributeChange = 0.0f;
};
