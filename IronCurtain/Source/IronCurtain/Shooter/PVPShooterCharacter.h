// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "PVPCharacter.h"
#include "AbilitySystem/PVPCombatantInterface.h"
#include "PVPShooterCharacter.generated.h"

class UPVPHealthComponent;
class UPVPHealthSet;
class UPVPAbilitySystemComponent;
class UGameplayAbility;

USTRUCT(BlueprintType)
struct FPVPAbilityInputBinding
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category="Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category = "Ability")
	FGameplayTag InputTag;
};



UCLASS()
class IRONCURTAIN_API APVPShooterCharacter : public APVPCharacter,public IAbilitySystemInterface,public IPVPCombatantInterface
{
	GENERATED_BODY()
	
public:
	APVPShooterCharacter();
	
	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// IPVPCombatantInterface
	virtual APVPWeapon* GetWeaponActor() const override;
	virtual void GetAimSource(FVector& OutOrigin, FRotator& OutRotation) const override;
	
	bool IsDead() const;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Tick(float DeltaSeconds) override;
	
	/** 控制台调试：输入 DebugKillSelf 直接走一遍死亡流程（绕过伤害，方便验证 Dying → Dead） */
	UFUNCTION(Exec)
	void DebugKillSelf();

protected:
	virtual void DoAim(float Yaw, float Pitch) override;
	virtual void DoMove(float Right, float Forward)  override;
	virtual void DoJumpStart()  override;
	virtual void DoJumpEnd()  override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	
	void InitializeAbilitySystem();
	void GrantDefaultAbilities();
	void SpawnDefaultWeapon();
	
	void InputShootStart();
	void InputShootStop();
	void InputReload();
	
	UFUNCTION()
	void HandleDeathStarted(AActor* OwningActor, AActor* Killer);
	
	/** 死亡结算完成（Dying → Dead）：日志锚点，也是后续死亡结算表现（音效/UI/掉落）的唯一挂载点 */
	UFUNCTION()
	void HandleDeathFinished(AActor* OwningActor, AActor* Killer);

	void RestartLevelAfterDeath();
	
	// Input System
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputAction> FireAction;
	UPROPERTY(EditAnywhere,Category="Input")
	TObjectPtr<UInputAction> ReloadAction;

	// Ability System
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="PVP|AbilitySystem")
	TObjectPtr<UPVPAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UPVPHealthSet> HealthSet;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="PVP|Health")
	TObjectPtr<UPVPHealthComponent> HealthComponent;
	
	UPROPERTY(EditDefaultsOnly,Category="PVP|AbilitySystem")
	TArray<FPVPAbilityInputBinding> DefaultAbilities;
	
	// Weapon
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="PVP|Shooter")
	TObjectPtr<APVPWeapon> CurrentWeapon;
	
	UPROPERTY(EditDefaultsOnly,Category="Shooter")
	TSubclassOf<APVPWeapon> WeaponClass;
	
	UPROPERTY(EditDefaultsOnly,Category="Shooter")
	FTransform WeaponAttachOffset;
	
	UPROPERTY(EditDefaultsOnly,Category="Shooter")
	bool bRestartLevelOnDeath;

private:
	bool bAbilitySystemInitialized = false;

	/** 死亡表现是否已经执行过（等价于蓝图里的 Do Once） */
	bool bDeathStateApplied = false;
};
