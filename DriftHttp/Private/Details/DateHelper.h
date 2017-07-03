// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once


namespace internal
{
    /**
     * The subset is defined in RFC 7231.
     * https://tools.ietf.org/html/rfc7231#section-7.1.1.1
     */
    bool ParseRfc7231DateTime(const TCHAR* dateTime, FDateTime& outDateTime);
}
