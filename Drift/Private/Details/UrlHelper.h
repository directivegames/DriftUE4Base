
#pragma once


namespace internal
{
    class UrlHelper
    {
    public:
        // TODO: There has to be a more elegant way...
        static FString EscapeUrlQuery(const FString& input)
        {
            auto escaped = input.Replace(TEXT("%"), TEXT("%25"));
            escaped.ReplaceInline(TEXT(" "), TEXT("%20"));
            escaped.ReplaceInline(TEXT("\""), TEXT("%22"));
            escaped.ReplaceInline(TEXT("#"), TEXT("%23"));
            escaped.ReplaceInline(TEXT("$"), TEXT("%24"));
            escaped.ReplaceInline(TEXT("&"), TEXT("%26"));
            escaped.ReplaceInline(TEXT("+"), TEXT("%2b"));
            escaped.ReplaceInline(TEXT(","), TEXT("%2c"));
            escaped.ReplaceInline(TEXT("."), TEXT("%2e"));
            escaped.ReplaceInline(TEXT("/"), TEXT("%2f"));
            escaped.ReplaceInline(TEXT(":"), TEXT("%3a"));
            escaped.ReplaceInline(TEXT(";"), TEXT("%3b"));
            escaped.ReplaceInline(TEXT("<"), TEXT("%3c"));
            escaped.ReplaceInline(TEXT("="), TEXT("%3d"));
            escaped.ReplaceInline(TEXT(">"), TEXT("%3e"));
            escaped.ReplaceInline(TEXT("?"), TEXT("%3f"));
            escaped.ReplaceInline(TEXT("@"), TEXT("%40"));
            return escaped;
        }


        static void AddUrlOption(FString& url, const FString& option, int32 value)
        {
            AddUrlOption(url, option, FString::Printf(TEXT("%d"), value));
        }
        
        
        static void AddUrlOption(FString& url, const FString& option, const FString& value)
        {
            if (url.IsEmpty() || option.IsEmpty() || value.IsEmpty())
            {
                return;
            }
            
            FString separator = TEXT("?");
            if (url.Contains(separator))
            {
                separator = TEXT("&");
            }
            auto escapedOption = EscapeUrlQuery(option);
            auto escapedValue = EscapeUrlQuery(value);
            url = FString::Printf(TEXT("%s%s%s=%s"), *url, *separator, *escapedOption, *escapedValue);
        }
    };
}
