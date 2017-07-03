// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "JsonRequestManager.h"


class DRIFTHTTP_API JTIRequestManager : public JsonRequestManager
{
public:
	JTIRequestManager(const FString& jti);

protected:
	void AddCustomHeaders(TSharedRef<HttpRequest> request) const override;

private:
	FString headerValue;
};
