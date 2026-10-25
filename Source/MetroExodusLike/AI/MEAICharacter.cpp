#include "AI/MEAICharacter.h"
#include "AI/MEAIController.h"
#include "AI/MEAIWeaponComponent.h"
#include "AI/MEEnemyProfile.h"
#include "AI/MESquadSubsystem.h"
#include "Survival/METoxicityComponent.h"
#include "Components/SpotLightComponent.h"
#include "Player/MEFootstepComponent.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "MetroExodusLike.h"
#include "Combat/MEHealthComponent.h"
#include "Player/MEAnimInstance.h"
#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AMEAICharacter::AMEAICharacter()
{
	// Only ticks during a hit reaction
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	AIControllerClass = AMEAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Feet on the capsule bottom, facing forward (skeletal meshes face +Y): same as the player
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));
	// Never drawn, but animated every frame: the visible body copies it
	Body->SetHiddenInGame(true);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	Visual = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Body);
	// Bullets hit the visible body's physics bodies, not the capsule
	Visual->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	// Physics too, for the hit reactions; it collides with nothing until it is a ragdoll
	Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Visual->SetCollisionResponseToAllChannels(ECR_Ignore);
	Visual->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	// Turns toward where it walks; the controller turns it while standing (facing a point, looking at a noise)
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->bUseControllerDesiredRotation = true;
	Movement->RotationRate = FRotator(0.f, 240.f, 0.f);
	Movement->MaxWalkSpeed = PatrolSpeed;
	// Ducks behind half cover
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->SetCrouchedHalfHeight(60.f);

	Health = CreateDefaultSubobject<UMEHealthComponent>(TEXT("Health"));
	AIWeapon = CreateDefaultSubobject<UMEAIWeaponComponent>(TEXT("AIWeapon"));
	Toxicity = CreateDefaultSubobject<UMEToxicityComponent>(TEXT("Toxicity"));
	Footsteps = CreateDefaultSubobject<UMEFootstepComponent>(TEXT("Footsteps"));

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(Body);
	Flashlight->SetVisibility(false);
	Flashlight->SetIntensityUnits(ELightUnits::Lumens);
	Flashlight->SetIntensity(1500.f);
	Flashlight->SetAttenuationRadius(2500.f);
	Flashlight->SetInnerConeAngle(10.f);
	Flashlight->SetOuterConeAngle(22.f);
}

float AMEAICharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FString Bone = static_cast<const FPointDamageEvent&>(DamageEvent).HitInfo.BoneName.ToString();
		auto StartsWithAny = [&Bone](std::initializer_list<const TCHAR*> Prefixes)
		{
			for (const TCHAR* Prefix : Prefixes)
			{
				if (Bone.StartsWith(Prefix))
				{
					return true;
				}
			}
			return false;
		};
		if (StartsWithAny({ TEXT("head"), TEXT("neck") }))
		{
			DamageAmount *= HeadDamageScale;
		}
		else if (StartsWithAny({ TEXT("clavicle"), TEXT("upperarm"), TEXT("lowerarm"), TEXT("hand") }))
		{
			DamageAmount *= ArmDamageScale;
		}
		else if (StartsWithAny({ TEXT("thigh"), TEXT("calf"), TEXT("foot"), TEXT("ball") }))
		{
			DamageAmount *= LegDamageScale;
		}
	}
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

const UMEEnemyProfile* AMEAICharacter::GetProfile() const
{
	return Profile ? Profile.Get() : GetDefault<UMEEnemyProfile>();
}

void AMEAICharacter::BeginPlay()
{
	Super::BeginPlay();

	// Without physics bodies the shots could not hit the body: fall back on the capsule
	if (!Visual->GetPhysicsAsset())
	{
		UE_LOG(LogME, Warning, TEXT("%s: visible body has no physics asset, shots hit the capsule."), *GetName());
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	// The retargeting AnimBP reads the animation body's pose: it must be up to date first
	Visual->AddTickPrerequisiteComponent(GetMesh());

	for (USkeletalMesh* PartMesh : BodyParts)
	{
		USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(this);
		Part->SetSkeletalMeshAsset(PartMesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetupAttachment(Visual);
		Part->SetLeaderPoseComponent(Visual);
		Part->RegisterComponent();
	}

	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
	EquipStartingWeapon();
	AIWeapon->SetWeapon(Weapon);
	Toxicity->SetProtected(GetProfile()->bHasGasMask);

	// The light follows the barrel
	if (bFlashlight && Weapon)
	{
		Flashlight->AttachToComponent(Weapon->GetMuzzle(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Flashlight->SetVisibility(true);
	}

	Health->OnDeath.AddUObject(this, &AMEAICharacter::HandleDeath);
	OnTakePointDamage.AddDynamic(this, &AMEAICharacter::HandlePointDamage);
}

void AMEAICharacter::HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* HitComponent,
	FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	if (HitComponent != Visual || BoneName.IsNone())
	{
		return;
	}
	LastHitBone = BoneName;

	if (Health->IsDead())
	{
		// The killing shot pushes the ragdoll
		if (Visual->IsSimulatingPhysics(BoneName))
		{
			Visual->AddImpulse(ShotFromDirection.GetSafeNormal() * DeathImpulse, BoneName, true);
		}
		return;
	}

	// Upper body only: it flinches without stumbling
	Visual->SetAllBodiesBelowSimulatePhysics(HitReactionBone, true, true);
	Visual->SetAllBodiesBelowPhysicsBlendWeight(HitReactionBone, HitReactionBlend, false, true);
	if (Visual->IsSimulatingPhysics(BoneName))
	{
		Visual->AddImpulse(ShotFromDirection.GetSafeNormal() * HitImpulse, BoneName, true);
	}
	HitReactionRemaining = HitReactionTime;
	SetActorTickEnabled(true);
}

void AMEAICharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HitReactionRemaining <= 0.f || Health->IsDead())
	{
		SetActorTickEnabled(false);
		return;
	}

	HitReactionRemaining = FMath::Max(0.f, HitReactionRemaining - DeltaSeconds);
	Visual->SetAllBodiesBelowPhysicsBlendWeight(HitReactionBone, HitReactionBlend * HitReactionRemaining / HitReactionTime, false, true);
	if (HitReactionRemaining <= 0.f)
	{
		Visual->SetAllBodiesBelowSimulatePhysics(HitReactionBone, false, true);
		SetActorTickEnabled(false);
	}
}

void AMEAICharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Weapon)
	{
		Weapon->Destroy();
		Weapon = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AMEAICharacter::EquipStartingWeapon()
{
	if (!StartingWeapon)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Weapon = GetWorld()->SpawnActor<AMEWeapon>(StartingWeapon, Params);
	const UMEWeaponData* Data = Weapon ? Weapon->GetData() : nullptr;
	if (!Data)
	{
		UE_LOG(LogME, Error, TEXT("%s: %s has no weapon Data."), *GetName(), *GetNameSafe(StartingWeapon));
		if (Weapon)
		{
			Weapon->Destroy();
			Weapon = nullptr;
		}
		return;
	}

	Weapon->Initialize(false);
	// Initialize(false) makes it the player's shadow-only weapon: an enemy's is seen normally
	TArray<UPrimitiveComponent*> WeaponParts;
	Weapon->GetComponents<UPrimitiveComponent>(WeaponParts);
	for (UPrimitiveComponent* Part : WeaponParts)
	{
		Part->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	}
	Weapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Data->GripBone);
	Weapon->SetActorRelativeTransform(Data->GripOffset);

	if (UMEAnimInstance* AnimInstance = Cast<UMEAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->SetWeaponPose(Data->BodyHoldPose);
	}
}

void AMEAICharacter::HandleDeath(AController* Killer)
{
	// Those who see it fall know something is wrong; if nobody does, they will miss it later
	const FVector BodyLocation = GetActorLocation();
	bool bSeen = false;
	for (TActorIterator<AMEAICharacter> It(GetWorld()); It; ++It)
	{
		AMEAIController* Other = *It != this ? Cast<AMEAIController>(It->GetController()) : nullptr;
		if (Other && Other->CanSeeLocation(BodyLocation, this))
		{
			Other->NotifyAllyDown(this);
			bSeen = true;
		}
	}
	GetWorld()->GetSubsystem<UMESquadSubsystem>()->EnemyDied(this, bSeen);
	if (USoundBase* DeathCry = GetProfile()->GetBark(EMEBark::Death))
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathCry, BodyLocation + FVector(0.f, 0.f, 70.f));
	}
	AIWeapon->StopFiring();
	Flashlight->SetVisibility(false);

	// The brain stops; the body drops
	if (AMEAIController* Brain = Cast<AMEAIController>(GetController()))
	{
		Brain->StopBrain();
		Brain->UnPossess();
		Brain->Destroy();
	}
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// The weapon stays in the visible hand and falls with the body
	if (Weapon && Weapon->GetAttachParentActor() == this)
	{
		Weapon->AttachToComponent(Visual, FAttachmentTransformRules::KeepWorldTransform, TEXT("hand_r"));
	}

	if (Visual->GetPhysicsAsset())
	{
		Visual->SetCollisionProfileName(TEXT("Ragdoll"));
		Visual->SetSimulatePhysics(true);
	}
	else
	{
		Visual->SetVisibility(false, true);
		if (Weapon)
		{
			Weapon->SetActorHiddenInGame(true);
		}
	}
}

void AMEAICharacter::DropWeapon()
{
	if (!Weapon || Weapon->GetAttachParentActor() != this)
	{
		return;
	}
	AIWeapon->StopFiring();
	Flashlight->SetVisibility(false);

	// Laid flat on the ground in front of it
	const FVector Front = GetActorLocation() + GetActorForwardVector() * 70.f;
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEDropWeapon), false, this);
	Params.AddIgnoredActor(Weapon);
	const bool bGround = GetWorld()->LineTraceSingleByChannel(Ground, Front, Front - FVector(0.f, 0.f, 300.f), ECC_Visibility, Params);
	Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Weapon->SetActorLocationAndRotation((bGround ? Ground.ImpactPoint : Front) + FVector(0.f, 0.f, 3.f), FRotator(0.f, GetActorRotation().Yaw + 90.f, 90.f));
}
