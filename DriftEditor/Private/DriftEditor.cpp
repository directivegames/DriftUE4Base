// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftEditorPrivatePCH.h"

#include "DriftEditor.h"

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
