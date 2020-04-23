#pragma once

#include <string>

struct HttpBuffer
{
public:
    explicit HttpBuffer(const std::string& aUri);

    void WriteKeyValue(
        const std::string& aTag,
        const std::string& aValue);

    std::string GetBuffer() const;

    void Clear();

private:

    std::string UrlEncode(const std::string& aValue);

    void WriteDelimiter();

    void WriteStartParams();

    std::string mBuffer;

    bool isFirst = true;
    const char getParamsStart = '?';
    const char delimiter = '&';
    const char keyValueSeparator = '=';
};
