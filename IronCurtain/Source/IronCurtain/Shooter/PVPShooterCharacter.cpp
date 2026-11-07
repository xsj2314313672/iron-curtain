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
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	return false;
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
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ClearAbilityInput();
		AbilitySystemComponent->CancelAbilities();
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(OwningActor))
	{
		DisableInput(PlayerController);
	}
	if (bRestartLevelOnDeath)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle,this,&APVPShooterCharacter::RestartLevelAfterDeath,2.5f,false);
	}
}

void APVPShooterCharacter::RestartLevelAfterDeath()
{
	UGameplayStatics::OpenLevel(this,FName(*GetWorld()->GetName()),false);
}
