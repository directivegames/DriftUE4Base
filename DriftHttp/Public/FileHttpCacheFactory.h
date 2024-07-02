// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "DriftHttpCache.h"


class DRIFTHTTP_API FileHttpCacheFactory : public IHttpCacheFactory
{
public:
    TSharedPtr<IHttpCache> Create() override;
};
