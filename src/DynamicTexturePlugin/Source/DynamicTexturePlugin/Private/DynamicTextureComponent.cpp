/*
 NOTE!
 1. Your build .CS file should have the last two appended to it:
    PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RHI", "RenderCore" });
 2. You should create a texture the size of what you're going to use, 1024x1024 by default
 3. Import it
 4. Change
       Mip Gen Settings -> NoMipmaps
       sRGB -> false
       Compression Settings -> TC Vector Displacementmap (aka B8G8R8A8)
 5. Create a material
 6. Create a TextureSampleParameter2D node, name it "DynamicTextureParam"
 7. Change texture to be the one you imported
 8. Change "Sampler Type" to "Linear Color"
 9. Add component to an actor with a StaticMeshComponent on it
*/

#include "DynamicTextureComponent.h"

// We're using textures in 8-bit BGRA format
#define RED 2
#define GREEN 1
#define BLUE 0
#define ALPHA 3

void UpdateTextureRegions(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, bool bFreeData)
{
    if (Texture && Texture->GetResource())
    {
        typedef struct FUpdateTextureRegionsData
        {
            FTexture2DResource* Texture2DResource;
            int32 MipIndex;
            uint32 NumRegions;
            FUpdateTextureRegion2D* Regions;
            uint32 SrcPitch;
            uint32 SrcBpp;
            uint8* SrcData;
        } FUpdateTextureRegionsData;

        FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

        RegionData->Texture2DResource = (FTexture2DResource*)Texture->GetResource();
        RegionData->MipIndex = MipIndex;
        RegionData->NumRegions = NumRegions;
        RegionData->Regions = Regions;
        RegionData->SrcPitch = SrcPitch;
        RegionData->SrcBpp = SrcBpp;
        RegionData->SrcData = SrcData;

        ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
            [RegionData, bFreeData](FRHICommandListImmediate& RHICmdList)
            {
                for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
                {
                    int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
                    if (RegionData->MipIndex >= CurrentFirstMip)
                    {
                        RHIUpdateTexture2D(
                            RegionData->Texture2DResource->GetTexture2DRHI(),
                            RegionData->MipIndex - CurrentFirstMip,
                            RegionData->Regions[RegionIndex],
                            RegionData->SrcPitch,
                            RegionData->SrcData
                            + RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
                            + RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
                        );
                    }
                }
                if (bFreeData)
                {
                    FMemory::Free(RegionData->Regions);
                    FMemory::Free(RegionData->SrcData);
                }
                delete RegionData;
            });
    }
}

// Sets default values for this component's properties
UDynamicTextureComponent::UDynamicTextureComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

    mDynamicColors = nullptr;
    mUpdateTextureRegion = nullptr;
}


// Called when the game starts
void UDynamicTextureComponent::BeginPlay()
{
	Super::BeginPlay();

    // Get the static mesh we're going to modify the material of
    mStaticMeshComponent = Cast<UStaticMeshComponent>(GetOwner()->GetComponentByClass(UStaticMeshComponent::StaticClass()));

    SetupTexture();
}

void UDynamicTextureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    delete[] mDynamicColors;
    mDynamicColors = nullptr;

    delete mUpdateTextureRegion;
    mUpdateTextureRegion = nullptr;

    Super::EndPlay(EndPlayReason);
}

void UDynamicTextureComponent::SetupTexture()
{
    if (mDynamicColors) delete[] mDynamicColors;
    if (mUpdateTextureRegion) delete mUpdateTextureRegion;

    if (!mStaticMeshComponent)
    {
        GEngine->AddOnScreenDebugMessage(1, 1, FColor::Red, "Could not get static mesh component");
        return;
    }

    int32 w, h;
    w = mTextureDimensions;
    h = w;

    mDynamicMaterials.Empty();
    mDynamicMaterials.Add(mStaticMeshComponent->CreateAndSetMaterialInstanceDynamic(0));
    mDynamicTexture = UTexture2D::CreateTransient(w, h);
    mDynamicTexture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
    mDynamicTexture->SRGB = 0;
    mDynamicTexture->Filter = TextureFilter::TF_Nearest;
    mDynamicTexture->AddToRoot();
    mDynamicTexture->UpdateResource();

    mUpdateTextureRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, w, h);

    mDynamicMaterials[0]->SetTextureParameterValue("DynamicTextureParam", mDynamicTexture);

    mDataSize = w * h * 4;
    mDataSqrtSize = w * 4;
    mArraySize = w * h;
    mArrayRowSize = w;

    mDynamicColors = new uint8[mDataSize];

    memset(mDynamicColors, 0, mDataSize);
}

void UDynamicTextureComponent::UpdateTexture()
{
    if (!mDynamicTexture)
        return;

    UpdateTextureRegions(mDynamicTexture, 0, 1, mUpdateTextureRegion, mDataSqrtSize, (uint32)4, mDynamicColors, false);
    mDynamicMaterials[0]->SetTextureParameterValue("DynamicTextureParam", mDynamicTexture);
}

// Called every frame
void UDynamicTextureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!mDynamicColors)
    {
        SetupTexture();
        return;
    }

    GEngine->AddOnScreenDebugMessage(6, 0.1, FColor::Blue, "Drawing");
    float t = GetWorld()->GetTimeSeconds() * 10;

    // TODO: your texture manipulation here!
    // Provided below is a simple and uninspired example of how to modify the texture on a per-pixel basis
    int pitch = mTextureDimensions * 4;
    for (int y = 0; y < mTextureDimensions; y++)
    {
        int verticalOffset = pitch * y;
        for (int x = 0; x < mTextureDimensions; x++)
        {
            // each pixel is 4 bytes
            *(mDynamicColors + verticalOffset + x * 4 + RED) = x + t;
            *(mDynamicColors + verticalOffset + x * 4 + GREEN) = y + t;
            *(mDynamicColors + verticalOffset + x * 4 + BLUE) = x * y;
            *(mDynamicColors + verticalOffset + x * 4 + ALPHA) = 255;
        }
    }
    UpdateTexture();
}
