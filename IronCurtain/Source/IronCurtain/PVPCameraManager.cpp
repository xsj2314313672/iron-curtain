// Fill out your copyright notice in the Description page of Project Settings.

#include "PVPCameraManager.h"

namespace PVPCameraManagerConstants
{
	/** 俯仰角的硬性安全边界，超出这个范围第一人称镜头会翻面 */
	constexpr float PitchHardMin = -89.0f;
	constexpr float PitchHardMax = 89.0f;

	/** 本项目 PvP 的默认俯仰范围：低头 65 度、抬头 65 度（组件 UPVPCameraPitchComponent 会覆盖它） */
	constexpr float DefaultPitchMin = -65.0f;
	constexpr float DefaultPitchMax = 65.0f;
}

APVPCameraManager::APVPCameraManager()
{
	SetPitchLimits(PVPCameraManagerConstants::DefaultPitchMin, PVPCameraManagerConstants::DefaultPitchMax);
}

void APVPCameraManager::SetPitchLimits(float InPitchMin, float InPitchMax)
{
	// 下限只能是负值或 0（向下看），上限只能是正值或 0（向上看）
	ViewPitchMin = FMath::Clamp(InPitchMin, PVPCameraManagerConstants::PitchHardMin, 0.0f);
	ViewPitchMax = FMath::Clamp(FMath::Max(InPitchMax, ViewPitchMin), 0.0f, PVPCameraManagerConstants::PitchHardMax);
}

void APVPCameraManager::GetPitchLimits(float& OutPitchMin, float& OutPitchMax) const
{
	OutPitchMin = ViewPitchMin;
	OutPitchMax = ViewPitchMax;
}
