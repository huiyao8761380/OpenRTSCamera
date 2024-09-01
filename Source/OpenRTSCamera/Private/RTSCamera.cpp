// Copyright 2024 Jesus Bracho All Rights Reserved.

#include "RTSCamera.h"
#include "UObject/ConstructorHelpers.h"
//#include "Math/UnrealMathUtility.h" // For FMath::Pow
//#include "Delegates/DelegateCombinations.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "RTSSelectable.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
//#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"


URTSCamera::URTSCamera()
{
	PrimaryComponentTick.bCanEverTick = true;
	this->CameraBlockingVolumeTag = FName("OpenRTSCamera#CameraBounds");
	this->CollisionChannel = ECC_WorldStatic;
	this->DragExtent = 0.6f;
	this->EdgeScrollSpeed = 20000;
	this->MoveSpeed = 20000;
	this->DistanceFromEdgeThreshold = 0.1f;
	this->EnableCameraLag = true;
	this->EnableCameraRotationLag = true;
	this->EnableDynamicCameraHeight = true;
	this->EnableEdgeScrolling = true;
	this->FindGroundTraceLength = 100000;
	this->MaximumZoomLength = 10000;//5000
	this->MinimumZoomLength = 100;
	this->RotateSpeed = 45;
	this->StartingYAngle = -45.0f;
	this->StartingZAngle = 0;
	this->ZoomCatchupSpeed = 4;
	this->ZoomSpeed = -200;

	static ConstructorHelpers::FObjectFinder<UInputAction>
		MoveCameraXAxisFinder(TEXT("/OpenRTSCamera/Inputs/MoveCameraXAxis"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		MoveCameraYAxisFinder(TEXT("/OpenRTSCamera/Inputs/MoveCameraYAxis"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		RotateCameraAxisFinder(TEXT("/OpenRTSCamera/Inputs/RotateCameraAxis"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		TurnCameraLeftFinder(TEXT("/OpenRTSCamera/Inputs/TurnCameraLeft"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		TurnCameraRightFinder(TEXT("/OpenRTSCamera/Inputs/TurnCameraRight"));
	static ConstructorHelpers::FObjectFinder<UInputAction>
		ZoomCameraFinder(TEXT("/OpenRTSCamera/Inputs/ZoomCamera"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext>
		InputMappingContextFinder(TEXT("/OpenRTSCamera/Inputs/OpenRTSCameraInputs"));

	this->MoveCameraXAxis = MoveCameraXAxisFinder.Object;
	this->MoveCameraYAxis = MoveCameraYAxisFinder.Object;
	this->RotateCameraAxis = RotateCameraAxisFinder.Object;
	this->TurnCameraLeft = TurnCameraLeftFinder.Object;
	this->TurnCameraRight = TurnCameraRightFinder.Object;
	this->ZoomCamera = ZoomCameraFinder.Object;
	this->InputMappingContext = InputMappingContextFinder.Object;



	//"RTSSelector.h"

	PrimaryComponentTick.bCanEverTick = true;
	LastMouseX = 0.0f;
	LastMouseY = 0.0f;
	bIsFirstTick = true;


	// Add defaults for input actions
	static ConstructorHelpers::FObjectFinder<UInputAction>
		BeginSelectionActionFinder(TEXT("/OpenRTSCamera/Inputs/BeginSelection"));

	this->BeginSelection = BeginSelectionActionFinder.Object;
	//this->InputMappingContext = InputMappingContextFinder.Object;

}

void URTSCamera::BeginPlay()
{
	Super::BeginPlay();

	const auto NetMode = this->GetNetMode();
	if (NetMode != NM_DedicatedServer)
	{
		this->CollectComponentDependencyReferences();
		this->ConfigureSpringArm();
		this->TryToFindBoundaryVolumeReference();
		this->ConditionallyEnableEdgeScrolling();
		this->CheckForEnhancedInputComponent();
		this->BindInputMappingContext();
		this->BindInputActions();


		//"RTSSelector.h"

		OnActorsSelected.AddDynamic(this, &URTSCamera::HandleSelectedActors);
		UE_LOG(LogTemp, Warning, TEXT("NetMode != NM_DedicatedServer"));
	}
}

void URTSCamera::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const auto NetMode = this->GetNetMode();
	if (NetMode != NM_DedicatedServer && this->PlayerController->GetViewTarget() == this->Owner)
	{
		this->DeltaSeconds = DeltaTime;
		this->ApplyMoveCameraCommands();
		this->ConditionallyPerformEdgeScrolling();
		this->ConditionallyKeepCameraAtDesiredZoomAboveGround();
		this->SmoothTargetArmLengthToDesiredZoom();
		this->FollowTargetIfSet();
		this->ConditionallyApplyCameraBounds();

		

		if (bIsMoveCameraYAxisCalled || bIsMoveCameraXAxisCalled)
		{
			// 方法被调用
			IsMove = true;
			IsKeyBoardMove = true;
		}
		else
		{
			// 方法未被调用
			IsMove = false;
			IsKeyBoardMove = false;
			RTSKeyXMovement = 0;
			RTSKeyYMovement = 0;
		}

		if (RTSMouseLeftMovement || RTSMouseRightMovement || RTSMouseUpMovement || RTSMouseDownMovement)
		{
			IsMove = true;
			IsMouseMove = true;
		}
		else
		{
			// 方法未被调用
			IsMove = false;
			IsMouseMove = false;
		}
		
		//UE_LOG(LogTemp, Warning, TEXT("IsMove: %s  IsKeyBoardMove: %s  IsMouseMove: %s"), IsMove ? TEXT("1") : TEXT("0"), IsKeyBoardMove ? TEXT("1") : TEXT("0"), IsMouseMove ? TEXT("1") : TEXT("0"));
		//UE_LOG(LogTemp, Warning, TEXT("MouseLeft: %f  MouseRight: %f  MouseUp: %f  MouseDown: %f,KeyX: %f,KeyY: %f"), RTSMouseLeftMovement, RTSMouseRightMovement, RTSMouseUpMovement, RTSMouseDownMovement, RTSKeyXMovement, RTSKeyYMovement);
		
		//UE_LOG(LogTemp, Warning, TEXT("bIsDrawingSelectionBox: %s ,bIsPerformingSelection: %s "), HUD->bIsDrawingSelectionBox ? TEXT("1") : TEXT("0"), HUD->bIsPerformingSelection ? TEXT("1") : TEXT("0"));


		// 重置标志
		bIsMoveCameraYAxisCalled = false;
		bIsMoveCameraXAxisCalled = false;
		IsMove = false;
		IsMouseMove = false;


	}
	/*
	APawn* ControlledPawn = this->PlayerController->GetPawn();
		// 获取当前位置
	if (ControlledPawn)
	{
		// 获取当前位置
		FVector CurrentPosition = ControlledPawn->GetActorLocation();

		// 检查位置是否发生变化
		if (CurrentPosition != LastPosition)
		{
			// 位置发生变化，表示Pawn移动了
			// 执行相应的操作
			this->IsMove = true;
		}
		else 
		{
			this->IsMove = false;
		}

		// 保存当前位置用于下一帧的比较
		LastPosition = CurrentPosition;
	}
	//UE_LOG(LogTemp, Warning, TEXT("IsMove: %s"), IsMove ? TEXT("true") : TEXT("false"));
	*/

}



//"RTSSelector.h"

void URTSCamera::HandleSelectedActors_Implementation(const TArray<AActor*>& NewSelectedActors)//HandleSelectedActors_Implementation
{
	
	// Convert NewSelectedActors to a set for efficient lookup 将新选定的演员转换为一个集合，以便高效查找。
	TSet<AActor*> NewSelectedActorSet;
	for (const auto& Actor : NewSelectedActors)
	{
		NewSelectedActorSet.Add(Actor);
		//UE_LOG(LogTemp, Warning, TEXT("NewSelectedActorSet"));
	}

	// Iterate over currently selected actors 循环遍历当前选中的演员。 清除旧选新的才使用
	for (const auto& Selected : SelectedActors)//SelectedActors
	{
		// Check if the actor is not in the new selection 检查演员是否不在新选择中
		if (!NewSelectedActorSet.Contains(Selected->GetOwner()))
		{
			// Call OnDeselected for actors that are no longer selected 对不再被选中的演员调用取消选中
			Selected->OnDeselected();
		}
	}

	// Clear the current selection 清除当前选择
	ClearSelectedActors();

	// Add new selected actors and call OnSelected 添加新的选定演员并调用OnSelected
	for (const auto& Actor : NewSelectedActors)
	{
		if (URTSSelectable* SelectableComponent = Actor->FindComponentByClass<URTSSelectable>())
		{
			this->SelectedActors.Add(SelectableComponent);
			SelectableComponent->OnSelected();
		}
	}


	
}

void URTSCamera::ClearSelectedActors_Implementation()//ClearSelectedActors_Implementation
{
	this->SelectedActors.Empty();
}

void URTSCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (const auto InputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		InputComponent->BindAction(this->BeginSelection, ETriggerEvent::Started, this, &URTSCamera::OnSelectionStart);
		InputComponent->BindAction(this->BeginSelection, ETriggerEvent::Completed, this, &URTSCamera::OnSelectionEnd);
	}
}

//"RTSSelector.h"






void URTSCamera::FollowTarget(AActor* Target)
{
	this->CameraFollowTarget = Target;
}

void URTSCamera::UnFollowTarget()
{
	this->CameraFollowTarget = nullptr;
}

void URTSCamera::OnZoomCamera(const FInputActionValue& Value)
{	
	this->ZoomSpeed = -20 - this->DesiredZoomLength / 20;
	this->MoveSpeed = this->DesiredZoomLength * 2;
	this->EdgeScrollSpeed = this->MoveSpeed;

	this->DesiredZoomLength = FMath::Clamp(
		this->DesiredZoomLength + Value.Get<float>() * this->ZoomSpeed,
		this->MinimumZoomLength,
		this->MaximumZoomLength
	);
	UE_LOG(LogTemp, Warning, TEXT("ZoomSpeed: %f,MoveSpeed: %f,DesiredZoomLength: %f"), this->ZoomSpeed, this->MoveSpeed, this->DesiredZoomLength);
}

void URTSCamera::OnRotateCamera(const FInputActionValue& Value)
{
	const auto WorldRotation = this->Root->GetComponentRotation();
	//const auto SpringArmRotation = this->SpringArm->GetComponentRotation();
	//APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (this->PlayerController)
	{
		// 获取当前鼠标位置
		float MouseX, MouseY, RotationRate;
		RotationRate= this->DesiredZoomLength / 20000;
		this->PlayerController->GetMousePosition(MouseX, MouseY);

		// 比较当前帧和上一帧的鼠标位置
		if (bIsFirstTick)
		{
			LastMouseX = MouseX;
			LastMouseY = MouseY;
			bIsFirstTick = false;
		}
		else
		{
			DeltaX = MouseX - LastMouseX;
			DeltaY = MouseY - LastMouseY;

			if (DeltaX > 0 or RTSMouseRightMovement)
			{
				RotationX =0.5 + RotationRate;// 1;//Value.Get<float>();
				//UE_LOG(LogTemp, Log, TEXT("Mouse moved right"));
			}
			else if (DeltaX < 0 or RTSMouseLeftMovement)
			{
				RotationX = -(0.5 + RotationRate);// -Value.Get<float>();
				//UE_LOG(LogTemp, Log, TEXT("Mouse moved left"));

			}
			else
			{
				RotationX = 0;
			}

			if (DeltaY > 0 or RTSMouseDownMovement)
			{
				RotationY = 0.4 + RotationRate;//设置上下旋转速度

			}
			else if (DeltaY < 0 or RTSMouseUpMovement)
			{
				RotationY = -(0.4 + RotationRate);
			}
			else
			{
				RotationY = 0;
			}

			// 更新上一帧的鼠标位置
			LastMouseX = MouseX;
			LastMouseY = MouseY;
		}
	}


	SpringArmLocalRotation = this->SpringArm->GetRelativeRotation();
	UE_LOG(LogTemp, Warning, TEXT("Spring Arm Local Rotation: Pitch: %f, Yaw: %f, Roll: %f"),
		SpringArmLocalRotation.Pitch, SpringArmLocalRotation.Yaw, SpringArmLocalRotation.Roll);


	/*
	UGameViewportClient* SceneViewport = GetWorld()->GetGameViewport();
	FViewport * Viewport = SceneViewport->Viewport;
	int32 HitX;
	int32 HitY;
	if (Viewport)
	{
		HitX = Viewport->GetMouseX();// Viewport->GetMousePos()//
		HitY = Viewport->GetMouseY();
	}
	*/
	this->SpringArm->AddLocalRotation(
		FRotator::MakeFromEuler(
			FVector(
				0,
				RotationY,
				0
			)
		)
	);
	this->Root->SetWorldRotation(
		FRotator::MakeFromEuler(
			FVector(
				WorldRotation.Euler().X,
				WorldRotation.Euler().Y,
				WorldRotation.Euler().Z + RotationX//Value -1
			)
		)
	);
	
	//this->SpringArm->SetRelativeRotation();


	//UE_LOG(LogTemp, Warning, TEXT("Z: %f, Value: %f, DeltaX:%d, DeltaY: %d  "), WorldRotation.Euler().Z, Value.Get<float>(), DeltaX, DeltaY);
}

void URTSCamera::OnTurnCameraLeft(const FInputActionValue&)
{
	const auto WorldRotation = this->Root->GetRelativeRotation();
	this->Root->SetRelativeRotation(
		FRotator::MakeFromEuler(
			FVector(
				WorldRotation.Euler().X,
				WorldRotation.Euler().Y,
				WorldRotation.Euler().Z - this->RotateSpeed
			)
		)
	);
}

void URTSCamera::OnTurnCameraRight(const FInputActionValue&)
{
	const auto WorldRotation = this->Root->GetRelativeRotation();
	this->Root->SetRelativeRotation(
		FRotator::MakeFromEuler(
			FVector(
				WorldRotation.Euler().X,
				WorldRotation.Euler().Y,
				WorldRotation.Euler().Z + this->RotateSpeed
			)
		)
	);
}

void URTSCamera::OnMoveCameraYAxis(const FInputActionValue& Value)
{
	float YAxisValue = Value.Get<float>();
	//SpringArmLocalRotation = this->SpringArm->GetRelativeRotation();
	if (SpringArmLocalRotation.Yaw >= 179.0 || SpringArmLocalRotation.Yaw <= -179.0)
	{
		YAxisValue = 0 - YAxisValue;
		//UE_LOG(LogTemp, Warning, TEXT("Spring Arm Local Rotation: Yaw: %f"), SpringArmLocalRotation.Yaw);
	}

	this->RequestMoveCamera(
		this->SpringArm->GetForwardVector().X,
		this->SpringArm->GetForwardVector().Y,
		YAxisValue
	);

	bIsMoveCameraYAxisCalled = true;

	RTSKeyYMovement = YAxisValue;

	
}

void URTSCamera::OnMoveCameraXAxis(const FInputActionValue& Value)
{
	this->RequestMoveCamera(
		this->SpringArm->GetRightVector().X,
		this->SpringArm->GetRightVector().Y,
		Value.Get<float>()
	);

	bIsMoveCameraXAxisCalled = true;

	RTSKeyXMovement = Value.Get<float>();

}

void URTSCamera::OnDragCamera(const FInputActionValue& Value)
{
	if (!this->IsDragging && Value.Get<bool>())
	{
		this->IsDragging = true;
		this->DragStartLocation = UWidgetLayoutLibrary::GetMousePositionOnViewport(this->GetWorld());
	}

	else if (this->IsDragging && Value.Get<bool>())
	{
		const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this->GetWorld());
		auto DragExtents = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this->GetWorld()).GetLocalSize();
		DragExtents *= DragExtent;

		auto Delta = MousePosition - this->DragStartLocation;
		Delta.X = FMath::Clamp(Delta.X, -DragExtents.X, DragExtents.X) / DragExtents.X;
		Delta.Y = FMath::Clamp(Delta.Y, -DragExtents.Y, DragExtents.Y) / DragExtents.Y;

		this->RequestMoveCamera(
			this->SpringArm->GetRightVector().X,
			this->SpringArm->GetRightVector().Y,
			Delta.X
		);

		this->RequestMoveCamera(
			this->SpringArm->GetForwardVector().X,
			this->SpringArm->GetForwardVector().Y,
			Delta.Y * -1
		);
	}

	else if (this->IsDragging && !Value.Get<bool>())
	{
		this->IsDragging = false;
	}
	
}

void URTSCamera::RequestMoveCamera(const float X, const float Y, const float Scale)
{
	FMoveCameraCommand MoveCameraCommand;
	MoveCameraCommand.X = X;
	MoveCameraCommand.Y = Y;
	MoveCameraCommand.Scale = Scale;
	MoveCameraCommands.Push(MoveCameraCommand);
	//UE_LOG(LogTemp, Warning, TEXT("RequestMoveCamera: %f,%f,%f"), MoveCameraCommand.X, MoveCameraCommand.Y, MoveCameraCommand.Scale);//方向
}

void URTSCamera::ApplyMoveCameraCommands()
{
	for (const auto& [X, Y, Scale] : this->MoveCameraCommands)
	{
		auto Movement = FVector2D(X, Y);
		Movement.Normalize();
		Movement *= this->MoveSpeed * Scale * this->DeltaSeconds;
		this->Root->SetWorldLocation(
			this->Root->GetComponentLocation() + FVector(Movement.X, Movement.Y, 0.0f)
		);
		//UE_LOG(LogTemp, Warning, TEXT("Movement: %f,%f"), Movement.X, Movement.Y);
	}
	
	this->MoveCameraCommands.Empty();
}

void URTSCamera::CollectComponentDependencyReferences()
{
	//初始化的情况
	this->Owner = this->GetOwner();
	this->Root = this->Owner->GetRootComponent();
	this->Camera = Cast<UCameraComponent>(this->Owner->GetComponentByClass(UCameraComponent::StaticClass()));
	this->SpringArm = Cast<USpringArmComponent>(this->Owner->GetComponentByClass(USpringArmComponent::StaticClass()));
	this->PlayerController = UGameplayStatics::GetPlayerController(this->GetWorld(), 0);


	//this->HUD = Cast<ARTSHUD>(this->PlayerController->GetHUD());

	//"RTSSelector.h"
	
	if (const auto PlayerControllerRef = UGameplayStatics::GetPlayerController(this->GetWorld(), 0))
	{
		this->PlayerController = PlayerControllerRef;
		this->HUD = Cast<ARTSHUD>(PlayerControllerRef->GetHUD());
		//UE_LOG(LogTemp, Error, TEXT("USelector!!!!!!!!!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USelector is not attached to a PlayerController!!!!!!!!!"));
	}
	
}

void URTSCamera::ConfigureSpringArm()
{
	this->DesiredZoomLength = this->MaximumZoomLength;
	this->SpringArm->TargetArmLength = this->DesiredZoomLength;
	this->SpringArm->bDoCollisionTest = false;
	this->SpringArm->bEnableCameraLag = this->EnableCameraLag;
	this->SpringArm->bEnableCameraRotationLag = this->EnableCameraRotationLag;
	this->SpringArm->SetRelativeRotation(
		FRotator::MakeFromEuler(
			FVector(
				0.0,
				this->StartingYAngle,
				this->StartingZAngle
			)
		)
	);
}

void URTSCamera::TryToFindBoundaryVolumeReference()
{
	TArray<AActor*> BlockingVolumes;
	UGameplayStatics::GetAllActorsOfClassWithTag(
		this->GetWorld(),
		AActor::StaticClass(),
		this->CameraBlockingVolumeTag,
		BlockingVolumes
	);

	if (BlockingVolumes.Num() > 0)
	{
		this->BoundaryVolume = BlockingVolumes[0];
	}
}

void URTSCamera::ConditionallyEnableEdgeScrolling() const
{
	if (this->EnableEdgeScrolling)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetHideCursorDuringCapture(false);
		this->PlayerController->SetInputMode(InputMode);
	}
}

void URTSCamera::CheckForEnhancedInputComponent() const
{
	if (Cast<UEnhancedInputComponent>(this->PlayerController->InputComponent) == nullptr)
	{
		UKismetSystemLibrary::PrintString(
			this->GetWorld(),
			TEXT("Set Edit > Project Settings > Input > Default Classes to Enhanced Input Classes"), true, true,
			FLinearColor::Red,
			100
		);

		UKismetSystemLibrary::PrintString(
			this->GetWorld(),
			TEXT("Keyboard inputs will probably not function."), true, true,
			FLinearColor::Red,
			100
		);

		UKismetSystemLibrary::PrintString(
			this->GetWorld(),
			TEXT("Error: Enhanced input component not found."), true, true,
			FLinearColor::Red,
			100
		);
	}
}

void URTSCamera::BindInputMappingContext() const
{
	if (PlayerController && PlayerController->GetLocalPlayer())
	{
		if (const auto Input = PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			PlayerController->bShowMouseCursor = true;

			// Check if the context is already bound to prevent double binding
			if (!Input->HasMappingContext(this->InputMappingContext))
			{
				Input->ClearAllMappings();
				Input->AddMappingContext(this->InputMappingContext, 0);
			}
		}
	}
}

void URTSCamera::BindInputActions()
{
	if (const auto EnhancedInputComponent = Cast<UEnhancedInputComponent>(this->PlayerController->InputComponent))
	{
		EnhancedInputComponent->BindAction(
			this->ZoomCamera,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnZoomCamera
		);

		EnhancedInputComponent->BindAction(
			this->RotateCameraAxis,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnRotateCamera
		);

		EnhancedInputComponent->BindAction(
			this->TurnCameraLeft,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnTurnCameraLeft
		);

		EnhancedInputComponent->BindAction(
			this->TurnCameraRight,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnTurnCameraRight
		);

		EnhancedInputComponent->BindAction(
			this->MoveCameraXAxis,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnMoveCameraXAxis
		);

		EnhancedInputComponent->BindAction(
			this->MoveCameraYAxis,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnMoveCameraYAxis
		);

		EnhancedInputComponent->BindAction(
			this->DragCamera,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnDragCamera
		);



		//"RTSSelector.h"

		EnhancedInputComponent->BindAction(
			this->BeginSelection,
			ETriggerEvent::Started,
			this,
			&URTSCamera::OnSelectionStart
		);

		EnhancedInputComponent->BindAction(
			this->BeginSelection,
			ETriggerEvent::Triggered,
			this,
			&URTSCamera::OnUpdateSelection
		);

		EnhancedInputComponent->BindAction(
			this->BeginSelection,
			ETriggerEvent::Completed,
			this,
			&URTSCamera::OnSelectionEnd
		);

	}
}

void URTSCamera::SetActiveCamera() const
{
	this->PlayerController->SetViewTarget(this->GetOwner());
}

void URTSCamera::JumpTo(const FVector Position) const
{
	this->Root->SetWorldLocation(Position);
}

void URTSCamera::ConditionallyPerformEdgeScrolling()
{
	if (this->EnableEdgeScrolling && !this->IsDragging)
	{
		//this->EdgeScrollLeft();
		//this->EdgeScrollRight();
		//this->EdgeScrollUp();
		//this->EdgeScrollDown();
		//获取屏幕是否拖拽及拖拽值
		RTSMouseLeftMovement = this->EdgeScrollLeft();
		RTSMouseRightMovement = this->EdgeScrollRight();
		RTSMouseUpMovement = this->EdgeScrollUp();
		RTSMouseDownMovement = this->EdgeScrollDown();

		RTSMouseLeftMovement = -1 * RTSMouseLeftMovement;
		RTSMouseDownMovement = -1 * RTSMouseDownMovement;

		//UE_LOG(LogTemp, Warning, TEXT("RTSMouseRightMovement: %f"), RTSMouseRightMovement);

	}
}

double URTSCamera::EdgeScrollLeft() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this->GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this->GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = 1 - UKismetMathLibrary::NormalizeToRange(
		MousePosition.X,
		0.0f,
		ViewportSize.X * 0.05f
	);

	const auto Movement = UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);

	this->Root->AddRelativeLocation(
		-1 * this->Root->GetRightVector() * Movement * this->EdgeScrollSpeed * this->DeltaSeconds
	);
	//UE_LOG(LogTemp, Warning, TEXT("Movement: %f"), Movement);
	return Movement;
}

double URTSCamera::EdgeScrollRight() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this->GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this->GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = UKismetMathLibrary::NormalizeToRange(
		MousePosition.X,
		ViewportSize.X * 0.95f,
		ViewportSize.X
	);

	const auto Movement = UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);
	auto RightLocation = this->Root->GetRightVector() * Movement * this->EdgeScrollSpeed * this->DeltaSeconds;
	this->Root->AddRelativeLocation(
		RightLocation
	);
	//UE_LOG(LogTemp, Warning, TEXT("RightLocation: %s"), *RightLocation.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("DeltaSeconds: %f"), DeltaSeconds);
	return Movement;
}

double URTSCamera::EdgeScrollUp() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this->GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this->GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = UKismetMathLibrary::NormalizeToRange(
		MousePosition.Y,
		0.0f,
		ViewportSize.Y * 0.05f
	);

	const auto Movement = 1 - UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);
	this->Root->AddRelativeLocation(
		this->Root->GetForwardVector() * Movement * this->EdgeScrollSpeed * this->DeltaSeconds
	);
	return Movement;
}

double URTSCamera::EdgeScrollDown() const
{
	const auto MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this->GetWorld());
	const auto ViewportSize = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this->GetWorld()).GetLocalSize();
	const auto NormalizedMousePosition = UKismetMathLibrary::NormalizeToRange(
		MousePosition.Y,
		ViewportSize.Y * 0.95f,
		ViewportSize.Y
	);

	const auto Movement = UKismetMathLibrary::FClamp(NormalizedMousePosition, 0.0, 1.0);
	this->Root->AddRelativeLocation(
		-1 * this->Root->GetForwardVector() * Movement * this->EdgeScrollSpeed * this->DeltaSeconds

	);
	return Movement;
}

void URTSCamera::FollowTargetIfSet() const
{
	if (this->CameraFollowTarget != nullptr)
	{
		this->Root->SetWorldLocation(this->CameraFollowTarget->GetActorLocation());
	}
}

void URTSCamera::SmoothTargetArmLengthToDesiredZoom() const
{
	this->SpringArm->TargetArmLength = FMath::FInterpTo(
		this->SpringArm->TargetArmLength,
		this->DesiredZoomLength,
		this->DeltaSeconds,
		this->ZoomCatchupSpeed
	);
}

void URTSCamera::ConditionallyKeepCameraAtDesiredZoomAboveGround()
{
	if (this->EnableDynamicCameraHeight)
	{
		const auto RootWorldLocation = this->Root->GetComponentLocation();
		const TArray<AActor*> ActorsToIgnore;

		auto HitResult = FHitResult();
		auto DidHit = UKismetSystemLibrary::LineTraceSingle(
			this->GetWorld(),
			FVector(RootWorldLocation.X, RootWorldLocation.Y, RootWorldLocation.Z + this->FindGroundTraceLength),
			FVector(RootWorldLocation.X, RootWorldLocation.Y, RootWorldLocation.Z - this->FindGroundTraceLength),
			UEngineTypes::ConvertToTraceType(this->CollisionChannel),
			true,
			ActorsToIgnore,
			EDrawDebugTrace::Type::None,
			HitResult,
			true
		);

		if (DidHit)
		{
			this->Root->SetWorldLocation(
				FVector(
					HitResult.Location.X,
					HitResult.Location.Y,
					HitResult.Location.Z
				)
			);
		}

		else if (!this->IsCameraOutOfBoundsErrorAlreadyDisplayed)
		{
			this->IsCameraOutOfBoundsErrorAlreadyDisplayed = true;

			UKismetSystemLibrary::PrintString(
				this->GetWorld(),
				"Or add a `RTSCameraBoundsVolume` actor to the scene.",
				true,
				true,
				FLinearColor::Red,
				100
			);

			UKismetSystemLibrary::PrintString(
				this->GetWorld(),
				"Increase trace length or change the starting position of the parent actor for the spring arm.",
				true,
				true,
				FLinearColor::Red,
				100
			);

			UKismetSystemLibrary::PrintString(
				this->GetWorld(),
				"Error: AC_RTSCamera needs to be placed on the ground!",
				true,
				true,
				FLinearColor::Red,
				100
			);
		}
	}
}

void URTSCamera::ConditionallyApplyCameraBounds() const
{
	if (this->BoundaryVolume != nullptr)
	{
		const auto RootWorldLocation = this->Root->GetComponentLocation();
		FVector Origin;
		FVector Extents;
		this->BoundaryVolume->GetActorBounds(false, Origin, Extents);
		this->Root->SetWorldLocation(
			FVector(
				UKismetMathLibrary::Clamp(RootWorldLocation.X, Origin.X - Extents.X, Origin.X + Extents.X),
				UKismetMathLibrary::Clamp(RootWorldLocation.Y, Origin.Y - Extents.Y, Origin.Y + Extents.Y),
				RootWorldLocation.Z
			)
		);
	}
}



//"RTSSelector.h"

void URTSCamera::OnSelectionStart(const FInputActionValue& Value)
{
	FVector2D MousePosition;
	double MouseX = MousePosition.X;
	double MouseY = MousePosition.Y;

	//MousePosition.X= MousePosition.X+500;
	//RTSCamera.MoveSpeed
	PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y);

	//PlayerController->GetMousePosition(MouseX, MouseY);//此处修改框选起始位置

	SelectionStart = MousePosition;
	
	//MousePosition.X = MousePosition.X - 5;//RTSMouseLefti

	HUD->BeginSelection(MousePosition);//MousePosition
}



void URTSCamera::OnUpdateSelection(const FInputActionValue& Value)
{
	FVector2D MousePosition;

	RTSMouseSelectRate = -0.0002 * this->EdgeScrollSpeed + 24;// -0.0003 * this->EdgeScrollSpeed + 25;//-12 * FMath::LogX(10.0f, this->DesiredZoomLength) + 65;

	PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y);

	SelectionEnd = MousePosition;
	
	if (SpringArmLocalRotation.Yaw >= 179.0 || SpringArmLocalRotation.Yaw <= -179.0)
	{
		RTSKeyYMovement = 0 - RTSKeyYMovement;
		//UE_LOG(LogTemp, Warning, TEXT("Spring Arm Local Rotation: Yaw: %f"), SpringArmLocalRotation.Yaw);
	}

	SelectionStart.X = SelectionStart.X - (RTSMouseLeftMovement + RTSMouseRightMovement + RTSKeyXMovement) * RTSMouseSelectRate;// 然后乘以一个距离系数 RTSMouseLefti
	SelectionStart.Y = SelectionStart.Y + (RTSMouseUpMovement + RTSMouseDownMovement + RTSKeyYMovement) * RTSMouseSelectRate*0.5625;//1080/1920 或试试角度

	HUD->BeginSelection(SelectionStart);
	HUD->UpdateSelection(SelectionEnd);
	//UE_LOG(LogTemp, Warning, TEXT("SelectionStart: %f , %f   DesiredZoomLength: %f , Rate: %f, EdgeScrollSpeed: %f"), SelectionStart.X, SelectionStart.Y, this->DesiredZoomLength, RTSMouseSelectRate, this->EdgeScrollSpeed);
	
	//3700 22.4 15 + this->DesiredZoomLength / 500
	// 4100- 19 15+ this->DesiredZoomLength / 1000
	//8000-18 10+ this->DesiredZoomLength / 1000
	//y=30⋅x(−0.5)+ 10
}

void URTSCamera::OnSelectionEnd(const FInputActionValue& Value)
{
	// Call PerformSelection on the HUD to execute selection logic
	HUD->EndSelection();
}

//"RTSSelector.h"