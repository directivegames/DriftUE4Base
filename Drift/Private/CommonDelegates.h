// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "CommonDelegates.generated.h"


USTRUCT(BlueprintType)
struct FDriftResponseInfo2
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	bool successfull;

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	FString origin;  // The origin of the request i.e. BP function, tick function, etc.

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	int32 http_code;

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	FString verb;

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	FString url;

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	FString request_body;

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	FString content;

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	FString error_text;

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	FString message; // A message from server, i.e. reflection of {"message": "You shall not pass!"}

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	bool is_fatal = true; // In case of errors, should the client app exit or not.

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	FString request_started;

	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = "Drift|Session")
	FString response_received;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnError_Response, FDriftResponseInfo2, error_response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKaleoErrorDelegate, FDriftResponseInfo2, response);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKaleoEmptySuccessDelegate);
