#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "METoxicZone.generated.h"

class UBoxComponent;
class UNavModifierComponent;

/**
 * Toxic air (gas, radiation dust...): without a gas mask the player (or an enemy) is slowly poisoned.
 * Enemies' paths avoid it when they can.
 * Scale the box in the level to fit the place. The look of the gas (fog, particles) goes in the Blueprint.
 */
UCLASS()
class METROEXODUSLIKE_API AMEToxicZone : public AActor
{
	GENERATED_BODY()

public:
	AMEToxicZone();

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Zone;

	/** Makes the zone costly on the navmesh: enemies go around it if there is a way. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNavModifierComponent> NavModifier;

	/** How fast the air poisons (1 = normal, 2 = twice as fast). */
	UPROPERTY(EditAnywhere, Category = "Toxic Zone", meta = (ClampMin = "0.1"))
	float Density = 1.f;
};
