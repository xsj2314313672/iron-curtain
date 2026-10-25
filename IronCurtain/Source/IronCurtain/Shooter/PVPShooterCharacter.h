// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PVPCharacter.h"
#include "PVPShooterCharacter.generated.h"

/**
 * 
 */
UCLASS()
class IRONCURTAIN_API APVPShooterCharacter : public APVPCharacter
{
	GENERATED_BODY()
	
public:
	/** Constructor */
	APVPShooterCharacter();
	
protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	
public:
	/** Handles aim inputs from either controls or UI interfaces */
	virtual void DoAim(float Yaw, float Pitch) override;
	
	/** Handles move inputs from either controls or UI interfaces */
	virtual void DoMove(float Right, float Forward)  override;

	/** Handles jump start inputs from either controls or UI interfaces */
	virtual void DoJumpStart()  override;

	/** Handles jump end inputs from either controls or UI interfaces */
	virtual void DoJumpEnd()  override;
public:

	/** Returns true if the character is dead */
	bool IsDead() const;
};
