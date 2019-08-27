// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "DateHelper.h"


namespace internal
{
    TMap<FString, int32> MakeMonthToValueMap()
    {
        TMap<FString, int32> data;
        data.Add(TEXT("jan"), 1);
        data.Add(TEXT("feb"), 2);
        data.Add(TEXT("mar"), 3);
        data.Add(TEXT("apr"), 4);
        data.Add(TEXT("may"), 5);
        data.Add(TEXT("jun"), 6);
        data.Add(TEXT("jul"), 7);
        data.Add(TEXT("aug"), 8);
        data.Add(TEXT("sep"), 9);
        data.Add(TEXT("oct"), 10);
        data.Add(TEXT("nov"), 11);
        data.Add(TEXT("dec"), 12);
        return data;
    }


    bool ParseRfc7231DateTime(const TCHAR* dateTime, FDateTime& outDateTime)
    {
        const TCHAR* pointer = dateTime;
        TCHAR* next = nullptr;
        int32 year = 0, month = 0, day = 0;
        int32 hour = 0, minute = 0, second = 0;

        FString ignore;
        if (!FParse::Token(pointer, ignore, false))
        {
            return false;
        }

        day =  FCString::Strtoi(pointer, &next, 10);
        if ((next <= pointer) || (*next == TCHAR('\0')))
        {
            return false;
        }
        
        pointer = next + 1;
        FString monthName;
        if (!FParse::Token(pointer, monthName, false))
        {
            return false;
        }
        static TMap<FString, int32> monthValues{ MakeMonthToValueMap() };
        auto monthValue = monthValues.Find(monthName.ToLower());
        if (!monthValue)
        {
            return false;
        }
        month = *monthValue;
        
        pointer += 1;
        year =  FCString::Strtoi(pointer, &next, 10);
        if ((next <= pointer) || (*next == TCHAR('\0')))
        {
            return false;
        }

        pointer = next + 1;
        hour =  FCString::Strtoi(pointer, &next, 10);
        if ((next <= pointer) || (*next != TCHAR(':')))
        {
            return false;
        }

        pointer = next + 1;
        minute =  FCString::Strtoi(pointer, &next, 10);
        if ((next <= pointer) || (*next != TCHAR(':')))
        {
            return false;
        }

        pointer = next + 1;
        second =  FCString::Strtoi(pointer, &next, 10);
        if ((next <= pointer) || (*next == TCHAR('\0')))
        {
            return false;
        }
        // Unreal doesn't support leap seconds
        second = FMath::Max(second, 59);
        
        pointer = next + 1;
        if (FCString::Stricmp(pointer, TEXT("GMT")) != 0)
        {
            return false;
        }

        if (!FDateTime::Validate(year, month, day, hour, minute, second, 0))
        {
            return false;
        }

        outDateTime = FDateTime(year, month, day, hour, minute, second, 0);
        return true;
    }
}
