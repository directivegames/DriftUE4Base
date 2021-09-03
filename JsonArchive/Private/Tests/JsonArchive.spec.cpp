/**
* This file is part of the Pandora library.
*
* Copyright (c) 2016 Hypodermic Project, 2020 Directive Games Limited
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#include "JsonArchive.h"

#include "Misc/AutomationTest.h"
#if WITH_EDITOR
#include "Tests/AutomationEditorCommon.h"
#endif

struct FWithOptionalProperty
{
	FString NullableString;
	FDateTime NullableDateTime{ 0 };

	bool Serialize(SerializationContext& Context)
	{
		const auto Result = SERIALIZE_OPTIONAL_PROPERTY(Context, NullableString)
			&& SERIALIZE_OPTIONAL_PROPERTY(Context, NullableDateTime);
		return Result;
	}
};

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(DriftJsonArchiveSpec, "Game.Drift.JsonArchive", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(DriftJsonArchiveSpec)

void DriftJsonArchiveSpec::Define()
{
    Describe("LoadObject", [this]
    {
        It("should not consider null an invalid value for optional properties", [this]
        {
	        const FString JsonString{ TEXT("{\"NullableString\": null, \"NullableDateTime\": null}") };
        	FWithOptionalProperty Data{ FString{TEXT("Dummy")}, FDateTime{42}};
        	TestTrue("Serializing should return success", JsonArchive::LoadObject(*JsonString, Data));
        	TestEqual("Nullable string retains empty value", Data.NullableString, FString{});
        	TestEqual("Nullable datetime retains default value", Data.NullableDateTime, FDateTime{ 0 });
        });
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
