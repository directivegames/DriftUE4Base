// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "EditorStyle.h"
#include "PropertyEditorModule.h"
#include "DriftProjectSettings.h"


class FDriftTargetSettingsCustomization : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

public:
	static FDriftTargetSettingsCustomization& getInstance()
	{
		static FDriftTargetSettingsCustomization instance;
		return instance;
	}

private:
	FDriftTargetSettingsCustomization() {};

	FDriftTargetSettingsCustomization(FDriftTargetSettingsCustomization const&) = delete;
	void operator=(FDriftTargetSettingsCustomization const&) = delete;

private:
	IDetailLayoutBuilder* SavedLayoutBuilder;
};
