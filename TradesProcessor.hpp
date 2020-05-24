#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
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

};

std::vector<std::string> GetTradesTableColumns();

std::string ShowEmpty(const std::string& aValue);

std::ostream& operator<<(std::ostream& outStream, const TradeToSave& aValue);

struct TradesProcessor final: IParserHandler
{
public:

    /// IParserHandler::OnMessageParsed
    virtual void OnMessageParsed(const TinkoffApi::MarketStocksResponse& aResponse) override;

    /// IParserHandler::OnMessageParsed
    virtual void OnMessageParsed(const TinkoffApi::OperationsResponse& aResponse) override;

    void SaveTrades() const;

    void SaveProfitLoss(const std::string& aFromTime, const std::string& toTime) const;

    virtual ~TradesProcessor() = default;

private:
    std::map<std::string, TinkoffApi::StockInstrument> mFigiToInstrument;
    std::vector<TradeToSave> mTrades;

    using TFigi = std::string;
    std::map<TFigi, std::set<TinkoffApi::Operation> > mOperations;
};
