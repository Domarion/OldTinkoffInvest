#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <cctype>
#include <iomanip>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace TinkoffApi
{
    enum class ResponseType
    {
        UNDEFINED = 0,
        ErrorResponse = 1,
        OperationsResponse = 2,
        PortfolioResponse = 3,
        MarketStocksResponse = 4
    };

    struct Portfolio
    {
        std::vector<std::string> positions;
    };

    struct Trade
    {
        std::string tradeId;
        std::string date;
        double price = 0.0;
        double quantity = 0.0;
    };

    struct Commission
    {
        std::string currency;
        double value = 0.0;
    };

    struct Operation
    {
        std::string id;
        std::string status;
        std::vector<Trade> trades;
        Commission commission;
        std::string currency;
        double payment = 0.0;
        double price = 0.0;
        double quantity = 0.0;
        std::string figi;
        std::string instrumentType;
        bool isMarginCall = false;
        std::string date;
        std::string operationType;
    };

    struct OperationsResponse
    {
        std::string trackingId;
        std::string status;

        std::vector<Operation> operations;
    };

    struct OperationRequest
    {
        std::string from;
        std::string to;
        std::string figi;

        std::string GetTarget() const
        {
            return "/openapi/operations";
        }
    };

    struct StockInstrument
    {
        std::string figi;
        std::string ticker;
        std::string isin;
        double minPriceIncrement = 0;
        double lot = 0;
        std::string currency;
        std::string name;
        std::string type;
    };

    struct MarketStocksResponse
    {
        std::vector<StockInstrument> instruments;
    };

    std::string GetMarketStocksTarget()
    {
        return "/openapi/market/stocks";
    }
}

struct HttpBuffer
{
public:
    explicit HttpBuffer(const std::string& aUri)
        : mBuffer(aUri)
    {
    }

    void WriteKeyValue(
        const std::string& aTag,
        const std::string& aValue)
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

    std::string GetBuffer() const
    {
        return mBuffer;
    }

    void Clear()
    {
        mBuffer.clear();
    }

private:

    std::string UrlEncode(const std::string& aValue)
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

    void WriteDelimiter()
    {
        mBuffer += delimiter;
    }

    void WriteStartParams()
    {
        mBuffer += getParamsStart;
    }

    std::string mBuffer;

    bool isFirst = true;
    const char getParamsStart = '?';
    const char delimiter = '&';
    const char keyValueSeparator = '=';
};

struct HttpGetTargetWriter
{
    static std::string GetTarget(
        const std::string& aUri,
        const TinkoffApi::OperationRequest& aRequest)
    {
        HttpBuffer buffer(aUri);
        buffer.WriteKeyValue("from", aRequest.from);
        buffer.WriteKeyValue("to", aRequest.to);
        buffer.WriteKeyValue("figi", aRequest.figi);

        return buffer.GetBuffer();
    }
};

struct JsonParser
{
public:
    using TOnMarketStocksParsed =
        std::function<void(const TinkoffApi::MarketStocksResponse&)>;

    using TOnOperationsParsed =
        std::function<void(const TinkoffApi::OperationsResponse&)>;

    explicit JsonParser(
        const TOnMarketStocksParsed& aMarketStocksCallback,
        const TOnOperationsParsed& aOperationsCallBack)
        : mStocksHandler(aMarketStocksCallback)
        , mOperationsHandler(aOperationsCallBack)
    {
    };

    void Parse(
        const std::string& aJsonString,
        TinkoffApi::ResponseType aResponseType)
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

    void ParsePortfolio()
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

    void ParseOperations()
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

        if (mOperationsHandler)
        {
            mOperationsHandler(operationsResponse);
        }
    }

    void ParseMarketStocks()
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

        if (mStocksHandler)
        {
            mStocksHandler(marketStocksResponse);
        }
    }

    bool CheckExist(
        const rapidjson::Value& aContainer,
        const std::string& aKey) const
    {
        if (aContainer.HasMember(aKey.c_str()))
        {
            return true;
        }

        std::cerr << "No " << aKey << std::endl;

        return false;
    }

    bool CheckJsonScheme(const std::string& aJsonString, std::string& outError)
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

private:
    rapidjson::Document mDocument{};

    TOnMarketStocksParsed mStocksHandler;
    TOnOperationsParsed mOperationsHandler;
};
