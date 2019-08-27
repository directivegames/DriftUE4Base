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


#include "JWTRequestManager.h"


JWTRequestManager::JWTRequestManager(const FString& token)
: headerValue(TEXT("Bearer ") + token)
{
}


void JWTRequestManager::AddCustomHeaders(TSharedRef<HttpRequest> request) const
{
    JsonRequestManager::AddCustomHeaders(request);
    
    request->SetHeader(TEXT("Authorization"), headerValue);
}
