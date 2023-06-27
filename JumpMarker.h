#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JumpMarker.generated.h"

UCLASS()
class CRYOFTHEFOX_API AJumpMarker : public AActor
{
	GENERATED_BODY()

	
public:
	// Sets default values for this actor's properties
	AJumpMarker();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBillboardComponent* Marker;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
