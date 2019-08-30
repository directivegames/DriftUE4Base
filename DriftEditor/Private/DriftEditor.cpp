/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2017 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#include "DriftEditor.h"
#include "DriftEditorPrivatePCH.h"


#include "DriftTargetSettingsCustomization.h"

#include "ModuleInterface.h"
#include "ISettingsModule.h"
#include "ModuleManager.h"

#include "DriftProjectSettings.h"


#define LOCTEXT_NAMESPACE "DriftPlugin"


IMPLEMENT_MODULE(FDriftEditor, DriftEditor)


void FDriftEditor::StartupModule()
{
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomClassLayout(
        UDriftProjectSettings::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FDriftTargetSettingsCustomization::MakeInstance)
    );
    
    PropertyModule.NotifyCustomizationModuleChanged();
    
    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule != nullptr)
    {
        SettingsModule->RegisterSettings(
            "Project", "Game", "Drift",
            LOCTEXT("DriftSettingsName", "Drift"),
            LOCTEXT("DriftSettingsDescription", "Drift settings"),
            GetMutableDefault<UDriftProjectSettings>()
        );
    }
}


void FDriftEditor::ShutdownModule()
{
    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule != nullptr)
    {
        SettingsModule->UnregisterSettings("Project", "Game", "Drift");
    }
}


#undef LOCTEXT_NAMESPACE
