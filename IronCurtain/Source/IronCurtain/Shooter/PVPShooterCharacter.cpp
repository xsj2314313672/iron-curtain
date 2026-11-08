// Fill out your copyright notice in the Description page of Project Settings.


#include "Shooter/PVPShooterCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "IronCurtain.h"
#include "UObject/ConstructorHelpers.h"
#include "PVPWeapon.h"
#include "AbilitySystem/PVPAbilitySystemComponent.h"
#include "AbilitySystem/PVPGameplayTags.h"
#include "AbilitySystem/PVPHealthComponent.h"
#include "AbilitySystem/Attributes/PVPHealthSet.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

APVPShooterCharacter::APVPShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
	
	AbilitySystemComponent = CreateDefaultSubobject<UPVPAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	HealthSet = CreateDefaultSubobject<UPVPHealthSet>(TEXT("HealthSet"));
	
	HealthComponent = CreateDefaultSubobject<UPVPHealthComponent>(TEXT("HealthComponent"));
	
	WeaponClass = APVPWeapon::StaticClass();
	
	// 输入资源兜底：蓝图里仍然可以覆盖 FireAction
	static ConstructorHelpers::FObjectFinder<UInputAction> FireActionAsset(TEXT("/Game/IronCurtain/Input/IA_Fire"));
	if (FireActionAsset.Succeeded())
	{
		FireAction = FireActionAsset.Object;
	}
	
	static ConstructorHelpers::FObjectFinder<UInputAction> ReloadActionAsset(TEXT("/Game/IronCurtain/Input/IA_Reload"));
	if (ReloadActionAsset.Succeeded())
	{
		ReloadAction = ReloadActionAsset.Object;
	}
}

UAbilitySystemComponent* APVPShooterCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

APVPWeapon* APVPShooterCharacter::GetWeaponActor() const
{
	return CurrentWeapon;
}

void APVPShooterCharacter::GetAimSource(FVector& OutOrigin, FRotator& OutRotation) const
{
	if (const UCameraComponent* Camera = GetFirstPersonCameraComponent())
	{
		OutOrigin = Camera->GetComponentLocation();
		OutRotation = Camera->GetComponentRotation();
		return;
	}
	OutOrigin = GetActorLocation();
	OutRotation = GetActorRotation();
}

bool APVPShooterCharacter::IsDead() const
{
	// the character is dead if their current HP drops to zero
	return HealthComponent && HealthComponent->IsDeadOrDying();
}

void APVPShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void APVPShooterCharacter::DoMove(float Right, float Forward)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoMove(Right, Forward);
	}
}

void APVPShooterCharacter::DoJumpStart()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpStart();
	}
}

void APVPShooterCharacter::DoJumpEnd()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpEnd();
	}
}

void APVPShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (HealthComponent)
	{
		HealthComponent->OnDeathStarted.AddDynamic(this, &APVPShooterCharacter::HandleDeathStarted);
		// 死亡结算（Dying → Dead）：目前不改表现，挂上来是为了拿到一条明确的日志锚点，
		// 后续"尸体停留 N 秒销毁""死亡结算音效"之类的逻辑直接加在这里
		HealthComponent->OnDeathFinished.AddDynamic(this, &APVPShooterCharacter::HandleDeathFinished);
	}
	SpawnDefaultWeapon();
	if (GetController())
	{
		InitializeAbilitySystem();
	}
}

void APVPShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (HealthComponent)
	{
		HealthComponent->OnDeathStarted.RemoveDynamic(this, &APVPShooterCharacter::HandleDeathStarted);
		HealthComponent->OnDeathFinished.RemoveDynamic(this, &APVPShooterCharacter::HandleDeathFinished);
		// 内部会 ClearDeathTimer，所以死亡后 3 秒内 EndPlay 不会被定时器回调打进来
		HealthComponent->UninitializeFromAbilitySystem();
	}
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ClearAbilityInput();
	}
	
	Super::EndPlay(EndPlayReason);
}

void APVPShooterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

void APVPShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ProcessAbilityInput(DeltaSeconds,false);
	}
}


void APVPShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}
	if (FireAction)
	{
		EnhancedInputComponent->BindAction(FireAction,ETriggerEvent::Started,this,&APVPShooterCharacter::InputShootStart);
		EnhancedInputComponent->BindAction(FireAction,ETriggerEvent::Completed,this,&APVPShooterCharacter::InputShootStop);
		EnhancedInputComponent->BindAction(FireAction,ETriggerEvent::Canceled,this,&APVPShooterCharacter::InputShootStop);
	}
	if (ReloadAction)
	{
		EnhancedInputComponent->BindAction(ReloadAction,ETriggerEvent::Started,this,&APVPShooterCharacter::InputReload);
	}
}

void APVPShooterCharacter::InitializeAbilitySystem()
{
	if (bAbilitySystemInitialized || !AbilitySystemComponent || !HealthComponent)
	{
		return;
	}
	AbilitySystemComponent->InitAbilityActorInfo(this,this);
	HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	GrantDefaultAbilities();
	bAbilitySystemInitialized = true;
}

void APVPShooterCharacter::GrantDefaultAbilities()
{
	for (const FPVPAbilityInputBinding& Binding:DefaultAbilities)
	{
		if (!Binding.AbilityClass)
		{
			continue;
		}
		FGameplayAbilitySpec Spec(Binding.AbilityClass,1,INDEX_NONE,this);
		if (Binding.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(Binding.InputTag);
		}
		AbilitySystemComponent->GiveAbility(Spec);
	}
}

void APVPShooterCharacter::SpawnDefaultWeapon()
{
	UWorld* const World = GetWorld();
	USkeletalMeshComponent* FirstPersonMeshComp = GetFirstPersonMesh();
	if (!World || !FirstPersonMeshComp || !WeaponClass)
	{
		return;
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	CurrentWeapon = World->SpawnActor<APVPWeapon>(WeaponClass,GetActorTransform(),SpawnParameters);
	if (!CurrentWeapon)
	{
		UE_LOG(LogIronCurtain,Error,TEXT("[Weapon] 生成失败"));
		return;
	}
	
	const FName CandidateSockets[] = { TEXT("weapon_r"), TEXT("weapon_r_muzzle"), TEXT("hand_r") };
	for (const FName& SocketName : CandidateSockets)
	{
		if (FirstPersonMeshComp->DoesSocketExist(SocketName))
		{
			CurrentWeapon->AttachToComponent(FirstPersonMeshComp,
				FAttachmentTransformRules::KeepRelativeTransform, SocketName);
			CurrentWeapon->SetActorRelativeTransform(WeaponAttachOffset);
			UE_LOG(LogIronCurtain, Log, TEXT("[Weapon] 挂到 socket '%s'"), *SocketName.ToString());
			return;
		}
	}

	CurrentWeapon->AttachToComponent(FirstPersonMeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale);
}

void APVPShooterCharacter::InputShootStart()
{
	if (AbilitySystemComponent && !IsDead())
	{
		AbilitySystemComponent->AbilityInputTagPressed(PVPGameplayTags::InputTag_Shoot);
	}
}

void APVPShooterCharacter::InputShootStop()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputTagReleased(PVPGameplayTags::InputTag_Shoot);
	}
}

void APVPShooterCharacter::InputReload()
{
	if (CurrentWeapon && !IsDead())
	{
		CurrentWeapon->Reload();
	}
}

void APVPShooterCharacter::HandleDeathStarted(AActor* OwningActor, AActor* Killer)
{
	UE_LOG(LogIronCurtain,Warning,TEXT("[Player] Death Started, The Killer Is :%s"),*GetNameSafe(Killer));

	// ---- 0. Do Once：防止重复触发 ----
	if (bDeathStateApplied)
	{
		return;
	}
	bDeathStateApplied = true;

	// ---- 1. 能力与输入：清输入、取消技能、关掉控制器的移动/视角输入 ----
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ClearAbilityInput();
		AbilitySystemComponent->CancelAbilities();
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		// 等价于蓝图里 "Disable Input (Target = self)" 的意图。
		// 注意别直接调 Pawn 上的 DisableInput：它只做 PopInputComponent，
		// 而 Pawn 的 InputComponent 不在 PlayerController 的 CurrentInputStack 里，调了等于没调。
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
	}

	// ---- 2. CharacterMovement → Disable Movement ----
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	// ---- 3. CapsuleComponent → Set Collision Enabled = No Collision ----
	// 不关的话尸体会被胶囊顶在半空，还会继续挡子弹、把其他角色推开
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// ---- 4~6. Mesh → 碰撞配置改 Ragdoll、开物理模拟 ----
	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		// "Ragdoll" 是引擎内置的碰撞配置：QueryAndPhysics + PhysicsBody，
		// 并且默认忽略 Pawn / Vehicle / Weapon 通道
		MeshComponent->SetCollisionProfileName(TEXT("Ragdoll"));

		// SetSimulatePhysics 只作用于 BodyInstance 单个刚体，骨骼网格必须用 AllBodies 版本
		MeshComponent->SetAllBodiesSimulatePhysics(true);
		MeshComponent->SetSimulatePhysics(true);
		MeshComponent->WakeAllRigidBodies();
	}

	// ---- 7. 重开关卡（bRestartLevelOnDeath 默认 false） ----
	if (bRestartLevelOnDeath)
	{
		// 必须晚于 DeathFinishDelay，否则关卡先重开了，Dying → Dead 的结算永远等不到
		const float RestartDelay = (HealthComponent ? HealthComponent->DeathFinishDelay : 0.f) + 0.5f;
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle,this,&APVPShooterCharacter::RestartLevelAfterDeath,RestartDelay,false);
	}
}

void APVPShooterCharacter::HandleDeathFinished(AActor* OwningActor, AActor* Killer)
{
	// 尸体留场不销毁（本轮不引入销毁/重生）。
	// 这个函数存在的意义：给"状态确实从 DeathStarted 走到了 DeathFinished"留一条服务端日志证据，
	// 也是后续所有"死亡结算"表现（音效、UI、掉落）的唯一挂载点
	UE_LOG(LogIronCurtain,Warning,TEXT("[Player] Death Finished，凶手 %s（布娃娃留场）"),*GetNameSafe(Killer));
}

void APVPShooterCharacter::DebugKillSelf()
{
	// 只给本地调试用：绕过伤害流程直接进状态机，方便验证 Dying → Dead 与标签切换
	if (HealthComponent)
	{
		HealthComponent->StartDeath();
	}
}

void APVPShooterCharacter::RestartLevelAfterDeath()
{
	UGameplayStatics::OpenLevel(this,FName(*GetWorld()->GetName()),false);
}
