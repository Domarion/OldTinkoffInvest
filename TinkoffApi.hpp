#pragma once

#include <vector>

#include "UrlEncoder.hpp"

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

    inline bool operator<(const Operation& aLeft, const Operation& aRight) noexcept
    {
        return std::stoll(aLeft.id) < std::stoll(aRight.id);
    }

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

    inline std::string GetMarketStocksTarget()
    {
        return "/openapi/market/stocks";
    }
}

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

