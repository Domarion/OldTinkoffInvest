#pragma once

#include <cstdlib>
#include <iostream>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include "TinkoffApi.hpp"

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

inline http::request<http::string_body> MakeGetRequest(
    const std::string& aHost,
    const std::string& aTarget,
    const std::string& aToken)
{
    http::request<http::string_body> req{http::verb::get, aTarget, 11};
    req.set(http::field::host, aHost);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, "Bearer " + aToken);

    std::cout << req << std::endl;

    return req;
}

class SimpleSslHttpClient
{
public:
    using THandler = std::function<void(const std::string&, TinkoffApi::ResponseType)>;

    explicit SimpleSslHttpClient()
        : ioc()
        , ctx(ssl::context::sslv23_client)
        , resolver(ioc)
        , stream(ioc, ctx)
    {
    }

    void Connect(const std::string& aHost, const std::string& aPort)
    {
        if(! SSL_set_tlsext_host_name(stream.native_handle(), aHost.c_str()))
        {
            boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            throw boost::system::system_error{ec};
        }

        auto const results = resolver.resolve(aHost, aPort);

        boost::asio::connect(stream.next_layer(), results.begin(), results.end());

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);
    }

    void SendHttpRequest(const http::request<http::string_body>& aHttpRequest)
    {
        http::write(stream, aHttpRequest);
    }

    void ProcessHttpResponse(const THandler& aHandler, TinkoffApi::ResponseType aResponseType)
    {
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out
        std::cout << "Http response:" << res << std::endl;

        if (!aHandler)
        {
            std::cerr << "Response handler is not initialized";
            return;
        }

        aHandler(boost::beast::buffers_to_string(res.body().data()), aResponseType);
    }

    void Shutdown()
    {
        boost::system::error_code ec;
        stream.shutdown(ec);
        if(ec == boost::asio::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec.assign(0, ec.category());
        }

        if (ec)
        {
            boost::system::system_error ex{ec};
            std::cerr << "Shutdown error: " << ex.what() << std::endl;
        }
    }

    ~SimpleSslHttpClient()
    {
        Shutdown();
    }

private:
    boost::asio::io_context ioc;
    ssl::context ctx;
    tcp::resolver resolver;
    ssl::stream<tcp::socket> stream;
};

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
