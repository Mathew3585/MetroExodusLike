#include "Weapons/MEShellCasing.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AMEShellCasing::AMEShellCasing()
{
	// Ticks only while growing out of the weapon and while shrinking away
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Sized for a 5.56 casing, adjusted in the Blueprint for other calibers
	Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	Collision->InitCapsuleSize(0.48f, 2.15f);
	Collision->SetCollisionObjectType(ECC_PhysicsBody);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Only the level: never blocks the player, the shots or the interaction trace
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	// Small and fast: would go through thin floors without it
	Collision->BodyInstance.bUseCCD = true;
	Collision->BodyInstance.SetMassOverride(0.05f);
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->SetCanEverAffectNavigation(false);
	Collision->SetHiddenInGame(true);
	SetRootComponent(Collision);

	// Casing meshes are modeled along Y: turned to lie along the capsule axis
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetRelativeRotation(FRotationMatrix::MakeFromXY(FVector::XAxisVector, FVector::ZAxisVector).Rotator());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	SetActorHiddenInGame(true);
}

void AMEShellCasing::Launch(const FVector& Location, const FRotator& Rotation, const FVector& Velocity, const FVector& AngularVelocity, float InStartScale)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	bActive = true;
	Age = 0.f;
	StartScale = InStartScale;
	ImpactSoundsPlayed = 0;
	LastImpactSoundTime = -1.0;

	Collision->SetSimulatePhysics(false);
	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::ResetPhysics);
	SetMeshScale(StartScale);
	SetActorHiddenInGame(false);

	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetSimulatePhysics(true);
	Collision->SetPhysicsLinearVelocity(Velocity);
	Collision->SetPhysicsAngularVelocityInDegrees(AngularVelocity);

	SetActorTickEnabled(true);
}

void AMEShellCasing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;
	const float ShrinkStart = Lifetime - ShrinkTime;

	if (Age < GrowTime)
	{
		SetMeshScale(FMath::Lerp(StartScale, 1.f, Age / GrowTime));
	}
	else if (Age < ShrinkStart)
	{
		// Lying on the ground: no need to tick until it starts shrinking
		SetMeshScale(1.f);
		SetActorTickEnabled(false);
		GetWorldTimerManager().SetTimer(LifetimeTimer, FTimerDelegate::CreateWeakLambda(this, [this, ShrinkStart]()
		{
			Age = ShrinkStart;
			SetActorTickEnabled(true);
		}), ShrinkStart - Age, false);
	}
	else if (Age < Lifetime)
	{
		SetMeshScale(1.f - (Age - ShrinkStart) / FMath::Max(ShrinkTime, KINDA_SMALL_NUMBER));
	}
	else
	{
		Deactivate();
	}
}

void AMEShellCasing::Deactivate()
{
	bActive = false;
	SetActorTickEnabled(false);
	GetWorldTimerManager().ClearAllTimersForObject(this);

	Collision->SetSimulatePhysics(false);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorHiddenInGame(true);
}

void AMEShellCasing::SetMeshScale(float Scale)
{
	// Only the visual: the physics body keeps its real size
	Mesh->SetRelativeScale3D(FVector(FMath::Max(Scale, 0.01f)));
}

void AMEShellCasing::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentHit.AddDynamic(this, &AMEShellCasing::HandleHit);
}

void AMEShellCasing::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!ImpactSound || !bActive || ImpactSoundsPlayed >= MaxImpactSounds)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (LastImpactSoundTime >= 0.0 && Now - LastImpactSoundTime < MinTimeBetweenImpactSounds)
	{
		return;
	}

	// Speed into the surface: rolling along the floor is silent
	const float ImpactSpeed = FMath::Abs(FVector::DotProduct(Collision->GetPhysicsLinearVelocity(), Hit.ImpactNormal));
	if (ImpactSpeed < MinImpactSpeed)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint);
	++ImpactSoundsPlayed;
	LastImpactSoundTime = Now;
}
