#include <cctype>
#include <iomanip>
#include <sstream>

#include "UrlEncoder.hpp"

HttpBuffer::HttpBuffer(const std::string& aUri)
    : mBuffer(aUri)
{
}

void HttpBuffer::WriteKeyValue(const std::string& aTag, const std::string& aValue)
{
    if (aValue.empty())
    {
        return;
    }

    if (isFirst)
    {
        WriteStartParams();
        isFirst = false;
    }
    else
    {
        WriteDelimiter();
    }


    mBuffer.append(aTag);
    mBuffer += keyValueSeparator;
    mBuffer.append(UrlEncode(aValue));
}

std::string HttpBuffer::GetBuffer() const
{
    return mBuffer;
}

void HttpBuffer::Clear()
{
    mBuffer.clear();
}

std::string HttpBuffer::UrlEncode(const std::string& aValue)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (const char c : aValue)
    {
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

void HttpBuffer::WriteDelimiter()
{
    mBuffer += delimiter;
}

void HttpBuffer::WriteStartParams()
{
    mBuffer += getParamsStart;
}
