/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2024 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#pragma once

#include "Modules/ModuleManager.h"
#include "DriftAPI.h"
#include "DriftProvider.h"
#include "Auth/DriftTokenAuthProviderFactory.h"


struct FAutoCompleteCommand;


class FDriftModule : public IModuleInterface
{
public:
    FDriftModule();

    bool IsGameModule() const override
    {
        return true;
    }

    void StartupModule() override;
    void ShutdownModule() override;

private:
#if ALLOW_CONSOLE
	// Callback function registered with Console to inject show debug auto complete command
	static void PopulateAutoCompleteEntries(TArray<FAutoCompleteCommand>& AutoCompleteList);
#endif // ALLOW_CONSOLE

	FDriftProvider provider;

    FDriftTokenAuthProviderFactory tokenProviderFactory;
};
