#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main(int argc, char** argv)
{
    try {
        if (argc != 4) {
            std::cerr << "Usage: " << argv[0] << " <host> <port> <message>" << std::endl;
            std::cerr << "Exemple: " << argv[0] << " 172.18.10.35 8080 \"Bonjour serveur!\"" << std::endl;
            return EXIT_FAILURE;
        }

        auto const host = argv[1];
        auto const port = argv[2];
        std::string message = argv[3];

        // Ajouter des parametres si besoin
        std::string body_content = "message=" + message;

        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        http::request<http::string_body> req{ http::verb::post, "/", 11 };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.body() = body_content;
        req.set(http::field::content_length, std::to_string(body_content.size()));
        req.set(http::field::content_type, "application/x-www-form-urlencoded");

        std::cout << "Envoi du message: " << body_content << std::endl;

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        std::cout << "Reponse du serveur:" << std::endl;
        std::cout << res << std::endl;

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    }
    catch (std::exception const& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}