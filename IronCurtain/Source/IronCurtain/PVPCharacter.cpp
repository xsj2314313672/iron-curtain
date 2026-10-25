// Fill out your copyright notice in the Description page of Project Settings.


#include "PVPCharacter.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "InputAction.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "IronCurtain.h"

// PVPCharacter.h 里用 DECLARE_LOG_CATEGORY_EXTERN 声明了该分类，必须有一处定义，否则链接会报 LNK2001
DEFINE_LOG_CATEGORY(LogTemplateCharacter);

// Sets default values
APVPCharacter::APVPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// 输入资源兜底：蓝图里仍然可以覆盖，避免新建角色蓝图忘记指定导致按键完全没反应
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionAsset(TEXT("/Game/IronCurtain/Input/IA_Jump"));
	if (JumpActionAsset.Succeeded())
	{
		JumpAction = JumpActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionAsset(TEXT("/Game/IronCurtain/Input/IA_Move"));
	if (MoveActionAsset.Succeeded())
	{
		MoveAction = MoveActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionAsset(TEXT("/Game/IronCurtain/Input/IA_Look"));
	if (LookActionAsset.Succeeded())
	{
		LookAction = LookActionAsset.Object;

		// 项目目前只有一个 IA_Look（Mouse2D），鼠标视角和手柄右摇杆共用同一个 Action
		if (!MouseLookAction)
		{
			MouseLookAction = LookActionAsset.Object;
		}
	}
}

void APVPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 绑定前先报告缺失的 Action，否则 Enhanced Input 会静默什么都不绑，很难排查
		if (!JumpAction || !MoveAction || !LookAction)
		{
			UE_LOG(LogTemplateCharacter, Error,
				TEXT("'%s' 输入 Action 未配置完整（Jump=%s, Move=%s, Look=%s），对应按键不会生效。"),
				*GetNameSafe(this),
				JumpAction ? TEXT("OK") : TEXT("空"),
				MoveAction ? TEXT("OK") : TEXT("空"),
				LookAction ? TEXT("OK") : TEXT("空"));
		}

		// Jumping
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APVPCharacter::DoJumpStart);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APVPCharacter::DoJumpEnd);
		}

		// Moving
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APVPCharacter::MoveInput);
		}

		// Looking/Aiming（鼠标与摇杆可能共用同一个 IA_Look，重复绑定同一个 Action 是安全的）
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APVPCharacter::LookInput);
		}

		if (MouseLookAction && MouseLookAction != LookAction)
		{
			EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &APVPCharacter::LookInput);
		}
	}
	else
	{
		UE_LOG(LogIronCurtain, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void APVPCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void APVPCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void APVPCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void APVPCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void APVPCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void APVPCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}