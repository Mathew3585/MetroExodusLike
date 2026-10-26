#include "Interaction/MESwitch.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "World/MELamp.h"

#define LOCTEXT_NAMESPACE "MESwitch"

AMESwitch::AMESwitch()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

void AMESwitch::BeginPlay()
{
	Super::BeginPlay();

	ApplyState();
}

FMEInteractionInfo AMESwitch::GetInteractionInfo(const AMECharacter* Interactor) const
{
	FMEInteractionInfo Info;
	Info.Gesture = EMEInteractionGesture::Switch;
	Info.ActionText = bIsOn ? LOCTEXT("TurnOff", "Turn off") : LOCTEXT("TurnOn", "Turn on");
	Info.HoldDuration = HoldDuration;
	return Info;
}

void AMESwitch::Interact(AMECharacter* Interactor)
{
	bIsOn = !bIsOn;
	ApplyState();
}

void AMESwitch::ApplyState()
{
	for (AActor* LightActor : ControlledLights)
	{
		if (!LightActor)
		{
			continue;
		}
		if (AMELamp* Lamp = Cast<AMELamp>(LightActor))
		{
			Lamp->SetOn(bIsOn);
			continue;
		}

		TArray<ULightComponent*> Lights;
		LightActor->GetComponents(Lights);
		for (ULightComponent* Light : Lights)
		{
			Light->SetVisibility(bIsOn);
		}
	}
}

#undef LOCTEXT_NAMESPACE
