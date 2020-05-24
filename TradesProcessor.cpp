#include "TradesProcessor.hpp"

std::vector<std::string> GetTradesTableColumns()
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

void TradesProcessor::OnMessageParsed(const TinkoffApi::MarketStocksResponse& aResponse)
{
    for (const auto& instrument : aResponse.instruments)
    {
        assert(!instrument.figi.empty() && !instrument.name.empty());
        mFigiToInstrument[instrument.figi] = instrument;
    }
}

void TradesProcessor::OnMessageParsed(const TinkoffApi::OperationsResponse& aResponse)
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
            || operation.instrumentType != "Stock"
            || operation.id == "-1")
        {
            continue;
        }

        for (const auto& originalTrade : operation.trades)
        {
            TradeToSave trade;
            assert(!operation.figi.empty());
            try
            {
                trade.InstrumentName = mFigiToInstrument.at(operation.figi).name;
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

            // TODO: правильно подставлять коммиссии
            trade.Commission = operation.commission;
            mTrades.emplace_back(trade);
        }

        mOperations[operation.figi].emplace(operation);
    }
}

void TradesProcessor::SaveTrades() const
{
    std::cout << "Save trades" << std::endl;
    if (mTrades.empty())
    {
        std::cerr << "SaveTrades. No trades" << std::endl;
        return;
    }

    std::ofstream fileStream("/home/kostya_hm/trades.output");
    if (fileStream.is_open())
    {
        std::cerr << "SaveTrades. Try to save" << std::endl;

        const auto columns = GetTradesTableColumns();
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

struct ProfitLossInfo
{
    std::string InstrumentName;
    long double FinancialResult = 0.0;
    long double Commission = 0.0;
    long double ProfitLoss = 0.0;
    std::string Currency;
};

std::ostream& operator<<(std::ostream& outStream, const ProfitLossInfo& aInfo)
{
    return outStream
        << std::endl
        << aInfo.InstrumentName << ';'
        << aInfo.FinancialResult << ';'
        << aInfo.Commission << ';'
        << aInfo.ProfitLoss << ';'
        << aInfo.Currency << ';';
}

void TradesProcessor::SaveProfitLoss(
    const std::string& aFromTime,
    const std::string& toTime) const
{
    std::cout << "Save profit loss" << std::endl;
    if (mOperations.empty())
    {
        std::cerr << "SaveProfitLoss. No operations" << std::endl;
        return;
    }

    std::ofstream fileStream("/home/kostya_hm/profit-loss"+aFromTime + "-" + toTime + ".output");
    if (fileStream.is_open())
    {
        std::cerr << "SaveProfitLoss. Try to save" << std::endl;

        std::vector<std::string> operations
        {
            "Instrument Name",
            "Result(without commission)",
            "Commission(only trades commission)",
            "Profit & Loss",
            "Currency"
        };
        std::copy(
            std::begin(operations),
            std::end(operations),
            std::ostream_iterator<std::string>(fileStream, ";"));

        std::vector<ProfitLossInfo> profitLossInfo;
        try
        {
            for (const auto& [figi, operations] : mOperations)
            {
                ProfitLossInfo info;

                const auto& instrument = mFigiToInstrument.at(figi);
                info.InstrumentName = instrument.name;
                info.Currency = instrument.currency;

                for (const auto& operation : operations)
                {
                    assert(!operation.trades.empty());
                    if (operation.trades.empty())
                    {
                        std::cerr << "SaveProfitLoss. Trade operation without deals." << std::endl;
                        continue;
                    }

                    info.FinancialResult -= operation.payment;
                    info.Commission += std::abs(operation.commission.value);
                }
                info.ProfitLoss = info.FinancialResult - info.Commission;

                profitLossInfo.emplace_back(std::move(info));
            }

            std::copy(
                std::begin(profitLossInfo),
                std::end(profitLossInfo),
                std::ostream_iterator<ProfitLossInfo>(fileStream));
        }
        catch (const std::exception& ex)
        {
            std::cerr << "SaveProfitLoss. Error occured: " << ex.what() << std::endl;
        }
    }

    fileStream.close();
}
