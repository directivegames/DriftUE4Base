
#include "DefaultErrorReporter.h"
#include "IErrorReporter.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"


DefaultErrorReporter defaultErrorReporter;
IErrorReporter* currentErrorReporter = &defaultErrorReporter;


IErrorReporter* IErrorReporter::Get()
{
    return currentErrorReporter;
}


void IErrorReporter::Set(IErrorReporter* errorReporter)
{
    if (errorReporter)
    {
        currentErrorReporter = errorReporter;
    }
    else
    {
        currentErrorReporter = &defaultErrorReporter;
    }
}


void Log(const FName& category, const TCHAR* format, ...)
{
    const int bufferSize = 64 << 10;
    TCHAR buffer[bufferSize];
    va_list args;
    va_start(args, format);
    int res = FCString::GetVarArgs(buffer, bufferSize, format, args);
    if (res < bufferSize && GWarn)
    {
        GWarn->Log(category, ELogVerbosity::Error, buffer);
    }
    va_end(args);
}


void DefaultErrorReporter::AddError(const FName& context, const FString& message)
{
    Log(context, TEXT("%s"), *message);
}


void DefaultErrorReporter::AddError(const FName& context, const FString& message, const TSharedPtr<FJsonObject>& extra)
{
    FString text;
    auto writer = TJsonWriterFactory<>::Create(&text);
    FJsonSerializer::Serialize(extra.ToSharedRef(), writer, true);

    Log(context, TEXT("%s - %s"), *message, *text);
}
