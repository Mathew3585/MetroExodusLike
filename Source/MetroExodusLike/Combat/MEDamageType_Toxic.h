#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "MEDamageType_Toxic.generated.h"

/** Choking on toxic air: the player coughs instead of grunting, the view does not jolt. */
UCLASS()
class METROEXODUSLIKE_API UMEDamageType_Toxic : public UDamageType
{
	GENERATED_BODY()
};
