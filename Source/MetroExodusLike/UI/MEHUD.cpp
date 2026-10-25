#include "UI/MEHUD.h"
#include "Core/MEGameMode.h"
#include "Player/MECharacter.h"
#include "Combat/MEHealthComponent.h"
#include "Inventory/MEInventoryComponent.h"
#include "Inventory/MEItemData.h"
#include "Survival/MEMedkitComponent.h"
#include "Survival/METoxicityComponent.h"
#include "Survival/MEGasMaskComponent.h"
#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponComponent.h"
#include "Weapons/MEWeaponData.h"
#include "GameFramework/CharacterMovementComponent.h"
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
