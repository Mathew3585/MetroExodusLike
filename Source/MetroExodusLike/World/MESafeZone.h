#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MESafeZone.generated.h"

class UBoxComponent;

/**
 * Place where the player puts the weapon away (station, friendly camp...): no firing, no aiming, no crosshair.
 * The weapon comes back out when leaving. Scale the box in the level to fit the place.
 */
UCLASS()
class METROEXODUSLIKE_API AMESafeZone : public AActor
{
	GENERATED_BODY()

public:
	AMESafeZone();

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Zone;
};
