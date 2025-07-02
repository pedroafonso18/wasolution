use reqwest;
use serde_json::{json, Value};
use crate::constants::consts::{MediaType, Status, StatusT};
use log::{info, debug, error};
use std::time::Instant;

pub struct Proxy {
    pub host: String,
    pub port: String,
    pub protocol: String,
    pub username: String,
    pub password: String
}

pub async fn send_message_e(phone: &str, token: &str, url: &str, m_type: &MediaType, msg_template: &str, inst_name: &str) -> Status {
    let start_time = Instant::now();
    info!("=== SEND MESSAGE (EVOLUTION) START ===");
    debug!("Phone: {}", phone);
    debug!("Token: {}", token);
    debug!("URL: {}", url);
    debug!("MediaType: {:?}", m_type);
    debug!("Instance Name: {}", inst_name);
    debug!("Message Template: {}", msg_template);

    let client = reqwest::Client::new();
    let req_body: Value;
    let req_url: String;
    if *m_type == MediaType::IMAGE {
        req_url = format!("{}/message/sendText/{}", url, inst_name);
        req_body = json!({
            "number": phone,
            "text": msg_template
        });
    } else if *m_type == MediaType::AUDIO {
        req_url = format!("{}/message/sendWhatsappAudio/{}", url, inst_name);
        req_body = json!({
            "number": phone,
            "audio": msg_template,
            "delay": 100
        });
    } else if *m_type == MediaType::IMAGE {
        req_url = format!("{}/message/sendMedia/{}", url, inst_name);
        let mut media_data: String = msg_template.to_string();
        if !media_data.is_empty() && media_data.starts_with("data:image/png;base64,") {
            media_data = media_data[22..].to_string();
            debug!("Removed data URL prefix from base64 data");
        } else if !media_data.is_empty() && media_data.starts_with("data:") {
            if let Some(comma_pos) = media_data.find(',') {
                media_data = media_data[(comma_pos + 1)..].to_string();
                debug!("Removed data URL prefix from base64 data");
            }
        }
        req_body = json!({
            "number": phone,
            "media" : media_data,
            "mimetype" : "image/png",
            "caption" : "",
            "fileName": "imagem.png"
        });
    } else {
        req_url = "invalid".to_string();
        req_body = json!({
            "error":"Invalid MediaType"
        });
    }
    debug!("URL da requisição: {}", req_url);
    debug!("Corpo da requisição: {}", req_body);

    let response = client
        .post(req_url)
        .header("accept", "application/json")
        .header("Content-Type", "application/json")
        .header("apikey", token)
        .body(serde_json::to_string(&req_body).unwrap())
        .send()
        .await;

    let (status_code, json_status) = match response {
        Ok(resp) => {
            let http_status = resp.status();
            let http_code = http_status.as_u16();
            let text = resp.text().await.unwrap_or_else(|_| "".to_string());
            debug!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", text);
            match serde_json::from_str::<serde_json::Value>(&text) {
                Ok(json) => {
                    if http_status.is_success() {
                        info!("Mensagem enviada com sucesso para: {}", phone);
                        (StatusT::OK, json)
                    } else {
                        error!("Erro HTTP ao enviar mensagem - Código: {}", http_code);
                        let mut json = json;
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                        }
                        (StatusT::ERR, json)
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta do envio: {}", e);
                    if !http_status.is_success() {
                        (StatusT::ERR, json!({
                            "error": "Erro no servidor remoto",
                            "raw_response": text
                        }))
                    } else {
                        (StatusT::OK, json!({
                            "raw_response": text
                        }))
                    }
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            (StatusT::ERR, json!({"error": format!("Request failed: {}", e)}))
        }
    };

    let duration = start_time.elapsed().as_millis();
    info!("=== SEND MESSAGE (EVOLUTION) END - Duração: {}ms ===", duration);

    Status {
        status_code,
        json_status,
    }
}

impl Proxy {
    pub fn parse_proxy(proxy_url: &str) -> Proxy {
        log::debug!("Analisando URL do proxy: {}", proxy_url);
        let mut proxy = Proxy {
            host: String::new(),
            port: String::new(),
            protocol: String::new(),
            username: String::new(),
            password: String::new(),
        };
        if let Some(proto_end) = proxy_url.find("://") {
            proxy.protocol = proxy_url[..proto_end].to_string();
            let creds_start = proto_end + 3;
            let at_pos = proxy_url[creds_start..].find('@').map(|p| p + creds_start);
            let mut host_start = creds_start;
            if let Some(at_pos) = at_pos {
                let creds = &proxy_url[creds_start..at_pos];
                if let Some(colon_pos) = creds.find(':') {
                    proxy.username = creds[..colon_pos].to_string();
                    proxy.password = creds[(colon_pos + 1)..].to_string();
                } else {
                    proxy.username = creds.to_string();
                }
                host_start = at_pos + 1;
            }
            let after_host = &proxy_url[host_start..];
            if let Some(colon_pos) = after_host.find(':') {
                proxy.host = after_host[..colon_pos].to_string();
                let port_start = colon_pos + 1;
                if let Some(slash_pos) = after_host[port_start..].find('/') {
                    proxy.port = after_host[port_start..port_start + slash_pos].to_string();
                } else {
                    proxy.port = after_host[port_start..].to_string();
                }
            } else {
                if let Some(slash_pos) = after_host.find('/') {
                    proxy.host = after_host[..slash_pos].to_string();
                } else {
                    proxy.host = after_host.to_string();
                }
            }
            log::debug!("Proxy analisado com sucesso: {}://{}:{}", proxy.protocol, proxy.host, proxy.port);
        } else {
            log::error!("URL do proxy inválida: protocolo não encontrado");
            return Proxy {
                host: String::new(),
                port: String::new(),
                protocol: String::new(),
                username: String::new(),
                password: String::new(),
            };
        }
        proxy
    }
}

pub async fn create_instance_e(
    evo_token: &str,
    inst_token: &str,
    inst_name: &str,
    url: &str,
    webhook_url: &str,
    proxy_url: &str,
) -> Status {
    let start_time = Instant::now();
    info!("=== CREATE INSTANCE (EVOLUTION) START ===");
    info!("Criando instância Evolution: {}", inst_name);
    debug!("Token Evolution: {}", evo_token);
    debug!("Token da instância: {}", inst_token);
    debug!("URL da API: {}", url);
    debug!("URL do webhook: {}", webhook_url);
    debug!("URL do proxy: {}", proxy_url);

    if evo_token.is_empty() {
        error!("Token Evolution inválido: evo_token está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid Evolution token: evo_token is empty"}),
        };
    }
    if inst_token.is_empty() {
        error!("Token da instância inválido: inst_token está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid instance token: inst_token is empty"}),
        };
    }
    if inst_name.is_empty() {
        error!("Nome da instância inválido: inst_name está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid instance name: inst_name is empty"}),
        };
    }
    if url.is_empty() {
        error!("URL da API inválida: url está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid API URL: url is empty"}),
        };
    }

    let prox = Proxy::parse_proxy(proxy_url);
    let req_url = format!("{}/instance/create", url);
    let req_body = if !prox.host.is_empty() && !webhook_url.is_empty() {
        debug!("Webhook e proxy configurados para a instância");
        json!({
            "instanceName": inst_name,
            "token": inst_token,
            "integration": "WHATSAPP-BAILEYS",
            "qrcode": true,
            "webhook": {
                "url": webhook_url,
                "byEvents": false,
                "base64": true,
                "events": ["MESSAGES_UPSERT"]
            },
            "proxyHost": prox.host,
            "proxyPort": prox.port,
            "proxyProtocol": prox.protocol,
            "proxyUsername": prox.username,
            "proxyPassword": prox.password
        })
    } else if !prox.host.is_empty() && webhook_url.is_empty() {
        debug!("Proxy configurado para a instância");
        json!({
            "instanceName": inst_name,
            "token": inst_token,
            "integration": "WHATSAPP-BAILEYS",
            "qrcode": true,
            "proxyHost": prox.host,
            "proxyPort": prox.port,
            "proxyProtocol": prox.protocol,
            "proxyUsername": prox.username,
            "proxyPassword": prox.password
        })
    } else if prox.host.is_empty() && !webhook_url.is_empty() {
        debug!("Webhook configurado para a instância");
        json!({
            "instanceName": inst_name,
            "token": inst_token,
            "integration": "WHATSAPP-BAILEYS",
            "qrcode": true,
            "webhook": {
                "url": webhook_url,
                "byEvents": false,
                "base64": true,
                "events": ["MESSAGES_UPSERT"]
            }
        })
    } else {
        json!({
            "instanceName": inst_name,
            "token": inst_token,
            "integration": "WHATSAPP-BAILEYS"
        })
    };
    debug!("URL da requisição: {}", req_url);
    debug!("Corpo da requisição: {}", req_body);

    let client = reqwest::Client::new();
    let response = client
        .post(&req_url)
        .header("accept", "application/json")
        .header("Content-Type", "application/json")
        .header("apikey", evo_token)
        .json(&req_body)
        .send()
        .await;

    let (status_code, json_status) = match response {
        Ok(resp) => {
            let http_status = resp.status();
            let http_code = http_status.as_u16();
            let text = resp.text().await.unwrap_or_else(|_| "".to_string());
            debug!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", text);
            match serde_json::from_str::<serde_json::Value>(&text) {
                Ok(json) => {
                    if http_status.is_success() {
                        info!("Instância criada com sucesso: {}", inst_name);
                        (StatusT::OK, json)
                    } else {
                        error!("Erro HTTP ao criar instância - Código: {}", http_code);
                        let mut json = json;
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                        }
                        (StatusT::ERR, json)
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta da criação: {}", e);
                    if !http_status.is_success() {
                        (StatusT::ERR, json!({
                            "error": "Erro no servidor remoto",
                            "raw_response": text
                        }))
                    } else {
                        (StatusT::OK, json!({
                            "raw_response": text
                        }))
                    }
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            (StatusT::ERR, json!({"error": format!("Request failed: {}", e)}))
        }
    };

    let duration = start_time.elapsed().as_millis();
    info!("=== CREATE INSTANCE (EVOLUTION) END - Duração: {}ms ===", duration);

    Status {
        status_code,
        json_status,
    }
}

pub async fn delete_instance_e(inst_token: &str, evo_token: &str, evo_url: &str) -> Status {
    let start_time = Instant::now();
    info!("=== DELETE INSTANCE (EVOLUTION) START ===");
    debug!("Instance Token: {}", inst_token);
    debug!("Evo Token: {}", evo_token);
    debug!("Evo URL: {}", evo_url);
    let client = reqwest::Client::new();
    let req_url = format!("{}/instance/delete/{}", evo_url, inst_token);
    debug!("URL da requisição: {}", req_url);
    let response = client.delete(req_url)
        .header("apikey", evo_token)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .send()
        .await;
    let (status_code, json_status) = match response {
        Ok(resp) => {
            let http_status = resp.status();
            let http_code = http_status.as_u16();
            let text = resp.text().await.unwrap_or_else(|_| "".to_string());
            debug!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", text);
            match serde_json::from_str::<serde_json::Value>(&text) {
                Ok(json) => {
                    if http_status.is_success() {
                        info!("Instância deletada com sucesso: {}", inst_token);
                        (StatusT::OK, json)
                    } else {
                        error!("Erro HTTP ao deletar instância - Código: {}", http_code);
                        let mut json = json;
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                        }
                        (StatusT::ERR, json)
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta de deletar: {}", e);
                    if !http_status.is_success() {
                        (StatusT::ERR, json!({
                            "error": "Erro no servidor remoto",
                            "raw_response": text
                        }))
                    } else {
                        (StatusT::OK, json!({
                            "raw_response": text
                        }))
                    }
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            (StatusT::ERR, json!({"error": format!("Request failed: {}", e)}))
        }
    };
    let duration = start_time.elapsed().as_millis();
    info!("=== DELETE INSTANCE (EVOLUTION) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

pub async fn connect_instance_e(inst_token: &str, evo_url: &str, evo_token: &str) -> Status {
    let start_time = Instant::now();
    info!("=== CONNECT INSTANCE (EVOLUTION) START ===");
    debug!("Instance Token: {}", inst_token);
    debug!("Evo Token: {}", evo_token);
    debug!("Evo URL: {}", evo_url);
    let client = reqwest::Client::new();
    let req_url = format!("{}/instance/connect/{}", evo_url, inst_token);
    debug!("URL da requisição: {}", req_url);
    let response = client.get(req_url)
        .header("apikey", evo_token)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .send()
        .await;
    let (status_code, json_status) = match response {
        Ok(resp) => {
            let http_status = resp.status();
            let http_code = http_status.as_u16();
            let text = resp.text().await.unwrap_or_else(|_| "".to_string());
            debug!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", text);
            match serde_json::from_str::<serde_json::Value>(&text) {
                Ok(json) => {
                    if http_status.is_success() {
                        info!("Instância conectada com sucesso: {}", inst_token);
                        (StatusT::OK, json)
                    } else {
                        error!("Erro HTTP ao conectar instância - Código: {}", http_code);
                        let mut json = json;
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                        }
                        (StatusT::ERR, json)
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta da conexão: {}", e);
                    if !http_status.is_success() {
                        (StatusT::ERR, json!({
                            "error": "Erro no servidor remoto",
                            "raw_response": text
                        }))
                    } else {
                        (StatusT::OK, json!({
                            "raw_response": text
                        }))
                    }
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            (StatusT::ERR, json!({"error": format!("Request failed: {}", e)}))
        }
    };
    let duration = start_time.elapsed().as_millis();
    info!("=== CONNECT INSTANCE (EVOLUTION) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

pub async fn logout_instance_e(inst_token: &str, evo_url: &str, evo_token: &str) -> Status {
    let start_time = Instant::now();
    info!("=== LOGOUT INSTANCE (EVOLUTION) START ===");
    debug!("Instance Token: {}", inst_token);
    debug!("Evo Token: {}", evo_token);
    debug!("Evo URL: {}", evo_url);
    let client = reqwest::Client::new();
    let req_url = format!("{}/instance/logout/{}", evo_url, inst_token);
    debug!("URL da requisição: {}", req_url);
    let response = client.delete(req_url)
        .header("apikey", evo_token)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .send()
        .await;
    let (status_code, json_status) = match response {
        Ok(resp) => {
            let http_status = resp.status();
            let http_code = http_status.as_u16();
            let text = resp.text().await.unwrap_or_else(|_| "".to_string());
            debug!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", text);
            match serde_json::from_str::<serde_json::Value>(&text) {
                Ok(json) => {
                    if http_status.is_success() {
                        info!("Instância deslogada com sucesso: {}", inst_token);
                        (StatusT::OK, json)
                    } else {
                        error!("Erro HTTP ao deslogar instância - Código: {}", http_code);
                        let mut json = json;
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                        }
                        (StatusT::ERR, json)
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta de deslogamento: {}", e);
                    if !http_status.is_success() {
                        (StatusT::ERR, json!({
                            "error": "Erro no servidor remoto",
                            "raw_response": text
                        }))
                    } else {
                        (StatusT::OK, json!({
                            "raw_response": text
                        }))
                    }
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            (StatusT::ERR, json!({"error": format!("Request failed: {}", e)}))
        }
    };
    let duration = start_time.elapsed().as_millis();
    info!("=== LOGOUT INSTANCE (EVOLUTION) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

pub async fn set_webhook_e(inst_token: &str, evo_url: &str, evo_token: &str, webhook_url: &str) -> Status {
    let start_time = Instant::now();
    info!("=== SET WEBHOOK (EVOLUTION) START ===");
    debug!("Instance Token: {}", inst_token);
    debug!("Evo Token: {}", evo_token);
    debug!("Evo URL: {}", evo_url);
    debug!("Webhook URL: {}", webhook_url);
    let client = reqwest::Client::new();
    let req_url = format!("{}/webhook/set/{}", evo_url, inst_token);
    let req_body = json!({
        "enabled": true,
        "url" : webhook_url,
        "webhookByEvents": false,
        "webhookBase64" : true,
        "events" : ["APPLICATION_STARTUP", "MESSAGE_UPSERT"]
    });
    debug!("URL da requisição: {}", req_url);
    debug!("Corpo da requisição: {}", req_body);
    let response = client.get(req_url)
        .header("apikey", evo_token)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .json(&req_body)
        .send()
        .await;
    let (status_code, json_status) = match response {
        Ok(resp) => {
            let http_status = resp.status();
            let http_code = http_status.as_u16();
            let text = resp.text().await.unwrap_or_else(|_| "".to_string());
            debug!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", text);
            match serde_json::from_str::<serde_json::Value>(&text) {
                Ok(json) => {
                    if http_status.is_success() {
                        info!("Webhook conectado com sucesso: {}", inst_token);
                        (StatusT::OK, json)
                    } else {
                        error!("Erro HTTP ao conectar webhook - Código: {}", http_code);
                        let mut json = json;
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                        }
                        (StatusT::ERR, json)
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta conexão de webhook: {}", e);
                    if !http_status.is_success() {
                        (StatusT::ERR, json!({
                            "error": "Erro no servidor remoto",
                            "raw_response": text
                        }))
                    } else {
                        (StatusT::OK, json!({
                            "raw_response": text
                        }))
                    }
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            (StatusT::ERR, json!({"error": format!("Request failed: {}", e)}))
        }
    };
    let duration = start_time.elapsed().as_millis();
    info!("=== SET WEBHOOK (EVOLUTION) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}