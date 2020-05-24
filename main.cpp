#include "Parser.hpp"
#include "SslClient.hpp"
#include "TradesProcessor.hpp"

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

    auto processor = std::make_shared<TradesProcessor>();

    JsonParser parser(processor);

    SimpleSslHttpClient client;

    TinkoffApi::OperationRequest request;
    request.from = "2019-01-01T00:00:01.000000+03:00";
    request.to = "2020-04-24T00:00:01.000000+03:00";

    const auto operationsRequest = MakeOperationsRequest(request, host, token);
    try
    {
        client.Connect(host, port);

        client.SendHttpRequest(MakeMarketStocksRequest(host, token));

        client.ProcessHttpResponse(
            std::bind(&JsonParser::Parse, &parser, std::placeholders::_1, std::placeholders::_2),
            TinkoffApi::ResponseType::MarketStocksResponse);

        client.SendHttpRequest(operationsRequest);

        client.ProcessHttpResponse(
            std::bind(&JsonParser::Parse, &parser, std::placeholders::_1, std::placeholders::_2),
            TinkoffApi::ResponseType::OperationsResponse);

        processor->SaveTrades();
        processor->SaveProfitLoss(request.from, request.to);
    }
    catch (std::exception const& e)
    {
        client.Shutdown();
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
