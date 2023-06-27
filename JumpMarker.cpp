


#include "JumpMarker.h"
#include "Components/BillboardComponent.h"
// Sets default values
AJumpMarker::AJumpMarker()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Marker = CreateDefaultSubobject<UBillboardComponent>(TEXT("Marker"));
}

// Called when the game starts or when spawned
void AJumpMarker::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AJumpMarker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

