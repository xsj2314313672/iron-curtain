// Fill out your copyright notice in the Description page of Project Settings.


#include "PVPHealthSet.h"

#include "GameplayEffectExtension.h"
#include "IronCurtain.h"

UPVPHealthSet::UPVPHealthSet():Health(100.f),MaxHealth(100.f)
{
	bOutOfHealth = false;
}

void UPVPHealthSet::ResetOutOfHealthFlag() const
{
	// 只复位闩锁，不动血量；血量由调用方通过 GE 或 SetNumericAttributeBase 设置
	bOutOfHealth = false;
}

void UPVPHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UPVPHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UPVPHealthSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* EffectInstigator = EffectContext.GetOriginalInstigator();
	AActor* EffectCauser = EffectContext.GetEffectCauser();
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		HealthBeforeAttributeChange = GetHealth();

		// 取完立刻清零：Damage 是元属性，只在本次 GE 执行期间有意义。
		// 不清零的话，下一次 GE 若用 Add 方式修改 Damage，会叠加上次的残留值。
		const float IncomingDamage = GetDamage();
		SetDamage(0.f);

		SetHealth(FMath::Clamp(HealthBeforeAttributeChange - IncomingDamage, 0.f, GetMaxHealth()));
		UE_LOG(LogIronCurtain, Log, TEXT("[GAS] %s 受到 %.1f 伤害 → %.1f / %.1f（发起者 %s）"),
			*GetNameSafe(GetOwningActor()), IncomingDamage,
			GetHealth(), GetMaxHealth(), *GetNameSafe(EffectInstigator));

		// 委托第三个参数是 const FGameplayEffectSpec*，这里取 Data.EffectSpec 的地址
		OnHealthChangeDelegate.Broadcast(EffectInstigator, EffectCauser, &Data.EffectSpec,
			Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
	}else if(Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		HealthBeforeAttributeChange = GetHealth();
		SetHealth(FMath::Clamp(GetHealth(),0.f,GetMaxHealth()));
		OnHealthChangeDelegate.Broadcast(EffectInstigator,EffectCauser,&Data.EffectSpec,Data.EvaluatedData.Magnitude,HealthBeforeAttributeChange,GetHealth());
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		OnMaxHealthChangeDelegate.Broadcast(EffectInstigator,EffectCauser,&Data.EffectSpec,Data.EvaluatedData.Magnitude,GetMaxHealth(),GetMaxHealth());
	}
	
	if (GetHealth()<=0.f && !bOutOfHealth)
	{
		UE_LOG(LogIronCurtain, Warning, TEXT("[GAS] %s 血量归零（凶手 %s）"),
			*GetNameSafe(GetOwningActor()), *GetNameSafe(EffectInstigator));
		OnOutOfHealthDelegate.Broadcast(EffectInstigator, EffectCauser, &Data.EffectSpec,
			Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
	}
	bOutOfHealth = (GetHealth() <= 0.f);
}

void UPVPHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& ClampedValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		ClampedValue = FMath::Clamp(ClampedValue, 0.f,GetMaxHealth());
	}else if (Attribute == GetMaxHealthAttribute())
	{
		ClampedValue = FMath::Max(ClampedValue,1.f);
	}
}
