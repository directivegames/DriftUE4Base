// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "JsonRequestManager.h"


class JWTRequestManager : public JsonRequestManager
{
public:
    JWTRequestManager(const FString& token);

protected:
    void AddCustomHeaders(TSharedRef<HttpRequest> request) const override;

private:
    FString headerValue;
};
