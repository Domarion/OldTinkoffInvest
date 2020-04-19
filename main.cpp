#include <fstream>
#include <set>

#include "sslClient.hpp"

http::request<http::string_body> MakePortfolioRequest(
    const std::string& aHost,
    const std::string& aToken)
{
    return MakeGetRequest(aHost, "/openapi/portfolio", aToken);
}

http::request<http::string_body> MakeOperationsRequest(
    const std::string& aHost,
    const std::string& aToken)
{
    TinkoffApi::OperationRequest request;
    request.from = "2020-01-01T00:00:01.000000+03:00";
    request.to = "2020-02-01T00:00:01.000000+03:00";

    const auto targetString = HttpGetTargetWriter::GetTarget(request.GetTarget(), request);

    return MakeGetRequest(aHost, targetString, aToken);
}

http::request<http::string_body> MakeMarketStocksRequest(
    const std::string& aHost,
    const std::string& aToken)
{
    return MakeGetRequest(aHost, TinkoffApi::GetMarketStocksTarget(), aToken);
}

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

void SaveTrades(const std::vector<TradeToSave>& aTrades)
{
    std::cout << "save trades" << std::endl;
    if (aTrades.empty())
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

        for (const auto& trade : aTrades)
        {
            fileStream << std::endl << trade;
        }
    }

    fileStream.close();
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "usage: TinkoffInvest {TOKEN}";
        return EXIT_FAILURE;
    }

    std::string token = argv[1];

    const std::string host = "api-invest.tinkoff.ru";
    const std::string port = "443";

    std::map<std::string, std::string> figiToName;

    auto marketStocksProcessor = [&figiToName](const TinkoffApi::MarketStocksResponse& aResponse)
    {
        for (const auto& instrument : aResponse.instruments)
        {
            assert(!instrument.figi.empty() && !instrument.name.empty());
            figiToName[instrument.figi] = instrument.name;
        }
    };

    std::vector<TradeToSave> trades;
    auto operationsProcessor = [&trades, &figiToName](const TinkoffApi::OperationsResponse& aResponse)
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
                    trade.InstrumentName = figiToName.at(operation.figi);
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
                trades.emplace_back(trade);
            }
        }
    };

    JsonParser parser(
        marketStocksProcessor,
        operationsProcessor);

    SimpleSslHttpClient client;

    try
    {
        client.Connect(host, port);

        client.SendHttpRequest(MakeMarketStocksRequest(host, token));

        client.ProcessHttpResponse(
            std::bind(&JsonParser::Parse, &parser, std::placeholders::_1, std::placeholders::_2),
            TinkoffApi::ResponseType::MarketStocksResponse);

        client.SendHttpRequest(MakeOperationsRequest(host, token));

        client.ProcessHttpResponse(
            std::bind(&JsonParser::Parse, &parser, std::placeholders::_1, std::placeholders::_2),
            TinkoffApi::ResponseType::OperationsResponse);

        SaveTrades(trades);
    }
    catch (std::exception const& e)
    {
        client.Shutdown();
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
