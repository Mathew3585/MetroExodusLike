#include "UI/MEHUD.h"
#include "Core/MEGameMode.h"
#include "Player/MECharacter.h"
#include "Combat/MEHealthComponent.h"
#include "Inventory/MEInventoryComponent.h"
#include "Inventory/MEItemData.h"
#include "Survival/MEMedkitComponent.h"
#include "Survival/METoxicityComponent.h"
#include "Survival/MEGasMaskComponent.h"
#include "Survival/MEVisorComponent.h"
#include "Survival/MEFlashlightComponent.h"
#include "Player/MEAimAssistComponent.h"
#include "Stealth/MEVisibilityComponent.h"
#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponComponent.h"
#include "Weapons/MEWeaponData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AI/MEAICharacter.h"
#include "AI/MEAIController.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

void AMEHUD::BeginPlay()
{
	Super::BeginPlay();

	if (const AMEGameMode* GameMode = GetWorld()->GetAuthGameMode<AMEGameMode>())
	{
		bShowDebugPanel = GameMode->ShouldShowDebugAtStart();
	}
}

void AMEHUD::DrawHUD()
{
	Super::DrawHUD();

	if (bShowDebugPanel)
	{
		DrawDebugPanel();
		DrawEnemyPerception();
	}
}

void AMEHUD::DrawEnemyPerception()
{
	if (!Canvas)
	{
		return;
	}

	const UFont* Font = GEngine->GetSmallFont();
	for (TActorIterator<AMEAICharacter> It(GetWorld()); It; ++It)
	{
		const AMEAIController* Brain = Cast<AMEAIController>(It->GetController());
		if (!Brain)
		{
			continue;
		}

		const FVector Screen = Canvas->Project(It->GetActorLocation() + FVector(0.f, 0.f, 120.f));
		if (Screen.Z <= 0.f)
		{
			// Behind the view
			continue;
		}

		// White while unsure, red once spotted
		const float Width = 60.f;
		const float Height = 6.f;
		const float Detection = Brain->GetDetection();
		const FLinearColor Fill = Detection >= 1.f ? FLinearColor::Red : FLinearColor::LerpUsingHSV(FLinearColor::White, FLinearColor(1.f, 0.5f, 0.f), Detection);
		// Yellow frame: on edge (wary or worse)
		if (Brain->GetAlertState() != EMEAlertState::Calm)
		{
			DrawRect(FLinearColor(1.f, 0.85f, 0.f, 0.9f), Screen.X - Width * 0.5f - 2.f, Screen.Y - 2.f, Width + 4.f, Height + 4.f);
		}
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), Screen.X - Width * 0.5f, Screen.Y, Width, Height);
		DrawRect(Fill, Screen.X - Width * 0.5f, Screen.Y, Width * Detection, Height);

		FVector NoiseLocation;
		FName NoiseTag;
		float NoiseAge;
		if (Brain->GetLastNoise(NoiseLocation, NoiseTag, NoiseAge) && NoiseAge < 3.f)
		{
			DrawText(FString::Printf(TEXT("? %s"), *NoiseTag.ToString()), FLinearColor::Yellow, Screen.X - Width * 0.5f, Screen.Y - 16.f, const_cast<UFont*>(Font), TextScale);
		}
	}
}

void AMEHUD::DrawDebugPanel()
{
	const AMECharacter* Character = Cast<AMECharacter>(GetOwningPawn());
	if (!Character || !Canvas)
	{
		return;
	}

	TArray<FString> Lines;

	const UMEHealthComponent* Health = Character->GetHealth();
	Lines.Add(FString::Printf(TEXT("Health    %.0f / %.0f%s"), Health->GetHealth(), Health->GetMaxHealth(), Health->IsDead() ? TEXT("  DEAD") : TEXT("")));
	if (Character->GetMedkit()->IsInUse())
	{
		Lines.Add(TEXT("Medkit    in use"));
	}
	const UMEToxicityComponent* Toxicity = Character->GetToxicity();
	Lines.Add(FString::Printf(TEXT("Toxicity  %.0f %%%s%s"), Toxicity->GetToxicity() * 100.f,
		Toxicity->GetExposure() > 0.f ? *FString::Printf(TEXT("  in gas (density %.1f)"), Toxicity->GetExposure()) : TEXT(""),
		Toxicity->IsProtected() ? TEXT("  PROTECTED") : TEXT("")));
	const UMEGasMaskComponent* GasMask = Character->GetGasMask();
	static const TCHAR* MaskStates[] = { TEXT("off"), TEXT("putting on"), TEXT("ON"), TEXT("taking off"), TEXT("changing filter") };
	const int32 FilterSeconds = FMath::CeilToInt(GasMask->GetFilterRemaining());
	Lines.Add(FString::Printf(TEXT("Gas mask  %s  filter %d:%02d  (%d spare)"), MaskStates[static_cast<int32>(GasMask->GetState())],
		FilterSeconds / 60, FilterSeconds % 60, GasMask->GetSpareFilters()));
	const UMEVisorComponent* Visor = Character->GetVisor();
	Lines.Add(FString::Printf(TEXT("Visor     fog %.0f %%  cracks %d / %d%s"), Visor->GetFog() * 100.f, Visor->GetCracks(), Visor->GetMaxCracks(),
		Visor->IsBroken() ? TEXT("  BROKEN") : TEXT("")));
	Lines.Add(FString::Printf(TEXT("Flashlight %s"), Character->GetFlashlight()->IsOn() ? TEXT("ON") : TEXT("off")));
	const UMEVisibilityComponent* Visibility = Character->GetVisibility();
	Lines.Add(FString::Printf(TEXT("Visibility %.0f %%  light %.1f lux (%d lights)%s%s"), Visibility->GetVisibility() * 100.f, Visibility->GetIlluminance(),
		Visibility->GetLightCount(), Character->GetFlashlight()->IsOn() ? TEXT("  FLASHLIGHT") : TEXT(""), Visibility->IsFlashing() ? TEXT("  SHOT") : TEXT("")));
	const UMEAimAssistComponent* AimAssist = Character->GetAimAssist();
	const AActor* AssistTarget = AimAssist->GetCurrentTarget();
	Lines.Add(FString::Printf(TEXT("Aim assist  %s  target %s  slow x%.2f%s%s"), Character->IsUsingGamepad() ? TEXT("gamepad") : TEXT("mouse"),
		AssistTarget ? *AssistTarget->GetName() : TEXT("-"), AimAssist->GetLookScale(),
		AimAssist->IsSnapping() ? TEXT("  SNAP") : TEXT(""), AimAssist->IsTracking() ? TEXT("  TRACKING") : TEXT("")));

	Lines.Add(TEXT(""));
	Lines.Add(TEXT("Inventory"));
	for (const TPair<TObjectPtr<UMEItemData>, int32>& Item : Character->GetInventory()->GetItems())
	{
		if (Item.Key)
		{
			Lines.Add(FString::Printf(TEXT("  %-20s %d / %d"), *Item.Key->DisplayName.ToString(), Item.Value, Item.Key->MaxCarry));
		}
	}

	const UMEWeaponComponent* Weapons = Character->GetWeapons();
	if (const AMEWeapon* Weapon = Weapons->GetWeapon())
	{
		Lines.Add(TEXT(""));
		Lines.Add(FString::Printf(TEXT("Weapon    %s"), *Weapons->GetWeaponData()->DisplayName.ToString()));
		Lines.Add(FString::Printf(TEXT("  Ammo    %d / %d  (reserve %d)"),
			Weapon->GetAmmoInMagazine(), Weapon->GetAmmoInMagazine() + Weapon->GetRoomInMagazine(), Weapons->GetReserveAmmo()));
		Lines.Add(FString::Printf(TEXT("  Mode    %s"), *UEnum::GetDisplayValueAsText(Weapon->GetFireMode()).ToString()));
		Lines.Add(FString::Printf(TEXT("  Aim %.2f  Spread %.2f deg%s"), Weapons->GetAimAlpha(), Weapons->GetSpreadAngle(),
			Weapons->IsHolstered() ? TEXT("  HOLSTERED (safe zone)") : Weapons->IsBusy() ? TEXT("  BUSY") : TEXT("")));
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const TCHAR* Stance = Movement->IsFalling() ? TEXT("in the air")
		: Character->bIsCrouched ? TEXT("crouched")
		: Character->IsSprinting() ? TEXT("sprinting")
		: TEXT("standing");
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("Move      %.0f cm/s  %s%s"), Movement->Velocity.Size2D(), Stance, Character->IsLandingStunned() ? TEXT("  LANDING") : TEXT("")));

	// The nearest living enemy: what its brain is doing
	const AMEAICharacter* Nearest = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<AMEAICharacter> It(GetWorld()); It; ++It)
	{
		const float Distance = FVector::Dist(It->GetActorLocation(), Character->GetActorLocation());
		if (!It->GetHealth()->IsDead() && Distance < NearestDistance)
		{
			Nearest = *It;
			NearestDistance = Distance;
		}
	}
	if (Nearest)
	{
		static const TCHAR* AlertStates[] = { TEXT("calm"), TEXT("wary"), TEXT("suspicious"), TEXT("alerted"), TEXT("COMBAT"), TEXT("searching"), TEXT("fleeing"), TEXT("surrendered") };
		const AMEAIController* Brain = Cast<AMEAIController>(Nearest->GetController());
		Lines.Add(FString::Printf(TEXT("Enemy     %.0f m  %.0f hp  %s%s"), NearestDistance / 100.f, Nearest->GetHealth()->GetHealth(),
			Brain ? AlertStates[static_cast<int32>(Brain->GetAlertState())] : TEXT("no brain"),
			Brain ? *(TEXT(", ") + Brain->GetActivityDescription()) : TEXT("")));
		if (Brain)
		{
			FVector NoiseLocation;
			FName NoiseTag;
			float NoiseAge;
			const bool bHeard = Brain->GetLastNoise(NoiseLocation, NoiseTag, NoiseAge);
			Lines.Add(FString::Printf(TEXT("  sees you %s  detection %.0f %%  heard %s"),
				Brain->CanSeePlayer() ? TEXT("YES") : Brain->HasPlayerInView() ? TEXT("no (too dark)") : TEXT("no"), Brain->GetDetection() * 100.f,
				bHeard ? *FString::Printf(TEXT("%s %.1f s ago"), *NoiseTag.ToString(), NoiseAge) : TEXT("nothing")));
			Lines.Add(FString::Printf(TEXT("  sight range on you %.1f m%s"), Brain->GetPlayerSightDistance() / 100.f,
				Brain->GetBeamExposure() > 0.f ? TEXT("  sees your beam") : TEXT("")));
		}
	}

	// Dark box behind the text so it reads on any background
	const UFont* Font = GEngine->GetSmallFont();
	const float LineHeight = Font->GetMaxCharHeight() * TextScale;
	float Width = 0.f;
	for (const FString& Line : Lines)
	{
		float LineWidth, LineHeightUnused;
		Canvas->StrLen(Font, Line, LineWidth, LineHeightUnused);
		Width = FMath::Max(Width, LineWidth * TextScale);
	}
	const float Padding = 8.f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), PanelPosition.X, PanelPosition.Y, Width + Padding * 2.f, LineHeight * Lines.Num() + Padding * 2.f);

	float Y = PanelPosition.Y + Padding;
	for (const FString& Line : Lines)
	{
		DrawText(Line, FLinearColor::White, PanelPosition.X + Padding, Y, const_cast<UFont*>(Font), TextScale);
		Y += LineHeight;
	}
}
