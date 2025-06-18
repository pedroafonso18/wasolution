#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "handler/handler.h"
#include "../dependencies/json.h"
#include "logger/logger.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

Logger apiLogger("../logs/api.log");

http::response<http::string_body> handle_request(http::request<http::string_body> const& req) {
    apiLogger.info("Requisição recebida: " + std::string(req.method_string()) + " " + std::string(req.target()));

    if (req.method() == http::verb::post && req.target() == "/createInstance") {
        Config cfg;
        auto env = cfg.getEnv();
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        try {
            auto body = nlohmann::json::parse(req.body());
            std::string instance_id = body.at("instance_id").get<std::string>();
            std::string instance_name = body.at("instance_name").get<std::string>();
            std::string api_type_str = body.at("api_type").get<std::string>();
            std::string webhook_url = body.value("webhook_url", env.default_webhook);
            std::string proxy_url = body.value("proxy_url", "");

            apiLogger.debug("Criando instância: ID=" + instance_id + ", Nome=" + instance_name + ", Tipo=" + api_type_str);

            ApiType api_type;
            if (api_type_str == "EVOLUTION") {
                api_type = ApiType::EVOLUTION;
            } else if (api_type_str == "WUZAPI") {
                api_type = ApiType::WUZAPI;
            } else {
                apiLogger.error("Tipo de API inválido: " + api_type_str);
                res.result(http::status::bad_request);
                res.body() = R"({\"error\":\"api_type inválido\"})";
                res.prepare_payload();
                return res;
            }

            Status stat = Handler::createInstance(instance_id, instance_name, api_type, webhook_url, proxy_url);
            nlohmann::json resp_json;
            resp_json["status_code"] = stat.status_code;
            resp_json["status_string"] = stat.status_string;
            res.body() = resp_json.dump();

            if (stat.status_code == c_status::ERR) {
                res.result(http::status::internal_server_error);
            }
        } catch (const std::exception& e) {
            apiLogger.error("Erro ao processar requisição createInstance: " + std::string(e.what()));
            res.result(http::status::bad_request);
            nlohmann::json err_json;
            err_json["error"] = e.what();
            res.body() = err_json.dump();
        }
        res.prepare_payload();
        return res;
    }
    if (req.method() == http::verb::post && req.target() == "/sendMessage") {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        try {
            auto body = nlohmann::json::parse(req.body());
            std::string instance_id = body.at("instance_id").get<std::string>();
            std::string number = body.at("number").get<std::string>();
            std::string msg_body = body.at("body").get<std::string>();
            std::string type_str = body.value("type", "TEXT");

            apiLogger.debug("Enviando mensagem: Instância=" + instance_id + ", Número=" + number + ", Tipo=" + type_str);

            MediaType type;
            if (type_str == "TEXT") {
                type = MediaType::TEXT;
            } else if (type_str == "IMAGE") {
                type = MediaType::IMAGE;
            } else if (type_str == "AUDIO") {
                type = MediaType::AUDIO;
            } else {
                apiLogger.error("Tipo de mídia inválido: " + type_str);
                res.result(http::status::bad_request);
                res.body() = R"({\"error\":\"type inválido\"})";
                res.prepare_payload();
                return res;
            }
            Status stat = Handler::sendMessage(instance_id, number, msg_body, type);
            nlohmann::json resp_json;
            resp_json["status_code"] = stat.status_code;
            resp_json["status_string"] = stat.status_string;
            res.body() = resp_json.dump();

            if (stat.status_code == c_status::ERR) {
                res.result(http::status::internal_server_error);
            }
        } catch (const std::exception& e) {
            res.result(http::status::bad_request);
            nlohmann::json err_json;
            err_json["error"] = e.what();
            res.body() = err_json.dump();
        }
        res.prepare_payload();
        return res;
    }
    if (req.method() == http::verb::delete_ && req.target() == "/deleteInstance") {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        try {
            auto body = nlohmann::json::parse(req.body());
            std::string instance_id = body.at("instance_id").get<std::string>();
            Status stat = Handler::deleteInstance(instance_id);
            nlohmann::json resp_json;
            resp_json["status_code"] = stat.status_code;
            resp_json["status_string"] = stat.status_string;
            res.body() = resp_json.dump();

            if (stat.status_code == c_status::ERR) {
                res.result(http::status::internal_server_error);
            }
        } catch (const std::exception& e) {
            res.result(http::status::bad_request);
            nlohmann::json err_json;
            err_json["error"] = e.what();
            res.body() = err_json.dump();
        }
        res.prepare_payload();
        return res;
    }
    if (req.method() == http::verb::delete_ && req.target() == "/logoutInstance") {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        try {
            auto body = nlohmann::json::parse(req.body());
            std::string instance_id = body.at("instance_id").get<std::string>();
            Status stat = Handler::logoutInstance(instance_id);
            nlohmann::json resp_json;
            resp_json["status_code"] = stat.status_code;
            resp_json["status_string"] = stat.status_string;
            res.body() = resp_json.dump();

            if (stat.status_code == c_status::ERR) {
                res.result(http::status::internal_server_error);
            }
        } catch (const std::exception& e) {
            res.result(http::status::bad_request);
            nlohmann::json err_json;
            err_json["error"] = e.what();
            res.body() = err_json.dump();
        }
        res.prepare_payload();
        return res;
    }
    if (req.method() == http::verb::post && req.target() == "/connectInstance") {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        try {
            auto body = nlohmann::json::parse(req.body());
            std::string instance_id = body.at("instance_id").get<std::string>();
            Status stat = Handler::connectInstance(instance_id);
            nlohmann::json resp_json;
            resp_json["status_code"] = stat.status_code;
            resp_json["status_string"] = stat.status_string;
            res.body() = resp_json.dump();

            if (stat.status_code == c_status::ERR) {
                res.result(http::status::internal_server_error);
            }
        } catch (const std::exception& e) {
            res.result(http::status::bad_request);
            nlohmann::json err_json;
            err_json["error"] = e.what();
            res.body() = err_json.dump();
        }
        res.prepare_payload();
        return res;
    } if (req.method() == http::verb::post && req.target() == "/setWebhook") {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        try {
            auto body = nlohmann::json::parse(req.body());
            std::string instance_id = body.at("instance_id").get<std::string>();
            std::string webhook_url = body.at("webhook_url").get<std::string>();
            Status stat = Handler::setWebhook(instance_id,webhook_url);
            nlohmann::json resp_json;
            resp_json["status_code"] = stat.status_code;
            resp_json["status_string"] = stat.status_string;
            res.body() = resp_json.dump();

            if (stat.status_code == c_status::ERR) {
                res.result(http::status::internal_server_error);
            }
        } catch (const std::exception& e) {
            res.result(http::status::bad_request);
            nlohmann::json err_json;
            err_json["error"] = e.what();
            res.body() = err_json.dump();
        }
        res.prepare_payload();
        return res;
        } if (req.method() == http::verb::post && req.target() == "/webhook") {

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Beast");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());

        try {
            auto body = nlohmann::json::parse(req.body());
            std::string instance_id;

            if (body.contains("event") && body.contains("instance") && body.contains("data")) {
                if (body["data"].contains("instanceId")) {
                    instance_id = body["data"]["instanceId"];
                } else if (body.contains("instance")) {
                    instance_id = body["instance"];
                } else {
                    res.result(http::status::bad_request);
                    res.body() = R"({"error":"Webhook da Evolution sem instanceId válido"})";
                    res.prepare_payload();
                    return res;
                }
            } else if (body.contains("type") && body.contains("token") && body.contains("instance_name")) {
                instance_id = body["token"];
            } else if (body.contains("instance_id")) {
                instance_id = body.at("instance_id").get<std::string>();
            } else {
                res.result(http::status::bad_request);
                res.body() = R"({"error":"Formato de webhook desconhecido"})";
                res.prepare_payload();
                return res;
            }

            std::cerr << "Webhook recebido para instance_id: " << instance_id << std::endl;

            Status stat = Handler::sendWebhook(body, instance_id);
            nlohmann::json resp_json;
            resp_json["status_code"] = stat.status_code;
            resp_json["status_string"] = stat.status_string;
            res.body() = resp_json.dump();

            if (stat.status_code == c_status::ERR) {
                res.result(http::status::internal_server_error);
            }
        } catch (const std::exception& e) {
            res.result(http::status::bad_request);
            nlohmann::json err_json;
            err_json["error"] = e.what();
            res.body() = err_json.dump();

            std::cerr << "Erro ao processar webhook: " << e.what() << std::endl;
        }
        res.prepare_payload();
        return res;
    }
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, "Beast");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    nlohmann::json err_json;
    err_json["error"] = "Endpoint não encontrado";
    res.body() = err_json.dump();
    res.prepare_payload();
    return res;
}

class Session : public std::enable_shared_from_this<Session> {
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;

public:
    explicit Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void run() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        http::async_read(socket_, buffer_, req_, [this, self](beast::error_code ec, std::size_t) {
            if (!ec) {
                apiLogger.debug("Requisição recebida: " + std::string(req_.method_string()) + " " + std::string(req_.target()));
                do_write(handle_request(req_));
            } else {
                apiLogger.error("Erro ao ler requisição: " + std::string(ec.message()));
            }
        });
    }

    void do_write(http::response<http::string_body> res) {
        auto self(shared_from_this());
        auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
        http::async_write(socket_, *sp, [this, self, sp](beast::error_code ec, std::size_t) {
            if (ec) {
                apiLogger.error("Erro ao escrever resposta: " + std::string(ec.message()));
            }
            socket_.shutdown(tcp::socket::shutdown_send, ec);
        });
    }
};

class Listener : public std::enable_shared_from_this<Listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint)
        : ioc_(ioc), acceptor_(net::make_strand(ioc)) {
        beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            apiLogger.error("Erro ao abrir acceptor: " + std::string(ec.message()));
            return;
        }

        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            apiLogger.error("Erro ao configurar opção do socket: " + std::string(ec.message()));
            return;
        }

        acceptor_.bind(endpoint, ec);
        if (ec) {
            apiLogger.error("Erro ao fazer bind: " + std::string(ec.message()));
            return;
        }

        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            apiLogger.error("Erro ao iniciar listen: " + std::string(ec.message()));
            return;
        }
        apiLogger.info("Listener configurado com sucesso");
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(net::make_strand(ioc_), [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket))->run();
            }
            do_accept();
        });
    }
};

int main() {
    try {
        apiLogger.info("Iniciando servidor...");
        auto const address = net::ip::make_address(IP);
        apiLogger.info("Endereço IP configurado: " + std::string(IP));

        const int threads = std::thread::hardware_concurrency();
        apiLogger.info("Número de threads: " + std::to_string(threads));
        net::io_context ioc{threads};

        auto listener = std::make_shared<Listener>(ioc, tcp::endpoint{address, PORT});
        apiLogger.info("Listener criado na porta: " + std::to_string(PORT));
        listener->run();

        std::vector<std::thread> thread_pool;
        for (int i = 0; i < threads; ++i) {
            thread_pool.emplace_back([&ioc] { ioc.run(); });
        }
        apiLogger.info("Thread pool iniciado");

        for (auto& t : thread_pool) {
            t.join();
        }
        apiLogger.info("Thread pool finalizado");

    } catch (const std::exception& e) {
        apiLogger.error("Erro fatal: " + std::string(e.what()));
        std::cerr << "Error: " << e.what() << std::endl;
    }
}