#include "AI/MEAICharacter.h"
#include "AI/MEAIController.h"
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
	PrimaryActorTick.bCanEverTick = false;

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
	Visual->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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

	Health = CreateDefaultSubobject<UMEHealthComponent>(TEXT("Health"));
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

	Health->OnDeath.AddUObject(this, &AMEAICharacter::HandleDeath);
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
	// Those who see it fall know something is wrong
	const FVector BodyLocation = GetActorLocation();
	for (TActorIterator<AMEAICharacter> It(GetWorld()); It; ++It)
	{
		AMEAIController* Other = *It != this ? Cast<AMEAIController>(It->GetController()) : nullptr;
		if (Other && Other->CanSeeLocation(BodyLocation, this))
		{
			Other->NotifyAllyDown(BodyLocation);
		}
	}

	// The brain stops; the body drops (a proper ragdoll and loot come later)
	if (AController* Brain = GetController())
	{
		Brain->UnPossess();
		Brain->Destroy();
	}
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (Visual->GetPhysicsAsset())
	{
		Visual->SetCollisionProfileName(TEXT("Ragdoll"));
		Visual->SetSimulatePhysics(true);
	}
	else
	{
		Visual->SetVisibility(false, true);
	}
	if (Weapon)
	{
		Weapon->SetActorHiddenInGame(true);
	}
}
