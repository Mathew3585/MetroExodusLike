#include "World/METoxicZone.h"
#include "Player/MECharacter.h"
#include "Survival/METoxicityComponent.h"
#include "Components/BoxComponent.h"

AMEToxicZone::AMEToxicZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	Zone->SetBoxExtent(FVector(300.f, 300.f, 150.f));
	Zone->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Zone->SetCollisionResponseToAllChannels(ECR_Ignore);
	Zone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Zone->SetGenerateOverlapEvents(true);
	Zone->ShapeColor = FColor(200, 220, 40);
	Zone->SetHiddenInGame(true);
	RootComponent = Zone;
}

void AMEToxicZone::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (const AMECharacter* Character = Cast<AMECharacter>(OtherActor))
	{
		Character->GetToxicity()->AddToxicZone(this, Density);
	}
}

void AMEToxicZone::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (const AMECharacter* Character = Cast<AMECharacter>(OtherActor))
	{
		Character->GetToxicity()->RemoveToxicZone(this);
	}
}
