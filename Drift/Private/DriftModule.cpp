/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2019 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/


#include "DriftModule.h"
#include "DriftBase.h"
#include "Engine/Console.h"

#include "Features/IModularFeatures.h"


#define LOCTEXT_NAMESPACE "Drift"


IMPLEMENT_MODULE(FDriftModule, Drift)


FDriftModule::FDriftModule()
{

}


void FDriftModule::StartupModule()
{
    IModularFeatures::Get().RegisterModularFeature(TEXT("Drift"), &provider);

#if ALLOW_CONSOLE
	UConsole::RegisterConsoleAutoCompleteEntries.AddStatic(&FDriftModule::PopulateAutoCompleteEntries);
#endif // ALLOW_CONSOLE
}


void FDriftModule::ShutdownModule()
{
    IModularFeatures::Get().UnregisterModularFeature(TEXT("Drift"), &provider);
}


#if ALLOW_CONSOLE
#endif // ALLOW_CONSOLE
void FDriftModule::PopulateAutoCompleteEntries(TArray<FAutoCompleteCommand>& AutoCompleteList)
{
	const UConsoleSettings* ConsoleSettings = GetDefault<UConsoleSettings>();

	auto Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Party SendInvite");
	AutoCompleteList[Index].Desc = TEXT("<player_id> Send a party invite to another player");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Party AcceptInvite");
	AutoCompleteList[Index].Desc = TEXT("<invite_id> Accept a party invite from another player");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Party DeclineInvite");
	AutoCompleteList[Index].Desc = TEXT("<invite_id> Decline a party invite from another player");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Party CancelInvite");
	AutoCompleteList[Index].Desc = TEXT("<invite_id> Cancel a party invite sent to another player");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Party Leave");
	AutoCompleteList[Index].Desc = TEXT("Leave the current party");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;
}

#undef LOCTEXT_NAMESPACE
