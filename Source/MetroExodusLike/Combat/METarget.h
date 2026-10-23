#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "METarget.generated.h"

class UMEHealthComponent;
class USoundBase;
class AController;

/**
 * Shooting range target: rocks back on each hit, falls over when its health runs out, stands back up after a while.
 * The pivot is the foot of the target; the target faces the actor's X axis and falls backward.
 */
UCLASS(Abstract)
class METROEXODUSLIKE_API AMETarget : public AActor
{
	GENERATED_BODY()

public:
	AMETarget();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Rotates when hit / falling. Place the mesh so its foot is on the pivot. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UMEHealthComponent> Health;

	/** Rock back per hit. */
	UPROPERTY(EditAnywhere, Category = "Target", meta = (ClampMin = "0", ClampMax = "45", Units = "Degrees"))
	float HitKickAngle = 6.f;

	/** How fast a hit kick settles back. */
	UPROPERTY(EditAnywhere, Category = "Target", meta = (ClampMin = "0"))
	float HitRecoverySpeed = 8.f;

	/** Lying angle once down. */
	UPROPERTY(EditAnywhere, Category = "Target", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float DownAngle = 85.f;

	/** Falling / standing up speed (higher = faster, eases out at the end). */
	UPROPERTY(EditAnywhere, Category = "Target", meta = (ClampMin = "0"))
	float FallSpeed = 7.f;

	/** Time down before standing back up. 0 = stays down. */
	UPROPERTY(EditAnywhere, Category = "Target", meta = (ClampMin = "0", Units = "s"))
	float ResetDelay = 3.f;

	UPROPERTY(EditAnywhere, Category = "Target|Sound")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, Category = "Target|Sound")
	TObjectPtr<USoundBase> DownSound;

private:
	void HandleHealthChanged(float NewHealth, float Delta, AController* DamageInstigator);
	void HandleDeath(AController* Killer);
	void StandUp();

	float FallAngle = 0.f;
	float TargetFallAngle = 0.f;
	float KickAngle = 0.f;
	FTimerHandle ResetTimer;
};
