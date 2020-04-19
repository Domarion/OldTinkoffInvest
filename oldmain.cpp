#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <vector>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>


//http::request<http::string_body> MakeRegisterRequest(const std::string& aHost)
//{
//    const std::string tinkoffToken = "NOTHING";
//    http::request<http::string_body> req{http::verb::post, "/openapi/sandbox/sandbox/register", 11};
//    req.set(http::field::host, aHost);
//    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
//    req.set(http::field::authorization, "Bearer " + tinkoffToken);
//    return req;
//}
namespace TinkoffApi
{
    struct Portfolio
    {
        std::vector<std::string> positions;

        void print()
        {
            std::cout << "Portfolio\n{" << std::endl;

            for (const auto& position : positions)
            {
                std::cout << position << std::endl;
            }
            std::cout << "}" << std::endl;
        }
    };
}

struct JsonParser
{
public:
    JsonParser() = default;

    void Parse(const std::string& aJsonString)
    {
        std::string outError;
        if (!CheckJsonScheme(aJsonString, outError))
        {
            std::cerr << "Json parsing error: " << outError;
            return;
        }
        ParsePortfolio();
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

        portfolio.print();
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
    rapidjson::Document mDocument;
};

http::request<http::string_body> MakePortfolioRequest(const std::string& aHost)
{
        //PROD
    const std::string tinkoffToken = "NOTHING";
    http::request<http::string_body> req{http::verb::get, "/openapi/portfolio", 11};
    req.set(http::field::host, aHost);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, "Bearer " + tinkoffToken);
    return req;
}

void RequestResponse(ssl::stream<tcp::socket>& outStream, const http::request<http::string_body>& aHttpRequest)
{
    // Send the HTTP request to the remote host
    http::write(outStream, aHttpRequest);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::read(outStream, buffer, res);

    // Write the message to standard out
    std::cout << res << std::endl;

    std::string body = boost::beast::buffers_to_string(res.body().data());

    std::cout << "BODY:" << body << std::endl;

    JsonParser parser;
    parser.Parse(body);
}


int main(int /*argc*/, char** /*argv*/)
{
   try
   {
       const char* host = "XXXXX";
       const std::string port = "443";

       // The io_context is required for all I/O
       boost::asio::io_context ioc;

       // The SSL context is required
       ssl::context ctx{ssl::context::sslv23_client};


       // These objects perform our I/O
       tcp::resolver resolver{ioc};
       ssl::stream<tcp::socket> stream{ioc, ctx};

       // Set SNI Hostname (many hosts need this to handshake successfully)
       if(! SSL_set_tlsext_host_name(stream.native_handle(), host))
       {
           boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
           throw boost::system::system_error{ec};
       }

       // Look up the domain name
       auto const results = resolver.resolve(host, port);

       // Make the connection on the IP address we get from a lookup
       boost::asio::connect(stream.next_layer(), results.begin(), results.end());

       // Perform the SSL handshake
       stream.handshake(ssl::stream_base::client);

//        RequestResponse(stream, MakeRegisterRequest(host));
       RequestResponse(stream, MakePortfolioRequest(host));

       // Gracefully close the stream
       boost::system::error_code ec;
       stream.shutdown(ec);
       if(ec == boost::asio::error::eof)
       {
           // Rationale:
           // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
           ec.assign(0, ec.category());
       }
       if(ec)
           throw boost::system::system_error{ec};

       // If we get here then the connection is closed gracefully
   }
   catch(std::exception const& e)
   {
       std::cerr << "Error: " << e.what() << std::endl;
       return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}
