#include "Survival/MEGasMaskComponent.h"
#include "MetroExodusLike.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Weapons/MEWeaponComponent.h"
#include "Inventory/MEInventoryComponent.h"
#include "Combat/MEHealthComponent.h"
#include "Survival/METoxicityComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UMEGasMaskComponent::UMEGasMaskComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMEGasMaskComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AMECharacter>(GetOwner());
	// The mask comes with a fresh filter in it
	FilterRemaining = FilterDuration;
}

void UMEGasMaskComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Off first: a stopped breath must not start again
	State = EMEGasMaskState::Off;
	StopBreathing();
	Super::EndPlay(EndPlayReason);
}

int32 UMEGasMaskComponent::GetSpareFilters() const
{
	return Character && FilterItem ? Character->GetInventory()->GetCount(FilterItem) : 0;
}

void UMEGasMaskComponent::MaskPressed()
{
	// A tap toggles, a hold changes the filter: wait to know which
	GetWorld()->GetTimerManager().SetTimer(HoldTimer, this, &UMEGasMaskComponent::ChangeFilter, ChangeFilterHoldTime);
}

void UMEGasMaskComponent::MaskReleased()
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	if (Timers.IsTimerActive(HoldTimer))
	{
		Timers.ClearTimer(HoldTimer);
		ToggleMask();
	}
}

void UMEGasMaskComponent::ToggleMask()
{
	if (State == EMEGasMaskState::Off)
	{
		StartGesture(EMEGasMaskState::PuttingOn, PutOnTime, PutOnSound, &UMEGasMaskComponent::FinishPutOn);
	}
	else if (State == EMEGasMaskState::On)
	{
		StartGesture(EMEGasMaskState::TakingOff, TakeOffTime, TakeOffSound, &UMEGasMaskComponent::FinishTakeOff);
	}
}

void UMEGasMaskComponent::ChangeFilter()
{
	if (State != EMEGasMaskState::On)
	{
		return;
	}
	if (GetSpareFilters() == 0)
	{
		UE_LOG(LogME, Verbose, TEXT("No spare filter."));
		return;
	}
	StartGesture(EMEGasMaskState::ChangingFilter, ChangeFilterTime, ChangeFilterSound, &UMEGasMaskComponent::FinishChangeFilter);
}

bool UMEGasMaskComponent::StartGesture(EMEGasMaskState GestureState, float Duration, USoundBase* Sound, void (UMEGasMaskComponent::*Finish)())
{
	if (!Character || Character->GetHealth()->IsDead())
	{
		return false;
	}

	const float WeaponAway = Character->GetWeapons()->PutAwayFor(Duration);
	if (WeaponAway < 0.f)
	{
		return false;
	}

	State = GestureState;
	SetComponentTickEnabled(true);

	FTimerManager& Timers = GetWorld()->GetTimerManager();
	if (Sound)
	{
		FTimerHandle SoundTimer;
		Timers.SetTimer(SoundTimer, FTimerDelegate::CreateWeakLambda(this, [this, Sound]()
		{
			UGameplayStatics::PlaySound2D(this, Sound);
		}), FMath::Max(WeaponAway, 0.01f), false);
	}
	Timers.SetTimer(GestureTimer, this, Finish, FMath::Max(WeaponAway + Duration, 0.01f));
	return true;
}

void UMEGasMaskComponent::FinishPutOn()
{
	State = EMEGasMaskState::On;
	StartBreathing();
	UpdateProtection();
}

void UMEGasMaskComponent::FinishTakeOff()
{
	State = EMEGasMaskState::Off;
	StopBreathing();
	UpdateProtection();
	SetComponentTickEnabled(false);
}

void UMEGasMaskComponent::FinishChangeFilter()
{
	State = EMEGasMaskState::On;
	// The old filter is thrown away, whatever was left in it
	if (Character->GetInventory()->Remove(FilterItem, 1) > 0)
	{
		FilterRemaining = FilterDuration;
	}
	UpdateProtection();
}

void UMEGasMaskComponent::UpdateProtection()
{
	Character->GetToxicity()->SetProtected(IsWorn() && FilterRemaining > 0.f);
}

void UMEGasMaskComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character->GetHealth()->IsDead())
	{
		GetWorld()->GetTimerManager().ClearTimer(GestureTimer);
		State = EMEGasMaskState::Off;
		StopBreathing();
		SetComponentTickEnabled(false);
		return;
	}

	if (!IsWorn())
	{
		return;
	}

	FilterRemaining = FMath::Max(FilterRemaining - DeltaTime, 0.f);
	UpdateProtection();

	// Out of breath from sprinting, or short of air with a dying filter
	const float Exertion = Character->GetFirstPersonCamera()->GetExertion();
	const float Suffocation = LowFilterTime > 0.f ? 1.f - FMath::Clamp(FilterRemaining / LowFilterTime, 0.f, 1.f) : 0.f;
	BreathEffort = FMath::FInterpConstantTo(BreathEffort, FMath::Max(Exertion, Suffocation), DeltaTime, BreathBlendSpeed);

	if (CalmBreath)
	{
		CalmBreath->SetVolumeMultiplier(FMath::Max(BreathVolume * (1.f - BreathEffort), 0.001f));
	}
	if (HardBreath)
	{
		HardBreath->SetVolumeMultiplier(FMath::Max(BreathVolume * BreathEffort, 0.001f));
	}
}

void UMEGasMaskComponent::StartBreathing()
{
	// Both run all the time the mask is on, only their volume changes: no gap when switching
	if (!CalmBreath && CalmBreathSound)
	{
		CalmBreath = UGameplayStatics::CreateSound2D(this, CalmBreathSound, BreathVolume, 1.f, 0.f, nullptr, false, false);
		CalmBreath->OnAudioFinished.AddDynamic(this, &UMEGasMaskComponent::OnCalmBreathFinished);
	}
	if (!HardBreath && HardBreathSound)
	{
		HardBreath = UGameplayStatics::CreateSound2D(this, HardBreathSound, 0.001f, 1.f, 0.f, nullptr, false, false);
		HardBreath->OnAudioFinished.AddDynamic(this, &UMEGasMaskComponent::OnHardBreathFinished);
	}
	if (CalmBreath)
	{
		CalmBreath->Play();
	}
	if (HardBreath)
	{
		HardBreath->Play();
	}
}

void UMEGasMaskComponent::StopBreathing()
{
	if (CalmBreath)
	{
		CalmBreath->Stop();
	}
	if (HardBreath)
	{
		HardBreath->Stop();
	}
}

void UMEGasMaskComponent::OnCalmBreathFinished()
{
	// Played back to back (a random take each time) while the mask is on
	if (IsWorn())
	{
		CalmBreath->Play();
	}
}

void UMEGasMaskComponent::OnHardBreathFinished()
{
	if (IsWorn())
	{
		HardBreath->Play();
	}
}
