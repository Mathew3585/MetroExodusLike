#include "World/MEAlarm.h"
#include "AI/MESquadSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AMEAlarm::AMEAlarm()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

void AMEAlarm::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetSubsystem<UMESquadSubsystem>()->RegisterAlarm(this);
}

void AMEAlarm::Raise(const FVector& ThreatLocation)
{
	if (bRaised)
	{
		return;
	}
	bRaised = true;

	if (AlarmSound)
	{
		Ringing = UGameplayStatics::SpawnSoundAttached(AlarmSound, Mesh);
		GetWorldTimerManager().SetTimer(RingTimer, this, &AMEAlarm::StopRinging, FMath::Max(RingTime, 0.1f));
	}
	GetWorld()->GetSubsystem<UMESquadSubsystem>()->AlarmRaised(ThreatLocation);
}

void AMEAlarm::StopRinging()
{
	if (Ringing)
	{
		Ringing->FadeOut(1.f, 0.f);
		Ringing = nullptr;
	}
}
