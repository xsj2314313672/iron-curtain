#pragma once

#include "NativeGameplayTags.h"

namespace  PVPGameplayTags
{
	/** 输入标签：Enhanced Input 的 IA_Fire 按下时推这个标签进 ASC */
	IRONCURTAIN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Shoot);
	/** 技能标签：同时也是"被死亡打断"的依据 */
	IRONCURTAIN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Shoot);
	/** SetByCaller：GE_Damage 靠它取伤害数值 */
	IRONCURTAIN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	/** 死亡状态：挂上后 Ability_Shoot 会被 ActivationBlockedTags 挡住 */
	IRONCURTAIN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dying);
	IRONCURTAIN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dead);
}
