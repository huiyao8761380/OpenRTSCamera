// Copyright 2024 Jesus Bracho All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InputMappingContext.h"
//#include "Delegates/DelegateCombinations.h"
#include "RTSHUD.h"
#include "RTSSelectable.h"
#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "RTSCamera.generated.h"

/**
 * We use these commands so that move camera inputs can be tied to the tick rate of the game.
 * https://github.com/HeyZoos/OpenRTSCamera/issues/27
 */
USTRUCT()
struct FMoveCameraCommand
{
	GENERATED_BODY()
	UPROPERTY()
	float X = 0;
	UPROPERTY()
	float Y = 0;
	UPROPERTY()
	float Scale = 0;
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OPENRTSCAMERA_API URTSCamera : public UActorComponent
{
	GENERATED_BODY()

public:
	URTSCamera();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void FollowTarget(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void UnFollowTarget();

	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void SetActiveCamera() const;
	
	UFUNCTION(BlueprintCallable, Category = "RTSCamera")
	void JumpTo(FVector Position) const;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Zoom Settings")
	float MinimumZoomLength;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Zoom Settings")
	float MaximumZoomLength;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Zoom Settings")
	float ZoomCatchupSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Zoom Settings")
	float ZoomSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float StartingYAngle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float StartingZAngle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float MoveSpeed;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	float RotateSpeed;
	
	/**
	 * Controls how fast the drag will move the camera.
	 * Higher values will make the camera move more slowly.
	 * The drag speed is calculated as follows:
	 *	DragSpeed = MousePositionDelta / (ViewportExtents * DragExtent)
	 * If the drag extent is small, the drag speed will hit the "max speed" of `this->MoveSpeed` more quickly.
	 */
	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Category = "RTSCamera",
		meta = (ClampMin = "0.0", ClampMax = "1.0")
	)
	float DragExtent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	bool EnableCameraLag;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	bool EnableCameraRotationLag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Dynamic Camera Height Settings")
	bool EnableDynamicCameraHeight;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Dynamic Camera Height Settings")
	TEnumAsByte<ECollisionChannel> CollisionChannel;
	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Category = "RTSCamera - Dynamic Camera Height Settings",
		meta=(EditCondition="EnableDynamicCameraHeight")
	)
	float FindGroundTraceLength;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Edge Scroll Settings")
	bool EnableEdgeScrolling;
	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Category = "RTSCamera - Edge Scroll Settings",
		meta=(EditCondition="EnableEdgeScrolling")
	)
	float EdgeScrollSpeed;
	UPROPERTY(
		BlueprintReadWrite,
		EditAnywhere,
		Category = "RTSCamera - Edge Scroll Settings",
		meta=(EditCondition="EnableEdgeScrolling")
	)
	float DistanceFromEdgeThreshold;




	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputMappingContext* InputMappingContext;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* RotateCameraAxis;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* TurnCameraLeft;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* TurnCameraRight;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* MoveCameraYAxis;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* MoveCameraXAxis;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* DragCamera;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* ZoomCamera;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	bool IsDragging;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	bool IsMove;


	UPROPERTY()
	double RTSMouseLeftMovement;
	UPROPERTY()
	double RTSMouseRightMovement;
	UPROPERTY()
	double RTSMouseUpMovement;
	UPROPERTY()
	double RTSMouseDownMovement;

	UPROPERTY()
	double RTSMouseSelectRate;

	UPROPERTY()
	int32 LastMouseX;
	UPROPERTY()
	int32 LastMouseY;
	UPROPERTY()
	bool bIsFirstTick;

	//��ȡ�������ת
	UPROPERTY()
	FRotator SpringArmLocalRotation;

	int32 DeltaX;
	int32 DeltaY;
	float RotationX;
	float RotationY;

	UPROPERTY()
	double RTSKeyXMovement;
	UPROPERTY()
	double RTSKeyYMovement;


	UPROPERTY()
	bool bIsMoveCameraYAxisCalled = false;
	UPROPERTY()
	bool bIsMoveCameraXAxisCalled = false;

	UPROPERTY()
	bool IsKeyBoardMove;
	UPROPERTY()
	bool IsMouseMove;



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera")
	FVector LastPosition;


	// "RTSSelector.h"

	// BlueprintAssignable allows binding in Blueprints
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorsSelected, const TArray<AActor*>&, SelectedActors);
	UPROPERTY(BlueprintAssignable)
	FOnActorsSelected OnActorsSelected;

	// BlueprintReadWrite allows access and modification in Blueprints
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	//UInputMappingContext* InputMappingContext;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* BeginSelection;

	// Function to clear selected actors, can be overridden in Blueprints
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSCamera - Selection")
	void ClearSelectedActors();

	// Function to handle selected actors, can be overridden in Blueprints
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSCamera - Selection")
	void HandleSelectedActors(const TArray<AActor*>& NewSelectedActors);

	// BlueprintCallable to allow calling from Blueprints
	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnSelectionStart(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnUpdateSelection(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnSelectionEnd(const FInputActionValue& Value);

	UPROPERTY(BlueprintReadOnly, Category = "RTSCamera - Selection")
	TArray<URTSSelectable*> SelectedActors;



protected:
	virtual void BeginPlay() override;

	void OnZoomCamera(const FInputActionValue& Value);
	void OnRotateCamera(const FInputActionValue& Value);
	void OnTurnCameraLeft(const FInputActionValue& Value);
	void OnTurnCameraRight(const FInputActionValue& Value);
	void OnMoveCameraYAxis(const FInputActionValue& Value);
	void OnMoveCameraXAxis(const FInputActionValue& Value);
	void OnDragCamera(const FInputActionValue& Value);

	void RequestMoveCamera(float X, float Y, float Scale);
	void ApplyMoveCameraCommands();

	UPROPERTY()
	AActor* Owner;
	UPROPERTY()
	USceneComponent* Root;
	UPROPERTY()
	UCameraComponent* Camera;
	UPROPERTY()
	USpringArmComponent* SpringArm;
	UPROPERTY()
	APlayerController* PlayerController;
	UPROPERTY()
	AActor* BoundaryVolume;
	UPROPERTY()
	float DesiredZoomLength;

	void ConditionallyPerformEdgeScrolling();


	// "RTSSelector.h"

	//virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent);
	//virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



private:
	void CollectComponentDependencyReferences();
	void ConfigureSpringArm();
	void TryToFindBoundaryVolumeReference();
	void ConditionallyEnableEdgeScrolling() const;
	void CheckForEnhancedInputComponent() const;
	void BindInputMappingContext() const;
	void BindInputActions();

	double EdgeScrollLeft() const;
	double EdgeScrollRight() const;
	double EdgeScrollUp() const;
	double EdgeScrollDown() const;

	void FollowTargetIfSet() const;
	void SmoothTargetArmLengthToDesiredZoom() const;
	void ConditionallyKeepCameraAtDesiredZoomAboveGround();
	void ConditionallyApplyCameraBounds() const;

	UPROPERTY()
	FName CameraBlockingVolumeTag;
	UPROPERTY()
	AActor* CameraFollowTarget;
	UPROPERTY()
	float DeltaSeconds;
	UPROPERTY()
	bool IsCameraOutOfBoundsErrorAlreadyDisplayed;

	UPROPERTY()
	FVector2D DragStartLocation;

	UPROPERTY()
	TArray<FMoveCameraCommand> MoveCameraCommands;


	// "RTSSelector.h"

	//UPROPERTY()
	//APlayerController* PlayerController;

	UPROPERTY()
	ARTSHUD* HUD;

	FVector2D SelectionStart;
	FVector2D SelectionEnd;

	bool bIsSelecting;

	//void BindInputActions();
	//void BindInputMappingContext();
	//void CollectComponentDependencyReferences();


};
