// Fill out your copyright notice in the Description page of Project Settings.

#include "Shooter/PVPShooterPlayerController.h"
#include "Shooter/PVPShooterCharacter.h"
#include "IronCurtain.h"
#include "AbilitySystem/PVPAbilitySystemComponent.h"
#include "AbilitySystem/PVPGameplayTags.h"
#include "GameplayEffect.h"

APVPShooterPlayerController::APVPShooterPlayerController()
{
	// 相机管理器与输入映射沿用 APVPPlayerController，这里不再重复指定
}

void APVPShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledShooterCharacter = Cast<APVPShooterCharacter>(InPawn);

	if (!ControlledShooterCharacter)
	{
		// 观察者、载具等非射击角色时属于正常情况，所以只记 Warning 不中断流程
		UE_LOG(LogIronCurtain, Warning, TEXT("'%s' 占用的 Pawn '%s' 不是 APVPShooterCharacter，射击相关功能不可用。"),
			*GetNameSafe(this),
			*GetNameSafe(InPawn));
	}
}

void APVPShooterPlayerController::OnUnPossess()
{
	ControlledShooterCharacter = nullptr;

	Super::OnUnPossess();
}

void APVPShooterPlayerController::PVPDamageSelf(float DamageAmount)
{
	APVPShooterCharacter* ShooterCharacter = GetShooterCharacter();
	if (!ShooterCharacter)
	{
		UE_LOG(LogIronCurtain, Error, TEXT("PVPDamageSelf: 没有角色"));
		return;
	}
	UAbilitySystemComponent *ASC = ShooterCharacter->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogIronCurtain, Error, TEXT("PVPDamageSelf: 没有ASC"));
		return;
	}
	// 运行期按路径加载，成功后缓存在 UPROPERTY 成员里，后续调用直接复用
	if (!DamageEffectClass)
	{
		DamageEffectClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/IronCurtain/GAS/GE/GE_Damage.GE_Damage_C"));

		if (!DamageEffectClass)
		{
			UE_LOG(LogIronCurtain, Error,TEXT("PVPDamageSelf: 加载 GE_Damage 失败（/Game/IronCurtain/GAS/GE/GE_Damage）"));
			return;
		}
	}
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddInstigator(ShooterCharacter,ShooterCharacter);
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageEffectClass,1.f,Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}
	SpecHandle.Data->SetSetByCallerMagnitude(PVPGameplayTags::SetByCaller_Damage,DamageAmount);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	UE_LOG(LogIronCurtain, Log, TEXT("PVPDamageSelf: 自伤 %.1f"), DamageAmount);
}
