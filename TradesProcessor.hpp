#pragma once

#include <iostream>
#include <fstream>
#include <map>
#include <set>

#include "IParserHandler.hpp"
#include "TinkoffApi.hpp"

enum class TradeType
{
    UNDEFINED = 0,
    LongTrade = 1,
    ShortTrade = 2,
};

struct TradeToSave
{
    std::string InstrumentName;
    std::string Side;

    double Price = 0;
    double Amount = 0;
    TinkoffApi::Commission Commission;

    static std::vector<std::string> GetTableColumns()
    {
        const std::string instrumentName = "Instrument Name";
        const std::string side = "Side";
        const std::string price = "Price";
        const std::string amount = "Amount";
        const std::string commissionCurrency = "Commission Currency";
        const std::string commissionValue = "Commission Value";

        return
        {
            instrumentName,
            side,
            price,
            amount,
            commissionCurrency,
            commissionValue
        };
    }
};

std::string ShowEmpty(const std::string& aValue)
{
    if (aValue.empty())
    {
        return "\"\"";
    }
    return aValue;
}
std::ostream& operator<<(std::ostream& outStream, const TradeToSave& aValue)
{
    const char delimiter = ';';
    return outStream
        << ShowEmpty(aValue.InstrumentName)
        << delimiter << ShowEmpty(aValue.Side)
        << delimiter << aValue.Price
        << delimiter << aValue.Amount
        << delimiter << ShowEmpty(aValue.Commission.currency)
        << delimiter << aValue.Commission.value;
}


struct TradesProcessor : IParserHandler
{
public:
    virtual void OnMessageParsed(const TinkoffApi::MarketStocksResponse& aResponse) override
    {
        for (const auto& instrument : aResponse.instruments)
        {
            assert(!instrument.figi.empty() && !instrument.name.empty());
            mFigiToName[instrument.figi] = instrument.name;
        }
    }

    virtual void OnMessageParsed(const TinkoffApi::OperationsResponse& aResponse) override
    {
        const std::set<std::string> allowedOperationTypes
        {
            "Buy",
            "Sell",
            "BuyCard"
        };
        for (const auto& operation : aResponse.operations)
        {
            if (allowedOperationTypes.count(operation.operationType) == 0
                || operation.status == "Declined"
                || operation.trades.empty()
                || operation.instrumentType != "Stock")
            {
                continue;
            }

            for (const auto& originalTrade : operation.trades)
            {
                TradeToSave trade;
                assert(!operation.figi.empty());
                try
                {
                    trade.InstrumentName = mFigiToName.at(operation.figi);
                }
                catch (std::exception& ex)
                {
                    std::cerr << "Error: " << ex.what() << std::endl;
                }

                trade.Price = originalTrade.price;
                trade.Side = operation.operationType == "Sell"
                    ? "Sell"
                    : "Buy";
                trade.Amount = originalTrade.quantity;
                trade.Commission = operation.commission;
                mTrades.emplace_back(trade);
            }
        }
    }

    void SaveTrades() const
    {
        std::cout << "save trades" << std::endl;
        if (mTrades.empty())
        {
            std::cerr << "SaveTrades. No trades" << std::endl;
            return;
        }

        std::ofstream fileStream("/home/kostya_hm/trades.output");
        if (fileStream.is_open())
        {
            std::cerr << "SaveTrades. Try to save" << std::endl;

            const auto columns = TradeToSave::GetTableColumns();
            std::copy(
                std::begin(columns),
                std::end(columns),
                std::ostream_iterator<std::string>(fileStream, ";"));

            for (const auto& trade : mTrades)
            {
                fileStream << std::endl << trade;
            }
        }

        fileStream.close();
    }

private:
    std::map<std::string, std::string> mFigiToName;
    std::vector<TradeToSave> mTrades;
};
