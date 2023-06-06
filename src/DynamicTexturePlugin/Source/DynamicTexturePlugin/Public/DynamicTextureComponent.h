// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Texture2D.h"
#include "Rendering/Texture2DResource.h"
#include "DynamicTextureComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DYNAMICTEXTUREPLUGIN_API UDynamicTextureComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDynamicTextureComponent();

protected:
    // The size of the render texture we're going to create
    const int mTextureDimensions = 1024;

protected:
	// Called when the game starts
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void SetupTexture();
    void UpdateTexture();

    TArray<class UMaterialInstanceDynamic*> mDynamicMaterials;
    UTexture2D* mDynamicTexture = nullptr;
    FUpdateTextureRegion2D* mUpdateTextureRegion;

    uint8* mDynamicColors = nullptr;

    uint32 mDataSize;
    uint32 mDataSqrtSize;
    uint32 mArraySize;
    uint32 mArrayRowSize;

    UStaticMeshComponent* mStaticMeshComponent;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
