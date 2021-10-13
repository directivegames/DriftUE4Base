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
void FDriftModule::PopulateAutoCompleteEntries(TArray<FAutoCompleteCommand>& AutoCompleteList)
{
	const UConsoleSettings* ConsoleSettings = GetDefault<UConsoleSettings>();


	// Drift parties

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

	// Drift lobbies

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby Get");
	AutoCompleteList[Index].Desc = TEXT("Gets the current lobby");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby Create");
	AutoCompleteList[Index].Desc = TEXT("Creates a new lobby");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby Join");
	AutoCompleteList[Index].Desc = TEXT("<lobby_id> Joins a lobby");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby Leave");
	AutoCompleteList[Index].Desc = TEXT("Leaves the current lobby");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby UpdateLobbyName");
	AutoCompleteList[Index].Desc = TEXT("<lobby_name> Updates the current lobby name. Must be the host");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby UpdateLobbyMap");
	AutoCompleteList[Index].Desc = TEXT("<map_name> Updates the current lobby map. Must be the host");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby UpdateLobbyTeamCapacity");
	AutoCompleteList[Index].Desc = TEXT("<team_capacity> Updates the current lobby team capacity. Must be the host");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby UpdateLobbyTeamNames");
	AutoCompleteList[Index].Desc = TEXT("<team_names> Updates the current lobby team names. Must be the host. Comma seperated list");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby UpdatePlayerTeamName");
	AutoCompleteList[Index].Desc = TEXT("<team_name> Updates the player's team name");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby UpdatePlayerReady");
	AutoCompleteList[Index].Desc = TEXT("<ready> Updates the player's ready status. 0 or 1");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby KickPlayer");
	AutoCompleteList[Index].Desc = TEXT("<player_id> Kicks the player from the current lobby. Must be the host");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;

	Index = AutoCompleteList.AddDefaulted();
	AutoCompleteList[Index].Command = TEXT("Drift.Lobby StartMatch");
	AutoCompleteList[Index].Desc = TEXT("Starts the lobby match. Must be the host");
	AutoCompleteList[Index].Color = ConsoleSettings->AutoCompleteCommandColor;
}
#endif // ALLOW_CONSOLE

#undef LOCTEXT_NAMESPACE
