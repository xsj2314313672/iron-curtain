// Fill out your copyright notice in the Description page of Project Settings.


#include "PVPHealthComponent.h"
#include "PVPAbilitySystemComponent.h"
#include "Attributes/PVPHealthSet.h"
#include "IronCurtain.h"
#include "PVPGameplayTags.h"
#include "Engine/World.h"          // GetWorld()->GetTimerManager()
#include "TimerManager.h"          // FTimerManager::SetTimer / ClearTimer

// Sets default values for this component's properties
UPVPHealthComponent::UPVPHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UPVPHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 组件被移除 / 关卡切换 / PIE 停止：先把定时器收掉，避免句柄一直挂在 TimerManager 上
	ClearDeathTimer();
	
	Super::EndPlay(EndPlayReason);
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
	
	// 走的是 SetNumericAttributeBase 而不是 GE，不会经过 PostGameplayEffectExecute，
	// 所以 bOutOfHealth 这个"已经归零过一次"的闩锁必须手动复位，否则复用后再也不会广播 OnOutOfHealth
	HealthSet->ResetOutOfHealthFlag();
	AbilitySystemComponent->SetNumericAttributeBase(UPVPHealthSet::GetHealthAttribute(),HealthSet->GetMaxHealth());
	DeathState = EPVPDeathState::NotDead;
	
	UE_LOG(LogIronCurtain,Log,TEXT("[PVPHealthComponent] %s 初始化完成: %.0f/ %.0f"),
		*GetNameSafe(Owner),HealthSet->GetHealth(),HealthSet->GetMaxHealth());
	// 委托签名是 (OldValue, NewValue, Instigator)：初始化属于全量同步，旧值和新值都应为当前血量，
	// 否则监听方按 NewValue - OldValue 算增减会得到 MaxHealth - Health 这种荒谬值
	OnHealthChanged.Broadcast(HealthSet->GetHealth(),HealthSet->GetHealth(),nullptr);
	
	// 按当前状态补一次标签（幂等）
	ApplyDeathTags(DeathState);
}

void UPVPHealthComponent::UninitializeFromAbilitySystem()
{
	// 先停定时器：否则和 ASC 断开之后，到点还会往一条已经没意义的路径上刷标签
	ClearDeathTimer();
	
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
	// 单向门：只有还没死过才能进死亡流程，蓝图/作弊命令重复调用都被这里吃掉
	if (DeathState != EPVPDeathState::NotDead)
	{
		return;
	}
	
	// 改状态（内部会立刻把 Dying 置 1、Dead 置 0）
	SetDeathStateInternal(EPVPDeathState::DeathStarted);
	
	// 关键：排一场"Dying → Dead"的结算，否则状态会永远停在 DeathStarted
	ScheduleFinishDeath();
	
	// 先刷标签再广播，保证监听者在回调里查 tag 时已经是新值
	UE_LOG(LogIronCurtain,Warning,TEXT("[PVPHealthComponent]的[Death] %s 进入死亡流程（%.2f 秒后转入 Dead），凶手 %s"),
		*GetNameSafe(GetOwner()),DeathFinishDelay,*GetNameSafe(LastKiller));
	OnDeathStarted.Broadcast(GetOwner(),LastKiller);
}

void UPVPHealthComponent::FinishDeath()
{
	// 定时器到点会走这里，蓝图也能手动提前调：进门先把句柄清掉，
	// 避免"手动调完 → 3 秒后定时器又进来一次"（下面还有状态检查，但句柄不能留着）
	ClearDeathTimer();
	
	// NotDead（根本没死过）和 DeathFinished（已经结算过）都不该再广播
	if (DeathState != EPVPDeathState::DeathStarted)
	{
		return;
	}
	
	// 内部会把 Dying 清 0、Dead 置 1
	SetDeathStateInternal(EPVPDeathState::DeathFinished);
	
	UE_LOG(LogIronCurtain,Warning,TEXT("[PVPHealthComponent]的[Death] %s 死亡结算完成，凶手 %s"),
		*GetNameSafe(GetOwner()),*GetNameSafe(LastKiller));
	OnDeathFinished.Broadcast(GetOwner(),LastKiller);
}

void UPVPHealthComponent::SetDeathStateInternal(EPVPDeathState NewState)
{
	if (DeathState == NewState)
	{
		return;
	}
	DeathState = NewState;
	ApplyDeathTags(NewState);
}

void UPVPHealthComponent::ApplyDeathTags(EPVPDeathState InState)
{
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogIronCurtain,Verbose,TEXT("[PVPHealthComponent] %s 暂无 ASC，跳过死亡标签同步（state=%d）"),
			*GetNameSafe(GetOwner()),static_cast<int32>(InState));
		return;
	}
	
	const int32 DyingCount = (InState == EPVPDeathState::DeathStarted) ? 1 : 0;
	const int32 DeadCount = (InState == EPVPDeathState::DeathFinished) ? 1 : 0;
	
	// 用 SetLooseGameplayTagCount 而不是 AddLooseGameplayTag / RemoveLooseGameplayTag：
	// 1) 幂等，不会像 Add 那样把计数叠成 2、3……（之后 Remove 一次还留着）
	// 2) 一次写死两个标签，数学上保证 Dying / Dead 严格互斥，任何时刻只会看到一个
	// 3) 第三参数保持默认 EGameplayTagReplicationState::None，标签不参与 GAS 自己的标签复制
	AbilitySystemComponent->SetLooseGameplayTagCount(PVPGameplayTags::Status_Death_Dying,DyingCount);
	AbilitySystemComponent->SetLooseGameplayTagCount(PVPGameplayTags::Status_Death_Dead,DeadCount);
	
	UE_LOG(LogIronCurtain,Log,TEXT("[PVPHealthComponent] %s 死亡标签同步 → Dying=%d Dead=%d"),
		*GetNameSafe(GetOwner()),DyingCount,DeadCount);
}

void UPVPHealthComponent::ScheduleFinishDeath()
{
	// 先清旧句柄，防止重复进入时留下两个定时器
	ClearDeathTimer();
	
	// 延迟配 0 时直接结算，方便调试
	if (DeathFinishDelay <= 0.f)
	{
		FinishDeath();
		return;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogIronCurtain,Error,TEXT("[PVPHealthComponent] %s 拿不到 World，无法排死亡结算定时器"),*GetNameSafe(GetOwner()));
		return;
	}
	
	// 绑 this 的成员函数：FTimerDelegate 内部存的是 TWeakObjectPtr，组件销毁后回调自动跳过，不会野指针
	World->GetTimerManager().SetTimer(DeathFinishTimerHandle,this,&UPVPHealthComponent::FinishDeath,DeathFinishDelay,false);
}

void UPVPHealthComponent::ClearDeathTimer()
{
	if (!DeathFinishTimerHandle.IsValid())
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathFinishTimerHandle);
	}
	// World 已经拆掉时也要把句柄作废，避免留下一个悬空句柄
	DeathFinishTimerHandle.Invalidate();
}

void UPVPHealthComponent::HandleHealthChanged(AActor* EffectInstigator, AActor* EffectCauser,
	const FGameplayEffectSpec* EffectSpec, float Magnitude, float OldValue, float NewValue)
{
	if (EffectInstigator && NewValue < OldValue)
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


