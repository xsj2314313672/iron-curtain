// Fill out your copyright notice in the Description page of Project Settings.


#include "PVPCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"


namespace
{
	/**
	 * 第一人称要隐藏的骨骼。只隐藏 head 一条就够：
	 * - head 覆盖整个头部 + 脸，是唯一会扫进相机的东西；
	 * - neck_01 / neck_02 留在原处，因为相机（Z=74 ≈ 世界 164cm）在脖子正前方，
	 *   脖子几何在相机背后/下方，平视和低头都看不到它，也就不会露出“断口”。
	 */
	const FName FirstPersonHiddenBone(TEXT("head"));
}


// Sets default values
APVPCharacter::APVPCharacter()
{
	// Avoid ticking characters if possible.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
	
	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));   // = -胶囊半高
	MeshComp->bCastHiddenShadow = true;   // 隐藏部分骨骼（如头部）时，影子仍然完整

	
	UCharacterMovementComponent* IronCurtainMoveComp = CastChecked<UCharacterMovementComponent>(GetCharacterMovement());
	IronCurtainMoveComp->GravityScale = 1.0f;
	IronCurtainMoveComp->MaxAcceleration = 2400.0f;
	IronCurtainMoveComp->BrakingFrictionFactor = 1.0f;
	IronCurtainMoveComp->BrakingFriction = 6.0f;
	IronCurtainMoveComp->GroundFriction = 8.0f;
	IronCurtainMoveComp->BrakingDecelerationWalking = 1400.0f;
	IronCurtainMoveComp->bUseControllerDesiredRotation = false;
	IronCurtainMoveComp->bOrientRotationToMovement = false;
	IronCurtainMoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	IronCurtainMoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	IronCurtainMoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	IronCurtainMoveComp->bCanWalkOffLedgesWhenCrouching = true;
	IronCurtainMoveComp->SetCrouchedHalfHeight(65.0f);
	
	// ---------------------------------------------------------------------
	// 第一人称相机
	// ---------------------------------------------------------------------
	// 眼高：胶囊半高 90 + Z 74 = 世界眼高 164cm（≈ 身高 180 × 0.9，与 CS2 的
	// 64/72 比例一致）。GetActorEyesViewPoint / AI 视线用 BaseEyeHeight，必须同步。
	//
	// X = 8：相机在胶囊中轴前方 8cm，也就是“脖子前面、脸后面”的位置 —— 这是
	// CS 类游戏的眼位。之所以敢放这么靠后，是因为本地玩家自己的 head 骨骼
	// 会被隐藏（见 UpdateFirstPersonHeadVisibility），头壳不会挡视线。
	//
	// 注意：这里不要为了“躲开头”把 X 往大调。只要头部几何还在，X 就必须大到
	// 18cm 以上才不穿帮，那时低头 45° 已经看不到身体（相机浮到胸口前面了）。
	// 正确解法是去掉头部几何，而不是把相机往前推。
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(GetCapsuleComponent()); 
	CameraComponent->SetRelativeLocation(FVector(8.0f, 0.0f, 74.0f));
	CameraComponent->bUsePawnControlRotation = true;

	// CS2 的 90° 是 4:3 下的水平 FOV，16:9 等价于 106°；锁定水平 FOV，换分辨率不变形
	CameraComponent->SetFieldOfView(106.0f);
	CameraComponent->bOverrideAspectRatioAxisConstraint = true;
	CameraComponent->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);

	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	
	// 与相机高度保持一致：GetActorEyesViewPoint / AI 视线用的是这个值
	BaseEyeHeight = 74.0f;

	// ---------------------------------------------------------------------
	// 阴影用全身副本（补回影子里的头）
	// ---------------------------------------------------------------------
	// 主网格对本地玩家隐藏 head 后，阴影也会缺头（骨骼可见性在蒙皮阶段生效，
	// 阴影通道用的就是这批骨骼）。这一具挂在主网格下面、位置与之完全重合，
	// 但：
	//   RenderInMainPass = false → 屏幕上永远不出现
	//   CastShadow        = 只有本地玩家才打开（别人由主网格正常投影，避免双份影子）
	// 骨骼由 SetLeaderPoseComponent 从主网格同步，所以不会有第二套动画开销。
	ShadowBody = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShadowBody"));
	ShadowBody->SetupAttachment(MeshComp);
	ShadowBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShadowBody->SetGenerateOverlapEvents(false);
	ShadowBody->SetCanEverAffectNavigation(false);
	ShadowBody->bReceivesDecals = false;
	ShadowBody->SetRenderInMainPass(false);
	ShadowBody->SetRenderInDepthPass(false);   // 也不写深度，否则会变成“看不见但挡光”的幽灵
	ShadowBody->SetVisibility(false, true);
	ShadowBody->SetCastShadow(false);
	ShadowBody->bCastHiddenShadow = false;

}

// Called when the game starts or when spawned
void APVPCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// 头一次尝试（此时可能还没被占有，函数内部自己判断 IsLocallyControlled）。
	UpdateFirstPersonHeadVisibility();

	// 自愈：骨骼可见性数组会在 LOD 切换时被重建，重建后就“重新长出头”了。
	// 这个定时器每秒两次只做一次数组比较（已隐藏就直接返回），成本可以忽略。
	GetWorldTimerManager().SetTimer(
		HeadVisibilityTimer,
		this,
		&APVPCharacter::UpdateFirstPersonHeadVisibility,
		0.5f,
		true);
}

void APVPCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// 客户端真正拿到控制权的时机。GameMode 生成 Pawn 的顺序是
	// SpawnActor(BeginPlay 已经跑完) → PossessedBy，所以 BeginPlay 里
	// IsLocallyControlled() 还是 false，必须在这里再补一次。
	UpdateFirstPersonHeadVisibility();
}

void APVPCharacter::UpdateFirstPersonHeadVisibility()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	const bool bLocalOwner = IsLocallyControlled();

	// 影子里的头：主网格的头被藏了，交给只投影的副本来补
	ConfigureShadowBody(bLocalOwner);

	if (!bLocalOwner)
	{
		// 别人看我：头必须在。万一本地玩家中途变成旁观（换人控制），把头顶回来。
		if (MeshComp->IsBoneHiddenByName(FirstPersonHiddenBone))
		{
			MeshComp->UnHideBoneByName(FirstPersonHiddenBone);
			UE_LOG(LogTemp, Warning, TEXT("[FirstPerson] UnHideBone 'head' on %s"), *GetName());
		}
		return;
	}

	if (!MeshComp->IsBoneHiddenByName(FirstPersonHiddenBone))
	{
		// PBO_None：不改物理体，只把这一条骨骼的几何从渲染里剔除。
		// 网格是单面的，只要这一块不画，相机放在头腔里也永远看不到头壳。
		MeshComp->HideBoneByName(FirstPersonHiddenBone, EPhysBodyOp::PBO_None);

		UE_LOG(LogTemp, Warning, TEXT("[FirstPerson] HideBone 'head' on %s -> %s"),
			*GetName(),
			MeshComp->IsBoneHiddenByName(FirstPersonHiddenBone) ? TEXT("OK") : TEXT("FAILED"));
	}
}

void APVPCharacter::ConfigureShadowBody(bool bLocalOwner)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!ShadowBody || !MeshComp)
	{
		return;
	}

	// 第一次：用主网格的资产（BP 里指定的那套），并把它设成 LeaderPose 的跟随者。
	// 跟随者不跑自己的动画，直接复用主网格的骨骼变换，所以没有额外动画开销。
	if (ShadowBody->GetSkeletalMeshAsset() != MeshComp->GetSkeletalMeshAsset())
	{
		ShadowBody->SetSkeletalMeshAsset(MeshComp->GetSkeletalMeshAsset());
		ShadowBody->SetLeaderPoseComponent(MeshComp);
	}

	if (ShadowBody->CastShadow != bLocalOwner)
	{
		// 本地玩家：影子全权交给副本，同时把主网格的投影关掉，
		// 否则两具完全重合的身体会叠出脏边和双重阴影。
		// 非本地玩家：副本整个不参与渲染（连 RenderState 都不创建），主网格照常投影。
		ShadowBody->SetVisibility(bLocalOwner, true);
		ShadowBody->SetCastShadow(bLocalOwner);
		MeshComp->SetCastShadow(!bLocalOwner);
	}
}

// Called to bind functionality to input
void APVPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APVPCharacter::Move);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APVPCharacter::Look);
		}
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &ACharacter::Jump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
	}
}

void APVPCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller) return;

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	if (!FMath::IsNearlyZero(Axis.X))
	{
		AddMovementInput(YawRotation.RotateVector(FVector::RightVector), Axis.X);
	}
	if (!FMath::IsNearlyZero(Axis.Y))
	{
		AddMovementInput(YawRotation.RotateVector(FVector::ForwardVector), Axis.Y);
	}
}

void APVPCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);     // 注意：不乘 DeltaTime
}


