#include <iostream>
#include <map>

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "Parser.hpp"

JsonParser::JsonParser(const std::shared_ptr<IParserHandler>& aProcessor)
    : mParserHander(aProcessor)
{
}

void JsonParser::Parse(const std::string& aJsonString, TinkoffApi::ResponseType aResponseType)
{
    std::string outError;
    if (!CheckJsonScheme(aJsonString, outError))
    {
        std::cerr << "Json parsing error: " << outError;
        return;
    }

    switch (aResponseType)
    {
    case TinkoffApi::ResponseType::PortfolioResponse:
        ParsePortfolio();
        break;
    case TinkoffApi::ResponseType::OperationsResponse:
        ParseOperations();
        break;
    case TinkoffApi::ResponseType::MarketStocksResponse:
        ParseMarketStocks();
        break;

    default:
        break;
    }
}

void JsonParser::ParsePortfolio()
{
    TinkoffApi::Portfolio portfolio;

    if (!mDocument.HasMember("payload"))
    {
        std::cerr << "no payload" << std::endl;
        return;
    }

    auto& payload = mDocument["payload"];

    if (!payload.HasMember("positions"))
    {
        std::cerr << "no positions" << std::endl;
        return;
    }
    auto& positions = payload["positions"];

    for (const auto& position : positions.GetArray())
    {
        if (!position.HasMember("figi"))
        {
            std::cerr << "no figi" << std::endl;
            return;
        }
        std::string figi = position["figi"].GetString();
        portfolio.positions.emplace_back(figi);
    }
}

void JsonParser::ParseOperations()
{
    TinkoffApi::OperationsResponse operationsResponse;

    if (!mDocument.HasMember("payload"))
    {
        std::cerr << "no payload" << std::endl;
        return;
    }

    auto& payload = mDocument["payload"];

    if (!CheckExist(payload, "operations"))
    {
        return;
    }

    const auto& operations = payload["operations"].GetArray();

    for (const auto& operationObj : operations)
    {
        TinkoffApi::Operation operation;

        if (CheckExist(operationObj, "id"))
        {
            operation.id = operationObj["id"].GetString();
        }

        if (CheckExist(operationObj, "status"))
        {
            operation.status = operationObj["status"].GetString();
        }

        if (CheckExist(operationObj, "operationType"))
        {
            operation.operationType = operationObj["operationType"].GetString();
        }

        if (CheckExist(operationObj, "figi"))
        {
            operation.figi = operationObj["figi"].GetString();
        }

        if (CheckExist(operationObj, "instrumentType"))
        {
            operation.instrumentType = operationObj["instrumentType"].GetString();
        }

        if (CheckExist(operationObj, "price"))
        {
            operation.price = operationObj["price"].GetDouble();
        }

        if (CheckExist(operationObj, "quantity"))
        {
            operation.quantity = operationObj["quantity"].GetDouble();
        }

        if (CheckExist(operationObj, "currency"))
        {
            operation.currency = operationObj["currency"].GetString();
        }

        if (CheckExist(operationObj, "date"))
        {
            operation.date = operationObj["date"].GetString();
        }

        if (CheckExist(operationObj, "payment"))
        {
            operation.payment = operationObj["payment"].GetDouble();
        }

        if (CheckExist(operationObj, "commission"))
        {
            const auto& commission = operationObj["commission"];
            operation.commission.currency = commission["currency"].GetString();
            operation.commission.value = commission["value"].GetDouble();
        }

        if (!CheckExist(operationObj, "trades"))
        {
            operationsResponse.operations.emplace_back(operation);
            continue;
        }

        const auto& trades = operationObj["trades"].GetArray();
        for (const auto& tradeObj : trades)
        {
            TinkoffApi::Trade trade;
            trade.tradeId = tradeObj["tradeId"].GetString();
            trade.date = tradeObj["date"].GetString();
            trade.price = tradeObj["price"].GetDouble();
            trade.quantity = tradeObj["quantity"].GetDouble();
            operation.trades.emplace_back(trade);
        }
        operationsResponse.operations.emplace_back(operation);
    }

    if (mParserHander)
    {
        mParserHander->OnMessageParsed(operationsResponse);
    }
}

void JsonParser::ParseMarketStocks()
{
    TinkoffApi::MarketStocksResponse marketStocksResponse;

    if (!mDocument.HasMember("payload"))
    {
        std::cerr << "no payload" << std::endl;
        return;
    }

    auto& payload = mDocument["payload"];

    if (!CheckExist(payload, "instruments"))
    {
        return;
    }

    const auto& instruments = payload["instruments"].GetArray();

    for (const auto& instrumentObj : instruments)
    {
        TinkoffApi::StockInstrument instrument;

        if (CheckExist(instrumentObj, "figi"))
        {
            instrument.figi = instrumentObj["figi"].GetString();
        }

        if (CheckExist(instrumentObj, "ticker"))
        {
            instrument.ticker = instrumentObj["ticker"].GetString();
        }

        if (CheckExist(instrumentObj, "isin"))
        {
            instrument.isin = instrumentObj["isin"].GetString();
        }

        if (CheckExist(instrumentObj, "minPriceIncrement"))
        {
            instrument.minPriceIncrement = instrumentObj["minPriceIncrement"].GetDouble();
        }

        if (CheckExist(instrumentObj, "lot"))
        {
            instrument.lot = instrumentObj["lot"].GetDouble();
        }

        if (CheckExist(instrumentObj, "currency"))
        {
            instrument.currency = instrumentObj["currency"].GetString();
        }

        if (CheckExist(instrumentObj, "name"))
        {
            instrument.name = instrumentObj["name"].GetString();
        }

        if (CheckExist(instrumentObj, "type"))
        {
            instrument.type = instrumentObj["type"].GetString();
        }

        marketStocksResponse.instruments.emplace_back(instrument);
    }

    if (mParserHander)
    {
        mParserHander->OnMessageParsed(marketStocksResponse);
    }
}

bool JsonParser::CheckExist(const rapidjson::Value& aContainer, const std::string& aKey) const
{
    if (aContainer.HasMember(aKey.c_str()))
    {
        return true;
    }

    std::cerr << "No " << aKey << std::endl;

    return false;
}

bool JsonParser::CheckJsonScheme(const std::string& aJsonString, std::string& outError)
{
    mDocument = rapidjson::Document{};
    mDocument.Parse(aJsonString.c_str());

    if (mDocument.HasParseError())
    {
        outError = rapidjson::GetParseError_En(mDocument.GetParseError());
        outError.append(", offset ");
        outError += std::to_string(mDocument.GetErrorOffset());
        return false;
    }
    return true;
}
