#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Components/ArrowComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"

AMEWeapon::AMEWeapon()
{
	// Only ticks while the muzzle light fades
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MuzzleFlash = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MuzzleFlash"));
	MuzzleFlash->SetupAttachment(Mesh);
	MuzzleFlash->SetAutoActivate(false);
	MuzzleFlash->SetCastShadow(false);

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(Mesh);
	MuzzleLight->SetIntensityUnits(ELightUnits::Candelas);
	MuzzleLight->SetIntensity(40.f);
	MuzzleLight->SetAttenuationRadius(800.f);
	MuzzleLight->SetLightColor(FLinearColor(1.f, 0.7f, 0.4f));
	// Visible in the Blueprint viewport to place it, turned off in game until a shot (Initialize)
	MuzzleLight->SetCastShadows(false);

	ShellEjectPort = CreateDefaultSubobject<UArrowComponent>(TEXT("ShellEjectPort"));
	ShellEjectPort->SetupAttachment(Mesh);
	ShellEjectPort->ArrowSize = 0.1f;
	ShellEjectPort->ArrowColor = FColor(255, 200, 0);
}

void AMEWeapon::Initialize(bool bFirstPerson)
{
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// Every visual, the parts added in the Blueprint included
	TArray<UPrimitiveComponent*> Visuals;
	GetComponents<UPrimitiveComponent>(Visuals);
	for (UPrimitiveComponent* Visual : Visuals)
	{
		if (Visual == ShellEjectPort)
		{
			continue;
		}
		Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (bFirstPerson)
		{
			// Drawn with the view model scale, the world weapon casts the shadow
			Visual->SetCastShadow(false);
			Visual->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
		}
		else
		{
			// Hidden from the owner, its shadow falls on the world but never on the first-person meshes
			Visual->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
		}
	}

	MuzzleLightPeak = MuzzleLight->Intensity;
	MuzzleLightRestTransform = MuzzleLight->GetRelativeTransform();
	MuzzleLight->SetVisibility(false);
	if (!bFirstPerson)
	{
		// Only the first-person weapon flashes: one light per shot
		MuzzleLightPeak = 0.f;
	}

	AmmoInMagazine = Data ? Data->MagazineSize : 0;
	FireModeIndex = 0;
}

int32 AMEWeapon::GetRoomInMagazine() const
{
	return Data ? FMath::Max(0, Data->MagazineSize - AmmoInMagazine) : 0;
}

void AMEWeapon::PlayWeaponAnimation(UAnimSequenceBase* Animation)
{
	if (Animation)
	{
		Mesh->PlayAnimation(Animation, false);
	}
}

USceneComponent* AMEWeapon::GetMuzzle() const
{
	return MuzzleFlash;
}

FVector AMEWeapon::GetBarrelDirection() const
{
	return MuzzleFlash->GetForwardVector();
}

EMEFireMode AMEWeapon::GetFireMode() const
{
	return Data && Data->FireModes.IsValidIndex(FireModeIndex) ? Data->FireModes[FireModeIndex] : EMEFireMode::Single;
}

bool AMEWeapon::CycleFireMode()
{
	if (!Data || Data->FireModes.Num() < 2)
	{
		return false;
	}
	FireModeIndex = (FireModeIndex + 1) % Data->FireModes.Num();
	return true;
}

void AMEWeapon::PlayMuzzleFlash(const FVector& Eye)
{
	if (MuzzleFlash->GetAsset())
	{
		MuzzleFlash->Activate(true);
	}

	if (MuzzleLightPeak <= 0.f)
	{
		return;
	}
	MuzzleEye = Eye;
	MuzzleLightAlpha = 1.f;
	MuzzleLight->SetVisibility(true);
	UpdateMuzzleLightLocation();
	MuzzleLight->SetIntensity(MuzzleLightPeak);
	SetActorTickEnabled(true);
}

void AMEWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	MuzzleLightAlpha = FMath::Max(0.f, MuzzleLightAlpha - DeltaSeconds / (Data ? Data->MuzzleLightDuration : 0.06f));
	if (MuzzleLightAlpha <= 0.f)
	{
		MuzzleLight->SetVisibility(false);
		SetActorTickEnabled(false);
		return;
	}

	UpdateMuzzleLightLocation();
	MuzzleLight->SetIntensity(MuzzleLightPeak * MuzzleLightAlpha);
}

void AMEWeapon::UpdateMuzzleLightLocation()
{
	// Where it was placed in the Blueprint: the real muzzle, out in front of the eye.
	// The weapon is drawn scaled toward the eye, so the real muzzle can be inside a wall: pulled back to our side of it
	MuzzleLight->SetRelativeTransform(MuzzleLightRestTransform);
	const FVector MuzzleLocation = MuzzleLight->GetComponentLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEMuzzleLight), false, this);
	Params.AddIgnoredActor(GetOwner());
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleEye, MuzzleLocation, ECC_Visibility, Params))
	{
		MuzzleLight->SetWorldLocation(Hit.Location - (MuzzleLocation - MuzzleEye).GetSafeNormal() * 10.f);
	}
}
