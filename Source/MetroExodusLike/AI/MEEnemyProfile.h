#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MEEnemyProfile.generated.h"

class USoundBase;

/** Lines an enemy says out loud: they tell the player what it is doing. */
UENUM()
enum class EMEBark : uint8
{
	/** "Did you hear that?" */
	Suspicious,
	/** "There he is!" */
	Spotted,
	/** "Where did he go?" */
	LostTarget,
	/** "Spread out, find him." */
	Searching,
	/** "Must have been a rat." */
	GiveUp,
	/** "Reloading!" */
	Reloading,
	/** "They got Misha!" */
	AllyDown,
	/** Grunt when a bullet hits. */
	Pain,
	/** Last cry. */
	Death,
	/** "I'm hit, help!" (low health, once) */
	Hurt,
	/** "Misha? Answer!" */
	CallingAlly,
	/** "I'm out of here!" */
	Fleeing,
	/** "Don't shoot!" */
	Surrender,
	/** "Sound the alarm!" */
	Alarm
};

/**
 * What kind of fighter an enemy is (DA_Enemy_Recruit, DA_Enemy_Veteran...): how fast it notices,
 * how well it shoots, how brave and how pushy it is, and its voice.
 */
UCLASS(BlueprintType)
class METROEXODUSLIKE_API UMEEnemyProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Sound of a line, or null if it has none. */
	USoundBase* GetBark(EMEBark Bark) const;

	// --- Senses ---

	/** Multiplies how fast the detection gauge fills. */
	UPROPERTY(EditDefaultsOnly, Category = "Senses", meta = (ClampMin = "0.1"))
	float DetectionSpeed = 1.f;

	/** Time to react: stares before checking something, waits before the first shot once it spots the player. */
	UPROPERTY(EditDefaultsOnly, Category = "Senses", meta = (ClampMin = "0", Units = "s"))
	float ReactionTime = 0.8f;

	// --- Combat ---

	/** Spread of its shots (degrees, around the target). Lower is deadlier. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", Units = "Degrees"))
	float Spread = 3.f;

	/** Damage of its shots, as a share of the weapon's (enemies hit softer than the player). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0"))
	float DamageScale = 0.5f;

	/** Rounds per burst, picked between these. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "1"))
	int32 BurstMin = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "1"))
	int32 BurstMax = 6;

	/** 0 = stays in cover far away, 1 = pushes close, flanks, ducks less. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", ClampMax = "1"))
	float Aggression = 0.5f;

	// --- Morale ---

	/** 0 = breaks at the first loss, 1 = never runs. */
	UPROPERTY(EditDefaultsOnly, Category = "Morale", meta = (ClampMin = "0", ClampMax = "1"))
	float Courage = 0.5f;

	/** Can give up (weapon dropped, hands up) instead of running when it has nothing left. */
	UPROPERTY(EditDefaultsOnly, Category = "Morale")
	bool bCanSurrender = true;

	// --- Gear ---

	/** Wears a gas mask: walks through toxic air unharmed. */
	UPROPERTY(EditDefaultsOnly, Category = "Gear")
	bool bHasGasMask = false;

	// --- Voice ---

	/** Its lines (random cues). Missing lines are simply not said. */
	UPROPERTY(EditDefaultsOnly, Category = "Voice")
	TMap<EMEBark, TObjectPtr<USoundBase>> Barks;
};
