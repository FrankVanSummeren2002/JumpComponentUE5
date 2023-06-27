


#include "JumpComponent.h"
#include "JumpMarker.h"
#include "Components/BillboardComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
// Sets default values for this component's properties
UJumpComponent::UJumpComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UJumpComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UJumpComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (HasHitAWall && IsLedgeJump)
		Owner->LaunchCharacter(Owner->GetActorForwardVector() * 3,false,false);
}

void  UJumpComponent::Init(float CapsuleRadius, float CapsuleHeight, float OffsetFromCentre, UTexture2D* MarkerImage, FVector Scale)
{
	fCapsuleRadius = CapsuleRadius;
	fCapsuleHeight = CapsuleHeight;
	fOffsetFromCentre = OffsetFromCentre;

	FRotator myRot(0, 0, 0);
	FVector myLoc(0, 0, 0);

	aMarker = GetWorld()->SpawnActor<AJumpMarker>(AJumpMarker::StaticClass(),myLoc,myRot);
	aMarker->Marker->SetSprite(MarkerImage);
	aMarker->Marker->SetWorldScale3D(Scale);
	HideMarker();
	Owner = Cast<ACharacter> (GetOwner());

	if(!Owner)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Used a CharacterJumpComponent on a non character"));
}

void  UJumpComponent::Jump() 
{
	//dont jump while in air
	if (Owner->GetCharacterMovement()->IsFalling())
		return;

	if (CalculatedJump)
	{
		HasHitAWall = false;
		IsLedgeJump = true;
		FVector vel;
		UGameplayStatics::SuggestProjectileVelocity_CustomArc(Owner, vel, Owner->GetActorLocation(), CalcJumpPos, 0.0f, GetArch());
		Owner->LaunchCharacter(vel * 1.4, true, false);
		FVector dir = vel;
		dir.Z = 0;

		FRotator rot =  dir.Rotation();
		rot.Roll = Owner->GetActorRotation().Roll;
		Owner->SetActorRotation(rot);
	}
	else
		Owner->LaunchCharacter(Owner->GetActorForwardVector() * FVector(500,500,0) + FVector(0,0,200), true, true);
}
void  UJumpComponent::Landed()
{
	HasHitAWall = false;
	IsLedgeJump = false;
}
void  UJumpComponent::UpdateJumpCalculations() 
{
	if (Owner->GetCharacterMovement()->IsFalling())
		HideMarker();
	else
	{
		FVector Location;
		FVector Normal;
		//if nothing found, hide marker
		if (!TraceForLines(Location, Normal))
			HideMarker();
		else
		{
			//make sure the player can stand there
			CalculatedJump = CanStandOnLocation(Location, Normal);
			if (!CalculatedJump)
				HideMarker();

			CalcJumpPos = Location + GetOffset();
		}
	}
}

void  UJumpComponent::HideMarker() 
{
	CalculatedJump = false;
	if (IsValid(aMarker))
		aMarker->Marker->SetVisibility(false);
}
void  UJumpComponent::PlaceMarker() 
{
	if (CalculatedJump)
	{
		if (IsValid(aMarker))
		{
			aMarker->Marker->SetWorldLocation(CalcJumpPos);
			aMarker->Marker->SetVisibility(true); 
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Marker Invalid"));
		}
	}
	else
		HideMarker();
}

bool UJumpComponent::FindJumpLocation(float angle, FVector& Location, FVector& Normal)
{
	int firstIndex = 0;
	int FinalIndex = FMath::RoundToInt(UpDistance / UpTraceIntervals);

	FVector horizontalOffset = Owner->GetActorForwardVector() * FVector(1,1,0);
	horizontalOffset.Normalize();
	horizontalOffset *= horizontalOffset;

	FVector TraceNormal;
	FVector firstEmptyLocation;
	//This while loop is here as we sometimes need to reset the 3 values
	
	while (firstIndex < FinalIndex)
	{
		FVector begin = TraceStartPosition();
		FVector lastLocationRegistered;
		lastLocationRegistered = RotateLineTrace(begin + Owner->GetActorForwardVector() * HorizontalDistance, begin, angle);
		bool registeredHit = false;
		for (int i = firstIndex; i < FinalIndex; i++)
		{
			FHitResult result;
			FVector start = TraceStartPosition() + FVector(0, 0, i * UpTraceIntervals);
			//makes sure on the XY values that we do not go further then the previous one 
			//to make sure that we only look at the first found area
			FVector end = lastLocationRegistered;
			end.Z = start.Z;
			if (this->DrawLines)
			{
				FColor Color = FColor::Red;
				DrawDebugLine(GetWorld(), start, end, Color);
			}
			if (GetWorld()->LineTraceSingleByChannel(result, start, end, TraceChannel))
			{
				if (IsBlockingHit(result.GetComponent()))
					return false;
				registeredHit = true;
				lastLocationRegistered = result.Location + horizontalOffset;
				TraceNormal = result.Normal;
			}
			else
			{
				firstEmptyLocation = end;
				firstEmptyLocation.Z = start.Z;
				//if the first index finds nothing bump it up by one
				if (i == firstIndex)
					i++;
				firstIndex = i;

				//reset the loop if the previous iteration and this one did not find anything
				if (!registeredHit)
					break;


				//check if the area is walkable
				if (!CheckOpenTop(firstEmptyLocation, end + FVector(0,0,VerticalOffset * -1 - 10), Location, Normal))
					return false;
				//if the player has enough space to stand there and it is not a direct slope then jump has been found
				if (IsAngleBigEnough(Normal, TraceNormal) && CanStandOnLocation(Location,Normal))
					return true;

				break;
			}

		}
	}
	return false;
}

FVector UJumpComponent::GetOffset()
{
	FVector Result;
	Owner->GetActorForwardVector().X;
	FVector origin;
	FVector boxExtent;
	Owner->GetActorBounds(true, origin, boxExtent);
	//push the actor forwards a bit based on where they are looking + there size
	//Make sure to elevate as well as the Jump now needs more Z velocity to get there
	Result.X = Owner->GetActorForwardVector().X * boxExtent.X * 0.25f;
	Result.Y = Owner->GetActorForwardVector().Y * boxExtent.Y * 0.25f;
	Result.Z = boxExtent.Z * 0.75f;
	return Result;
}

bool UJumpComponent::CanStandOnLocation(FVector Location, FVector Normal)
{
	//increase the location so that the player capsule is no longer in the center but a bit on top
	FVector location = Location + Normal * (fCapsuleHeight + 5.f);

	//push the player a bit further onto the object
	FVector offset = GetOffset();
	offset.Z = 0;
	//at this point this is just a magic number
	location += offset * Normal + Owner->GetActorForwardVector() * fCapsuleRadius * 0.5f;
	
	//draws the debug capsule
	if (DrawCapsule)
	{
		FQuat rot;
		FColor col = FColor::Red;
		DrawDebugCapsule(GetWorld(), location, fCapsuleHeight, fCapsuleRadius, rot,col);
	}

	TArray<AActor*> ignore;
	TArray<UPrimitiveComponent*> outComp;
	ignore.Add(Owner);

	//overlaps with the world and check if anything blocks me from going there
	UKismetSystemLibrary::CapsuleOverlapComponents(GetWorld(), location, fCapsuleRadius, fCapsuleHeight, CanCollideWith, nullptr, ignore, outComp);
	for (auto temp : outComp)
	{
		if (temp->GetCollisionResponseToChannel(TraceChannel) == ECollisionResponse::ECR_Block)
			return false;
	}

	return true;
}

bool UJumpComponent::FindLocationDownWards(FVector Begin, FVector End, int Attempts, FVector& Location, FVector Normal)
{
	FVector currentLocation = Begin;
	FVector prevLocation = Begin;
	FVector direction = (End - Begin);
	direction.Normalize();

	FVector currentJumpLocation;
	FVector nextJumpLocation;
	FVector nextLocation;
	
	bool cancelJump;
	bool emptyTrace = false;

	TraceDown(currentLocation, direction, nextLocation, nextJumpLocation, Normal, cancelJump);
	if (cancelJump)
		return false;

	currentLocation = nextLocation;
	currentJumpLocation = nextJumpLocation;

	for (int i = 0; i < Attempts; i++)
	{
		bool found = TraceDown(currentLocation, direction, nextLocation, nextJumpLocation, Normal, cancelJump);
		//if there is a border that doesnt allow us to jump return
		if (cancelJump)
			return false;
		//there was no ground found
		if (!found)
		{
			prevLocation = currentLocation;
			currentLocation = nextLocation;
			emptyTrace = true;
		}
		//check if the difference between the ground is big enough
		else if(nextJumpLocation.Z - currentJumpLocation.Z >= MinZDiff || emptyTrace)
		{
			FVector normal;
			bool blocked = !FindDownWardsEdge(nextJumpLocation, prevLocation, Location, normal);
			return IsAngleBigEnough(normal, Normal) && !blocked;
		}
		//update the variables
		else 
		{
			prevLocation = currentLocation;
			currentLocation = nextLocation;
			currentJumpLocation = nextJumpLocation;
		}
	}
	return false;
}

bool UJumpComponent::TraceDown(FVector Begin, FVector Direction, FVector& NextLocation, FVector& JumpLocation, FVector& Normal, bool& CancelJump)
{
	CancelJump = false;
	FHitResult result;
	FVector End = Begin + FVector(0,0,DownDistance * -1.f);
	if (this->DrawLines)
	{
		FColor Color = FColor::Red;
		DrawDebugLine(GetWorld(), Begin, End, Color);
	}
	GetWorld()->LineTraceSingleByChannel(result, Begin, End, TraceChannel);

	//make sure Direction doesnt have a z value and is normalized
	Direction.Z = 0;
	Direction.Normalize();

	NextLocation = Begin + Direction * DownTraceInterval;
	
	//doesnt find anything but do not cancel jump
	if(!result.bBlockingHit)
		return false;
	//return if we are not allowed to be there
	if (IsBlockingHit(result.GetComponent()))
	{
		CancelJump = true;
		return false;
	}

	JumpLocation = result.Location;
	Normal = result.Normal;

	return result.bBlockingHit;

}

bool UJumpComponent::CheckOpenTop(FVector FirstEmptyLocation, FVector LastLocation, FVector& Location, FVector& Normal)
{
	FHitResult result;
	GetWorld()->LineTraceSingleByChannel(result, FirstEmptyLocation, LastLocation, TraceChannel);
	if (this->DrawLines)
	{
		FColor Color = FColor::Red;
		DrawDebugLine(GetWorld(), FirstEmptyLocation, LastLocation, Color);
	}
	//check if there is an object and check if you are allowed to be on top of it
	if (!result.bBlockingHit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("No blocking hit"));
		return false;
	}
	if (IsBlockingHit(result.GetComponent()))
	{
		
		return false;
	}

	Location = result.Location;
	Normal = result.Normal;
	return true;
}

bool UJumpComponent::IsBlockingHit(UPrimitiveComponent* OutHit)
{
	return OutHit->GetCollisionResponseToChannel(BlockJump) == ECollisionResponse::ECR_Block;
}

FVector UJumpComponent::RotateLineTrace(FVector EndPoint, FVector BeginPoint, float rotation)
{
	FVector result = EndPoint - BeginPoint;
	return BeginPoint + result.RotateAngleAxis(rotation, FVector(0, 0, 1));
}



bool UJumpComponent::TraceForLines(FVector& Location, FVector& Normal)
{
	bool Found = false;
	//First try finding by the upwards traces only
	if (FindJumpLocation(0.f, Location, Normal))
		Found = true;
	
	if (FindJumpLocation(Angle, Location, Normal))
		Found = true;
	if (FindJumpLocation(Angle * -1.f, Location, Normal))
		Found = true;
	
	if (Found)
		return true;	

	//if not found also try looking for the downwards traces
	
	FVector Begin, End;
	int attempts;
	GetHorizontalTraceBeginEnd(0.f, Begin, End, attempts);
	if (FindLocationDownWards(Begin, End, attempts, Location, Normal))
		Found = true;
	GetHorizontalTraceBeginEnd(Angle, Begin, End, attempts);
	if (FindLocationDownWards(Begin, End, attempts, Location, Normal))
		Found = true;
	GetHorizontalTraceBeginEnd(Angle * -1.f, Begin, End, attempts);
	if (FindLocationDownWards(Begin, End, attempts, Location, Normal))
		Found = true;

	return Found;
}

void UJumpComponent::GetHorizontalTraceBeginEnd(float angle, FVector& Begin, FVector& End, int& Attempts)
{
	//The down traces should start a bit higher to make sure everything is caught
	Begin = TraceStartPosition() + FVector(0,0,fCapsuleHeight * 1.5);
	End = RotateLineTrace(Begin + Owner->GetActorForwardVector() * HorizontalDistance, Begin, angle);
	Attempts = FMath::Floor(HorizontalDistance / DownTraceInterval) - 1;
}

bool UJumpComponent::FindDownWardsEdge(FVector Edge, FVector Previous, FVector& Location, FVector& Normal)
{
	//slightly lower the value to make sure we hit the floor
	Edge.Z -= 1;

	Previous.Z = Edge.Z;
	FHitResult result;
	//if the line trace doesn't find anything something went wrong

	if (this->DrawLines)
	{
		FColor Color = FColor::Red;
		DrawDebugLine(GetWorld(), Edge, Previous, Color);
	}
	if (!GetWorld()->LineTraceSingleByChannel(result, Previous, Edge, ECC_WorldStatic))
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("no edge found"));
		return false;
	}
	Location = result.Location;
	Normal = result.Normal;
	return !IsBlockingHit(result.Component.Get());

}

bool UJumpComponent::IsAngleBigEnough(FVector NormalA, FVector NormalB)
{
	return FMath::Abs(FMath::RadiansToDegrees(FMath::Acos(NormalA.Dot(NormalB)))) >= MinEdgeAngle;
}

FVector UJumpComponent::TraceStartPosition()
{
	return Owner->GetActorLocation() + Owner->GetActorForwardVector() * fOffsetFromCentre;
}

float UJumpComponent::GetArch()
{
	float zDiff = (CalcJumpPos - Owner->GetActorLocation()).Z;
	
	if (FMath::Abs(zDiff) < 0.1f)
		return ArchHeight;
	FVector begin = Owner->GetActorLocation() + Owner->GetActorForwardVector() * fOffsetFromCentre * FMath::Sign(zDiff);
	FVector End = CalcJumpPos - FVector(0, 0, 5);

	FHitResult result;
	if (this->DrawLines)
	{
		FColor Color = FColor::Red;
		DrawDebugLine(GetWorld(), begin, End, Color);
	}
	if (GetWorld()->LineTraceSingleByChannel(result, begin, End, TraceChannel))
		return -0.2 + ArchHeight;

	
	return FMath::Clamp(Owner->GetVelocity().Length() / 500.f, 0.f, 0.2f) + ArchHeight;
}
