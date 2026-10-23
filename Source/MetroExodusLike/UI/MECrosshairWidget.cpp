#include "UI/MECrosshairWidget.h"
#include "UI/MECrosshairData.h"
#include "Player/MECharacter.h"
#include "Weapons/MEWeaponComponent.h"
#include "Weapons/MEWeaponData.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName TextureParam(TEXT("Texture"));
	const FName ColorParam(TEXT("Color"));
	const FName ImageSizeParam(TEXT("ImageSize"));
	const FName WidgetSizeParam(TEXT("WidgetSize"));
	const FName SpreadParam(TEXT("Spread"));
	const FName CenterRadiusParam(TEXT("CenterRadius"));
}

void UMECrosshairWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	CrosshairImage->SetVisibility(ESlateVisibility::Collapsed);
	HitMarkerImage->SetVisibility(ESlateVisibility::Collapsed);

	if (Material)
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(Material, this);
		CrosshairImage->SetBrushFromMaterial(MaterialInstance);
	}
}

void UMECrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AMECharacter* Character = Cast<AMECharacter>(GetOwningPlayerPawn());
	const UMEWeaponComponent* Weapons = Character ? Character->GetWeapons() : nullptr;
	const UMEWeaponData* WeaponData = Weapons ? Weapons->GetWeaponData() : nullptr;

	// The weapon's own crosshair wins over the default one
	ApplyStyle(WeaponData ? (WeaponData->Crosshair ? WeaponData->Crosshair.Get() : DefaultCrosshair.Get()) : nullptr);
	UpdateHitMarker(InDeltaTime);
	if (!Current)
	{
		return;
	}

	// Bullet cone on screen: tan(angle) over tan(half FOV) gives the share of the half screen width
	const APlayerController* PlayerController = GetOwningPlayer();
	int32 ViewportWidth = 0, ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	const float FieldOfView = PlayerController->PlayerCameraManager ? PlayerController->PlayerCameraManager->GetFOVAngle() : 90.f;
	const float PixelsPerTan = ViewportWidth * 0.5f / FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5f))
		/ FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);

	// At rest (hip, standing still) the image is shown as drawn: only the extra spread moves the parts apart
	const float RestTan = FMath::Tan(FMath::DegreesToRadians(WeaponData->Spread.Hip));
	const float SpreadTan = FMath::Tan(FMath::DegreesToRadians(Weapons->GetSpreadAngle()));
	const float TargetSpread = FMath::Clamp((SpreadTan - RestTan) * PixelsPerTan * Current->SpreadScale, 0.f, Current->MaxSpread);
	Spread = FMath::FInterpTo(Spread, TargetSpread, InDeltaTime, Current->SmoothingSpeed);

	// Gone before the sights reach the eye
	const float Opacity = FMath::Clamp(1.f - Weapons->GetAimAlpha() * 1.5f, 0.f, 1.f);

	if (MaterialInstance)
	{
		MaterialInstance->SetScalarParameterValue(SpreadParam, Spread);
		MaterialInstance->SetVectorParameterValue(ColorParam, FLinearColor(Current->Color.R, Current->Color.G, Current->Color.B, Current->Color.A * Opacity));
	}
}

void UMECrosshairWidget::ShowHitMarker(bool bKilled)
{
	if (!Current || !Current->HitMarkerTexture)
	{
		return;
	}

	// A kill stays a kill until the marker fades, even if a plain hit lands right after
	bHitMarkerKill = bKilled || (bHitMarkerKill && HitMarkerAlpha > 0.f);
	HitMarkerAlpha = 1.f;
	HitMarkerImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMECrosshairWidget::UpdateHitMarker(float DeltaTime)
{
	if (HitMarkerAlpha <= 0.f)
	{
		return;
	}

	HitMarkerAlpha = Current ? FMath::Max(0.f, HitMarkerAlpha - DeltaTime / Current->HitMarkerDuration) : 0.f;
	if (HitMarkerAlpha <= 0.f)
	{
		HitMarkerImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// Pops out and shrinks back while fading, a kill pops bigger
	const float Pop = bHitMarkerKill ? Current->HitMarkerPop * 1.2f : Current->HitMarkerPop;
	HitMarkerImage->SetDesiredSizeOverride(FVector2D(Current->HitMarkerSize * FMath::Lerp(1.f, Pop, HitMarkerAlpha * HitMarkerAlpha)));

	FLinearColor Color = bHitMarkerKill ? Current->KillColor : Current->HitColor;
	Color.A *= HitMarkerAlpha;
	HitMarkerImage->SetColorAndOpacity(Color);
}

void UMECrosshairWidget::ApplyStyle(UMECrosshairData* Style)
{
	if (Style == Current)
	{
		return;
	}
	Current = Style;

	if (Current && Current->HitMarkerTexture)
	{
		HitMarkerImage->SetBrushFromTexture(Current->HitMarkerTexture);
	}

	if (!Current || !Current->Texture || !MaterialInstance)
	{
		CrosshairImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// Room around the image for the parts to move out
	const float WidgetSize = Current->Size + Current->MaxSpread * 2.f;
	CrosshairImage->SetDesiredSizeOverride(FVector2D(WidgetSize));
	MaterialInstance->SetTextureParameterValue(TextureParam, Current->Texture);
	MaterialInstance->SetScalarParameterValue(ImageSizeParam, Current->Size);
	MaterialInstance->SetScalarParameterValue(WidgetSizeParam, WidgetSize);
	MaterialInstance->SetScalarParameterValue(CenterRadiusParam, Current->CenterRadius * Current->Size * 0.5f);
	CrosshairImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}
