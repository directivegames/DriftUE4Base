#pragma once

#include "IErrorReporter.h"


class DefaultErrorReporter : public IErrorReporter
{
    // IErrorReporter implementation
    void AddError(const FName& context, const FString& message) override;
    void AddError(const FName& context, const FString& message, const TSharedPtr<FJsonObject>& extra) override;
};
