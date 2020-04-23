#pragma once

#include <memory>

#include <rapidjson/document.h>

#include "IParserHandler.hpp"
#include "TinkoffApi.hpp"

struct JsonParser
{
public:
    explicit JsonParser(const std::shared_ptr<IParserHandler>& aProcessor);

    void Parse(
        const std::string& aJsonString,
        TinkoffApi::ResponseType aResponseType);

    void ParsePortfolio();

    void ParseOperations();

    void ParseMarketStocks();

    bool CheckExist(
        const rapidjson::Value& aContainer,
        const std::string& aKey) const;

    bool CheckJsonScheme(const std::string& aJsonString, std::string& outError);

private:
    rapidjson::Document mDocument{};

    std::shared_ptr<IParserHandler> mParserHander;
};
