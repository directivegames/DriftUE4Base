// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "Features/IModularFeatures.h"


class IDriftAPI;


class IDriftProvider : public IModularFeature
{
public:
	virtual ~IDriftProvider() = default;

    virtual IDriftAPI* GetInstance(const FName& identifier) = 0;
    virtual IDriftAPI* GetInstance(const FName& identifier, const FString& config) = 0;
    virtual void DestroyInstance(const FName& identifier) = 0;
    virtual void DestroyInstance(IDriftAPI* instance) = 0;
};
