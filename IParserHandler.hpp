#pragma once

namespace TinkoffApi
{
    struct MarketStocksResponse;
    struct OperationsResponse;
}

struct IParserHandler
{
    virtual void OnMessageParsed(const TinkoffApi::MarketStocksResponse&) = 0;
    virtual void OnMessageParsed(const TinkoffApi::OperationsResponse&) = 0;
};
