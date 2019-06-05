// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftPrivatePCH.h"

#include "DriftUtils.h"
#include "DriftProvider.h"

#include "Features/IModularFeatures.h"


namespace internal
{
#if UE_EDITOR
    FName GetIdentifierFromWorld(UWorld* world)
    {
        if (world != nullptr)
        {
            FWorldContext& currentContext = GEngine->GetWorldContextFromWorldChecked(world);
            if (currentContext.WorldType == EWorldType::PIE)
            {
                return currentContext.ContextHandle;
            }
        }
        return NAME_None;
    }
#endif // UE_EDITOR
}


const FName DriftModuleName = TEXT("Drift");


FDriftWorldHelper::FDriftWorldHelper(UObject* worldContextObject)
: world_{ nullptr }
{
    if (worldContextObject != nullptr && worldContextObject->IsValidLowLevel())
    {
        world_ = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    }
}


FDriftWorldHelper::FDriftWorldHelper(UWorld* world)
: world_{ world }
{
}


FDriftWorldHelper::FDriftWorldHelper(FName context)
: world_{ nullptr }
, context_{ context }
{
}


IDriftAPI* FDriftWorldHelper::GetInstance()
{
    return GetInstance(TEXT(""));
}


IDriftAPI* FDriftWorldHelper::GetInstance(const FString& config)
{
    /**
     * We're treating a null world as a valid case, returning the default Drift instance,
     * but a world without context, as invalid. This is to let us call methods during
     * network travel, where there might be no world for a short period of time.
     * TODO: Keep an eye on this to make sure it doesn't cause any problems...
     */
    if (world_ != nullptr && (world_->GetGameInstance() == nullptr || world_->GetGameInstance()->GetWorldContext() == nullptr))
    {
        // Too late, no more Drift
        return nullptr;
    }

    FName identifier = NAME_None;
#if UE_EDITOR
    if (world_ == nullptr && context_ != NAME_None)
    {
        identifier = context_;
    }
    else
    {
        identifier = internal::GetIdentifierFromWorld(world_);
    }
#endif // UE_EDITOR
    auto& provider = IModularFeatures::Get().GetModularFeature<IDriftProvider>(DriftModuleName);
    return provider.GetInstance(identifier, config);
}


void FDriftWorldHelper::DestroyInstance()
{
    FName identifier = NAME_None;
#if UE_EDITOR
    identifier = internal::GetIdentifierFromWorld(world_);
#endif // UE_EDITOR
    auto& provider = IModularFeatures::Get().GetModularFeature<IDriftProvider>(DriftModuleName);
    provider.DestroyInstance(identifier);
}

void FDriftWorldHelper::DestroyInstance(IDriftAPI* instance)
{
    auto& provider = IModularFeatures::Get().GetModularFeature<IDriftProvider>(DriftModuleName);
    provider.DestroyInstance(instance);
}
