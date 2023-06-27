

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JumpComponent.generated.h"
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CRYOFTHEFOX_API UJumpComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UJumpComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Default")
	void Init(float CapsuleRadius, float CapsuleHeight, float OffsetFromCentre, UTexture2D* MarkerImage, FVector Scale);
	
	UFUNCTION(BlueprintCallable, Category = "Jump")
	//The player will either do a normal jump or a calculated jump
	//Make sure to call UpdateJumpCalculations before this or during tick
	void Jump();
	UFUNCTION(BlueprintCallable, Category = "Jump")
	//This function needs to be called when the player lands on the floor
	void Landed();
	UFUNCTION(BlueprintCallable, Category = "Jump")
	//Updates the Jump calculations to see where the player can jump
	void UpdateJumpCalculations();

	UFUNCTION(BlueprintCallable, Category = "Marking")
	//hides the marker for the player jump
	void HideMarker();
	UFUNCTION(BlueprintCallable, Category = "Marking")
	//shows the marker on the place where the player will jump
	void PlaceMarker();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool CalculatedJump = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	bool HasHitAWall = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		//How far to search for a spot horizontally
		float HorizontalDistance = 300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		//how far to search upwards for a spot
		float UpDistance = 300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		//how far to search downwards for a spot
		float DownDistance = 300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		//intervals between the up searches, the lower the value the more performance intensive it becomes
		//value should be smaller then the narrowest gap
		float UpTraceIntervals = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		//intervals between the gaps on the floor, the lower the value the more performance intensive it becomes
		//value should be a tiny bit smaller then the smallest gap you are planning on jumping
		float DownTraceInterval = 40;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		//what should be the minimal difference between two objects to be counted as an elevation to jump up too
		float MinZDiff = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		//what should be the minimal angle between two sides to count as a wall instead of a slope to jump on
		float MinEdgeAngle = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		//offset of the player
		float VerticalOffset = 50;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		float HoriztonalOffset = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Metrics")
		//the angle between the 3 rays being shot out
		//1: shoots out at 0
		//2: shoots out at Angle
		//3: shoots out at Angle * -1
		float Angle = 7.5;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
		//which objects the capsule should check for
		TArray< TEnumAsByte<EObjectTypeQuery>> CanCollideWith{ EObjectTypeQuery::ObjectTypeQuery1, EObjectTypeQuery::ObjectTypeQuery2 };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
		//This is the collision channel that blocks players from jumping there across it
		TEnumAsByte<ECollisionChannel> BlockJump = ECollisionChannel::ECC_GameTraceChannel3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
		//this collision channel is the channel that the line trace will use for finding objects
		TEnumAsByte < ECollisionChannel> TraceChannel = ECollisionChannel::ECC_GameTraceChannel2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool DrawCapsule = false;
private:
	//traces upwards to find a gap in the wall
	//returns true if a place has been found where the player can actually stand
	bool FindJumpLocation(float Angle, FVector& Location, FVector& Normal);
	//pushes the Actor forwards so that it jumps further on the ledge, uses Actor bounds for this
	FVector GetOffset();
	//Does a collision check to make sure this area is clear for the player
	//returns true if player can stand there
	bool CanStandOnLocation(FVector Location, FVector Normal);
	//uses downward traces to find an edge to jump on
	//returns true if there is a valid jump found
	bool FindLocationDownWards(FVector Begin, FVector End, int Attempts,FVector& Location, FVector Normal);
	//Does a singular downtrace
	//returns true if a valid hit has been found
	bool TraceDown(FVector Begin, FVector Direction, FVector& NextLocation, FVector& JumpLocation, FVector& Normal, bool& CancelJump);
	//finds the location between the gap where the player will land
	//returns true if it is a valid area
	bool CheckOpenTop(FVector FirstEmptyLocation, FVector LastLocation, FVector& Location, FVector& Normal);
	//Checks if the found component is allowed to be used for the calculatiosn
	bool IsBlockingHit(UPrimitiveComponent* OutHit);
	//Rotates the line trace towards the given angle
	//only returns the updated EndPoint as Begin point will stay the same
	FVector RotateLineTrace(FVector EndPoint, FVector BeginPoint, float rotation);
	//Does all of the Traces in order
	//First does all the Up Traces
	//if that doesnt return anything it will do all the down traces
	//will use the closest one from each angle
	//returns true if a location has been found
	bool TraceForLines(FVector& Location, FVector& Normal);
	//returns the Begin and end point of the downwards search
	//Also returns how many down traces should be done
	void GetHorizontalTraceBeginEnd(float Angle, FVector& Begin, FVector& End, int& Attempts);
	//Checks to see where the actual edge is of the gap
	//returns true if valid edge has been found
	bool FindDownWardsEdge(FVector Edge, FVector Previous, FVector& Location, FVector& Normal);
	//Check if the Angle between two normals is big enough to be considered a wall and not a slope
	bool IsAngleBigEnough(FVector NormalA, FVector NormalB);
	//gets the start location of the trace
	FVector TraceStartPosition();
	//gets how strong the arch should be based on speed that the character is going 
	float GetArch();

	
	float fCapsuleRadius = 0.f;
	float fCapsuleHeight = 0.f;
	float fOffsetFromCentre = 0.f;

	class AJumpMarker* aMarker;
	class ACharacter* Owner;

	bool IsLedgeJump = false;

	float ArchHeight = 0.5f;
	FVector CalcJumpPos;
};
