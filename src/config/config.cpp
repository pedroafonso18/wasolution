#include "config.h"
#include "../../dependencies/dotenv.h"
#include "../constants.h"
#include <filesystem>
#include <iostream>
#include <boost/asio.hpp>

Config::Config() {
    std::string root_path = "../.env";
    if (std::filesystem::exists(".env")) {
        root_path = ".env";
    }

    std::cout << "Tentando carregar .env de: " << root_path << std::endl;
    dotenv::init(root_path.c_str());
    loadEnv();

    std::cout << "EVO_URL carregada: [" << env_vars.evo_url << "]" << std::endl;
}


void Config::loadEnv() {
    env_vars.evo_url = dotenv::getenv("EVO_URL", "");
    env_vars.evo_token = dotenv::getenv("EVO_TOKEN", "");
    env_vars.wuz_url = dotenv::getenv("WUZ_URL", "");
    env_vars.db_url = dotenv::getenv("DB_URL", "");
    env_vars.db_url_wuz = dotenv::getenv("DB_URL_WUZ", "");
    env_vars.default_webhook = dotenv::getenv("DEFAULT_WEBHOOK", "");
    env_vars.ip_address = dotenv::getenv("IP_ADDRESS", getIpAddress());
}

const Env& Config::getEnv() const {
    return env_vars;
}

const std::string Config::getIpAddress() const {
    try {
        boost::asio::io_context io_context;

        boost::asio::ip::udp::resolver resolver(io_context);
        boost::asio::ip::udp::endpoint remote_endpoint =
            *resolver.resolve(boost::asio::ip::udp::v4(), "8.8.8.8", "80").begin();

        boost::asio::ip::udp::socket socket(io_context);
        socket.connect(remote_endpoint);

        boost::asio::ip::udp::endpoint local_endpoint = socket.local_endpoint();
        return local_endpoint.address().to_string();
    }
    catch (std::exception& e) {
        std::cerr << "Error getting local IP: " << e.what() << std::endl;
        return "localhost";
    }
}