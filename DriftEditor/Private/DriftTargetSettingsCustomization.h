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

#pragma once

#include "EditorStyle.h"
#include "PropertyEditorModule.h"
#include "DriftProjectSettings.h"


class FDriftTargetSettingsCustomization : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

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
