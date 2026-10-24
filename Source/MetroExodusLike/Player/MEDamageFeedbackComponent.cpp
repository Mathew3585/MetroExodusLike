#include "Player/MEDamageFeedbackComponent.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Combat/MEHealthComponent.h"
#include "Combat/MEDamageType_Toxic.h"
#include "Survival/METoxicityComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

UMEDamageFeedbackComponent::UMEDamageFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMEDamageFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AMECharacter>(GetOwner());
	if (!Character)
	{
		SetComponentTickEnabled(false);
		return;
	}

	Character->GetHealth()->OnHealthChanged.AddUObject(this, &UMEDamageFeedbackComponent::HandleHealthChanged);
	Character->GetHealth()->OnDeath.AddUObject(this, &UMEDamageFeedbackComponent::HandleDeath);

	if (HeartbeatSound)
	{
		Heartbeat = UGameplayStatics::SpawnSound2D(this, HeartbeatSound, 1.f, 1.f, 0.f, nullptr, false, false);
		if (Heartbeat)
		{
			Heartbeat->SetVolumeMultiplier(0.f);
		}
	}
}

void UMEDamageFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Heartbeat)
	{
		Heartbeat->Stop();
	}
	Super::EndPlay(EndPlayReason);
}

void UMEDamageFeedbackComponent::HandleHealthChanged(float Health, float Delta, AController* Instigator)
{
	if (Delta >= 0.f)
	{
		return;
	}

	// Choking on gas: the coughs and the toxic screen say it, no grunt, no jolt
	const UDamageType* DamageType = Character->GetHealth()->GetCurrentDamageType();
	if (DamageType && DamageType->IsA<UMEDamageType_Toxic>())
	{
		return;
	}

	Flash = FMath::Min(1.f, Flash - Delta * FlashPerDamage);
	Character->GetFirstPersonCamera()->AddImpactKick(-Delta * KickPerDamage);
	const float Now = GetWorld()->GetTimeSeconds();
	if (HurtSound && !Character->GetHealth()->IsDead() && Now - LastHurtSoundTime >= HurtSoundCooldown)
	{
		LastHurtSoundTime = Now;
		UGameplayStatics::PlaySound2D(this, HurtSound);
	}
}

void UMEDamageFeedbackComponent::HandleDeath(AController* Killer)
{
	DeathTime = GetWorld()->GetTimeSeconds();
	if (DeathSound)
	{
		UGameplayStatics::PlaySound2D(this, DeathSound);
	}
}

void UMEDamageFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UMEHealthComponent* Health = Character->GetHealth();
	Flash = FMath::Max(0.f, Flash - FlashFadeSpeed * DeltaTime);

	const float TargetLowHealth = LowHealthThreshold > 0.f
		? 1.f - FMath::Clamp(Health->GetHealthPercent() / LowHealthThreshold, 0.f, 1.f)
		: 0.f;
	LowHealth = FMath::FInterpTo(LowHealth, TargetLowHealth, DeltaTime, LowHealthBlendSpeed);

	// Heartbeat: a sharp beat then a slow release, faster near death
	HeartPhase = FMath::Fmod(HeartPhase + DeltaTime * FMath::Lerp(CalmHeartRate, PanicHeartRate, LowHealth), 1.f);
	const float Beat = FMath::Pow(1.f - HeartPhase, 3.f);
	const float Pulse = LowHealth * FMath::Lerp(0.6f, 1.f, Beat);

	const float Fade = DeathTime >= 0.f
		? FMath::Clamp((GetWorld()->GetTimeSeconds() - DeathTime - DeathFadeDelay) / DeathFadeTime, 0.f, 1.f)
		: 0.f;

	const float Toxic = Character->GetToxicity()->GetToxicity();

	// Blended over whatever the level's post-process volumes set
	FPostProcessSettings& Settings = Character->GetFirstPersonCamera()->PostProcessSettings;
	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = 0.4f + Flash * FlashVignette + Pulse * LowHealthVignette + Toxic * ToxicVignette;
	Settings.bOverride_ColorSaturation = true;
	Settings.ColorSaturation = FVector4(FVector(FMath::Lerp(1.f, LowHealthSaturation, LowHealth) * FMath::Lerp(1.f, ToxicSaturation, Toxic)), 1.f);
	Settings.bOverride_SceneColorTint = true;
	Settings.SceneColorTint = FMath::Lerp(FLinearColor::White, FlashTint, Flash) * FMath::Lerp(FLinearColor::White, ToxicTint, Toxic) * (1.f - Fade);
	Settings.SceneColorTint.A = 1.f;
	Settings.bOverride_SceneFringeIntensity = true;
	Settings.SceneFringeIntensity = Toxic * ToxicFringe;

	if (Heartbeat)
	{
		Heartbeat->SetVolumeMultiplier(LowHealth * (1.f - Fade));
	}
}
