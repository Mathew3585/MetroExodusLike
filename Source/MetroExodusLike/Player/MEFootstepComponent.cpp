#include "Player/MEFootstepComponent.h"
#include "Player/MEFootstepData.h"
#include "Player/MECharacter.h"
#include "AI/MENoise.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UMEFootstepComponent::UMEFootstepComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// After the body's animation: the feet are where they are drawn this frame
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UMEFootstepComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	Feet.SetNum(FootBones.Num());
	if (!Character || !FootstepData)
	{
		SetComponentTickEnabled(false);
	}
}

void UMEFootstepComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const USkeletalMeshComponent* Body = Character->GetMesh();
	if (!Movement->IsMovingOnGround())
	{
		// Feet in the air are not steps; the landing has its own sound
		for (FFootState& Foot : Feet)
		{
			Foot.bLifted = false;
		}
		return;
	}

	const AMECharacter* MECharacter = Cast<AMECharacter>(Character);
	const bool bSprinting = MECharacter && MECharacter->IsSprinting();
	const float Volume = Character->bIsCrouched ? FootstepData->CrouchVolume
		: bSprinting ? FootstepData->SprintVolume
		: FootstepData->WalkVolume;
	const float Noise = Character->bIsCrouched ? FootstepData->CrouchNoise
		: bSprinting ? FootstepData->SprintNoise
		: FootstepData->WalkNoise;

	const float GroundZ = Character->GetActorLocation().Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const double Now = GetWorld()->GetTimeSeconds();
	for (int32 Index = 0; Index < FootBones.Num(); ++Index)
	{
		FFootState& Foot = Feet[Index];
		const FVector FootLocation = Body->GetSocketLocation(FootBones[Index]);
		const float Height = FootLocation.Z - GroundZ;

		if (Height > LiftHeight)
		{
			Foot.bLifted = true;
		}
		else if (Foot.bLifted && Height < PlantHeight)
		{
			Foot.bLifted = false;
			if (Now - Foot.LastStepTime >= MinStepInterval)
			{
				Foot.LastStepTime = Now;
				PlayStep(FootLocation, Volume, Noise, MENoise::Footstep);
			}
		}
	}
}

void UMEFootstepComponent::PlayLanding(float Strength)
{
	if (!Character || !FootstepData)
	{
		return;
	}

	const FVector FeetLocation = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const float Scale = FMath::Clamp(Strength, 0.3f, 1.5f);
	PlayStep(FeetLocation, FootstepData->LandingVolume * Scale, FootstepData->LandingNoise * Scale, MENoise::Landing);
}

void UMEFootstepComponent::PlayStep(const FVector& FootLocation, float Volume, float Noise, FName NoiseTag)
{
	// What is under the foot decides the sound
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Footstep), false, Character);
	Params.bReturnPhysicalMaterial = true;
	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, FootLocation + FVector(0.f, 0.f, 20.f), FootLocation - FVector(0.f, 0.f, 50.f), ECC_Visibility, Params);

	if (USoundBase* Sound = FootstepData->FindSound(Hit.PhysMaterial.Get()))
	{
		const float Pitch = 1.f + FMath::FRandRange(-FootstepData->PitchVariation, FootstepData->PitchVariation);
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Hit.bBlockingHit ? Hit.ImpactPoint : FootLocation, Volume, Pitch);
	}
	MENoise::Report(Character, FootLocation, Noise, NoiseTag);
}
