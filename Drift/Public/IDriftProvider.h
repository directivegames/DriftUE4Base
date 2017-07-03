// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "Features/IModularFeatures.h"


class IDriftAPI;


class IDriftProvider : public IModularFeature
{
public:
    virtual IDriftAPI* GetInstance(const FName& identifier) = 0;
    virtual void DestroyInstance(const FName& identifier) = 0;
};
