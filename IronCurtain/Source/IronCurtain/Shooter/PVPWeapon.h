// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "PVPWeapon.generated.h"

class USkeletalMeshComponent;
class UDamageType;

/**
 * 简单枪械武器 Actor（原型阶段用）。
 *
 * 只做三件事：
 *   1. 射速限速 + 弹药计数（Fire / CanFire / Reload）；
 *   2. 一次射线检测（LineTraceSingleByChannel，默认 Visibility 通道）；
 *   3. 命中后 UGameplayStatics::ApplyPointDamage 结算伤害。
 *
 * 用法：
 *   - 蓝图继承它，设置武器模型（Weapon Mesh）、枪口插槽名（默认 "Muzzle"）、
 *     射速 / 弹匣容量 / 伤害 / 弹道距离；
 *   - 角色生成武器后调用 SetOwningPawn(this)，再手动 AttachToComponent 到
 *     第一人称 Mesh 的手部插槽；
 *   - 单发：扳机按下调一次 Fire()；全自动：按住期间在 Tick 里反复调 Fire()，
 *     内部会按 RoundsPerMinute 限速，多余的调用直接返回 false。
 *
 * 注意：伤害还没接 GAS，走的是引擎原生 Damage。以后要改成 GameplayEffect，
 * 只需要改 PerformTraceShot() 里命中结算的那几行。
 */
UCLASS(Blueprintable, meta = (DisplayName = "PVP Weapon"))
class IRONCURTAIN_API APVPWeapon : public AActor
{
	GENERATED_BODY()

public:

	/** Constructor */
	APVPWeapon();

	/**
	 * 扣一次扳机：限速 → 扣弹药 → 射线检测 → 结算伤害。
	 * 返回 true 表示这一发真的打出去了（被射速或空弹匣挡住时返回 false）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool Fire();

	/** 换弹：直接把弹药补满（换弹时间、动画、音效留给蓝图或后续实现） */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Reload();

	/** 设置持有人，同时同步 Owner / Instigator，伤害归属才正确 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void SetOwningPawn(APawn* InPawn);

	/** 当前持有人，可能为空 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	APawn* GetOwningPawn() const { return OwningPawn; }

	/** 当前弹药数 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	/** 现在能不能开火（弹药 + 射速冷却） */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanFire() const;

	/** 两发之间的最短间隔（秒） */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetFireInterval() const;

	/** 枪口的世界变换：优先取 MuzzleSocketName 插槽，没有插槽就退回 Mesh 自身 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	FTransform GetMuzzleTransform() const;

	/** 武器模型 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

protected:

	/** Gameplay 初始化：补满弹药 */
	virtual void BeginPlay() override;

	/** 真正做一次射线检测并结算伤害 */
	virtual void PerformTraceShot();

	/** 算弹道的起点和方向（枪口 or 持有人视线） */
	virtual void GetTraceStartAndDirection(FVector& OutStartLocation, FVector& OutDirection) const;

	/** 开火成功的蓝图钩子：枪口特效、音效、后坐力、动画都可以挂这里 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnFired();

	/** 命中的蓝图钩子：弹孔、血雾、命中提示 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon")
	void OnWeaponHit(const FHitResult& Hit);

	/** 武器模型（Actor 的根组件） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	USkeletalMeshComponent* WeaponMesh;

	/** 枪口插槽名；模型上没有这个插槽时会退回 Mesh 自身的变换 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	FName MuzzleSocketName = FName("Muzzle");

	/** 第一人称武器：只给自己看，并走 FirstPerson 渲染通道（避免贴脸穿模） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	bool bFirstPersonOnly = true;

	/** 射速（发 / 分钟），600 就是 0.1 秒一发 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire", meta = (ClampMin = "1.0"))
	float RoundsPerMinute = 600.0f;

	/** 弹匣容量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire", meta = (ClampMin = "1"))
	int32 MaxAmmo = 30;

	/** 无限弹药（调试用） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire")
	bool bInfiniteAmmo = false;

	/** 单发伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire", meta = (ClampMin = "0.0"))
	float BaseDamage = 20.0f;

	/** 最大弹道距离（厘米，10000 = 100 米） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire", meta = (ClampMin = "0.0"))
	float TraceDistance = 10000.0f;

	/** 弹道检测用的碰撞通道 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** 伤害类型；留空就用引擎默认的 UDamageType */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire")
	TSubclassOf<UDamageType> DamageTypeClass;

	/** true = 从枪口发射；false = 从持有人视线发射（第一人称更准，默认） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Fire")
	bool bUseMuzzleAsTraceStart = false;

	/** 在游戏里画出弹道，方便没有特效时调试 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Debug")
	bool bDrawDebugTrace = false;

	/** 当前弹药数 */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	int32 CurrentAmmo = 0;

	/** 上一次开火的世界时间（秒），用来算射速冷却 */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	float LastFireTime = -1000.0f;

	/** 持有人 */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	APawn* OwningPawn = nullptr;
};
