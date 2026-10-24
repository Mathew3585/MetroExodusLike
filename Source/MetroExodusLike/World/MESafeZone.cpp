#include "World/MESafeZone.h"
#include "Player/MECharacter.h"
#include "Weapons/MEWeaponComponent.h"
#include "Components/BoxComponent.h"

AMESafeZone::AMESafeZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	Zone->SetBoxExtent(FVector(300.f, 300.f, 150.f));
	Zone->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Zone->SetCollisionResponseToAllChannels(ECR_Ignore);
	Zone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Zone->SetGenerateOverlapEvents(true);
	Zone->ShapeColor = FColor(80, 220, 120);
	Zone->SetHiddenInGame(true);
	RootComponent = Zone;
}

void AMESafeZone::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (const AMECharacter* Character = Cast<AMECharacter>(OtherActor))
	{
		Character->GetWeapons()->AddHolsterRequest();
	}
}

void AMESafeZone::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (const AMECharacter* Character = Cast<AMECharacter>(OtherActor))
	{
		Character->GetWeapons()->RemoveHolsterRequest();
	}
}
