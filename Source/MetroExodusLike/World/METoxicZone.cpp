#include "World/METoxicZone.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Obstacle.h"
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

	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
	NavModifier->SetAreaClass(UNavArea_Obstacle::StaticClass());
}

void AMEToxicZone::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (UMEToxicityComponent* Toxicity = OtherActor->FindComponentByClass<UMEToxicityComponent>())
	{
		Toxicity->AddToxicZone(this, Density);
	}
}

void AMEToxicZone::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (UMEToxicityComponent* Toxicity = OtherActor->FindComponentByClass<UMEToxicityComponent>())
	{
		Toxicity->RemoveToxicZone(this);
	}
}
