use reqwest;
use serde_json::{json, Value};
use crate::constants::consts::{MediaType, Status, StatusT};
use log::{info, debug, error, warn};
use std::time::Instant;
use tokio_postgres::Client;
use crate::database::fetch;

pub async fn set_proxy_w(inst_token: &str, proxy_url: &str, wuz_url: &str) -> Status {
    let start_time = Instant::now();
    info!("=== SET PROXY (WUZAPI) START ===");
    debug!("Instance Token: {}", inst_token);
    debug!("Proxy Url: {}", proxy_url);
    debug!("Wuz URL: {}", wuz_url);
    let client = reqwest::Client::new();
    let req_url = format!("{}/proxy", wuz_url);
    let req_body = json!({
        "proxy_url": proxy_url,
        "enable" : true
    });
    debug!("URL da requisição: {}", req_url);
    debug!("Corpo da requisição: {}", req_body);
    let response = client.post(req_url)
        .header("token", inst_token)
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
                        info!("Proxy conectado com sucesso: {}", inst_token);
                        (StatusT::OK, json)
                    } else {
                        error!("Erro HTTP ao conectar proxy - Código: {}", http_code);
                        let mut json = json;
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                        }
                        (StatusT::ERR, json)
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta conexão de proxy: {}", e);
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
    info!("=== SET PROXY (WUZAPI) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

pub async fn get_qr_code_w(client: &Client, inst_token: &str, wuz_url: &str) -> Status {
    let start_time = Instant::now();
    info!("=== GET QR CODE (WUZAPI) START ===");
    info!("Buscando QR Code para instância: {}", inst_token);
    debug!("Instance Token: {}", inst_token);
    debug!("Wuz URL: {}", wuz_url);

    if inst_token.is_empty() {
        error!("Token inválido: token está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid token: token is empty"}),
        };
    }
    if wuz_url.is_empty() {
        error!("URL da API inválida: url está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid API URL: url is empty"}),
        };
    }

    let http_client = reqwest::Client::new();
    let req_url = format!("{}/session/qr", wuz_url);
    debug!("URL da requisição: {}", req_url);
    let response = http_client.get(&req_url)
        .header("token", inst_token)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .send()
        .await;

    let status_code;
    let mut json_status;
    let http_ok;
    let response_body;
    let http_code;

    match response {
        Ok(resp) => {
            let http_status = resp.status();
            http_code = http_status.as_u16();
            response_body = resp.text().await.unwrap_or_else(|_| "".to_string());
            debug!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", response_body);
            http_ok = http_status.is_success();
            match serde_json::from_str::<serde_json::Value>(&response_body) {
                Ok(json) => {
                    let mut json_clone = json.clone();
                    if !http_ok {
                        error!("Erro HTTP ao buscar QR Code - Código: {}", http_code);
                        status_code = StatusT::ERR;
                        json_status = json_clone.clone();
                        if !json_clone.get("error").is_some() {
                            json_clone["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                            json_status = json_clone.clone();
                        }
                    } else {
                        status_code = StatusT::OK;
                        json_status = json_clone.clone();
                    }

                    let mut need_qr_from_db = false;
                    if let Some(data) = json_clone.get("data") {
                        if data.is_object() {
                            if let Some(qr) = data.get("QRCode") {
                                if qr.is_null() || (qr.is_string() && qr.as_str().unwrap_or("").is_empty()) {
                                    debug!("QRCode vazio encontrado na resposta da API - buscando no banco de dados");
                                    need_qr_from_db = true;
                                }
                            }
                        }
                    }

                    if need_qr_from_db {
                        info!("Iniciando busca do QR Code no banco de dados para token: {}", inst_token);
                        debug!("Aguardando 500ms antes de buscar o QR Code no banco...");
                        tokio::time::sleep(std::time::Duration::from_millis(500)).await;
                        let qr_code = fetch::get_qr_code(client, inst_token).await.ok();
                        let mut qr_code_val = qr_code.unwrap_or_default();
                        if qr_code_val.is_empty() {
                            debug!("QR Code não encontrado na primeira tentativa, aguardando mais 1.5 segundos");
                            tokio::time::sleep(std::time::Duration::from_millis(1500)).await;
                            qr_code_val = fetch::get_qr_code(client, inst_token).await.ok().unwrap_or_default();
                        }
                        if !qr_code_val.is_empty() {
                            info!("QR Code encontrado no banco de dados para token: {}", inst_token);
                            if let Some(data) = json_clone.get_mut("data") {
                                if data.is_object() {
                                    data["QRCode"] = serde_json::json!(qr_code_val);
                                }
                            } else {
                                json_clone["data"] = json!({"QRCode": qr_code_val});
                            }
                            json_status = json_clone;
                        } else {
                            error!("QR Code não encontrado no banco de dados após múltiplas tentativas para o token: {}", inst_token);
                        }
                    } else {
                        info!("QR Code já presente na resposta da API para token: {}", inst_token);
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta do QR Code: {}", e);
                    if !http_ok {
                        status_code = StatusT::ERR;
                        json_status = json!({
                            "error": "Erro no servidor remoto",
                            "raw_response": response_body
                        });
                    } else {
                        status_code = StatusT::OK;
                        json_status = json!({
                            "raw_response": response_body
                        });
                    }
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            status_code = StatusT::ERR;
            json_status = json!({"error": format!("Request failed: {}", e)});
        }
    }

    let duration = start_time.elapsed().as_millis();
    info!("=== GET QR CODE (WUZAPI) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

pub async fn set_webhook_w(inst_token: &str, webhook_url: &str, wuz_url: &str ) -> Status {
    let start_time = Instant::now();
    info!("=== SET WEBHOOK (WUZAPI) START ===");
    debug!("Instance Token: {}", inst_token);
    debug!("Webhook Url: {}", webhook_url);
    debug!("Wuz URL: {}", wuz_url);
    let client = reqwest::Client::new();
    let req_url = format!("{}/webhook", wuz_url);
    let req_body = json!({
        "webhook": webhook_url,
        "data" : [
            "Message",
            "ReadReceipt",
            "Presence",
            "HistorySync",
            "ChatPresence"
        ]
    });
    debug!("URL da requisição: {}", req_url);
    debug!("Corpo da requisição: {}", req_body);
    let response = client.post(req_url)
        .header("token", inst_token)
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
    info!("=== SET WEBHOOK (WUZAPI) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

pub async fn send_message_w(phone: &str, token: &str, url: &str, m_type: &MediaType, msg_template: &str) -> Status {
    let start_time = Instant::now();
    info!("=== SEND MESSAGE (WUZAPI) START ===");
    debug!("Phone: {}", phone);
    debug!("Token: {}", token);
    debug!("URL: {}", url);
    debug!("MediaType: {:?}", m_type);
    debug!("Message Template: {}", msg_template);

    let client = reqwest::Client::new();
    let req_body: Value;
    let req_url: String;
    if *m_type == MediaType::TEXT {
        req_url = format!("{}/chat/send/text", url);
        req_body = json!({
            "Phone": phone,
            "Body": msg_template
        });
    } else if *m_type == MediaType::AUDIO {
        req_url = format!("{}/chat/send/audio", url);
        req_body = json!({
            "Phone": phone,
            "Audio": msg_template
        });
    } else if *m_type == MediaType::IMAGE {
        req_url = format!("{}/chat/send/image", url);
        
        req_body = json!({
            "Phone": phone,
            "Image" : msg_template,
            "Caption" : ""
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
        .header("token", token)
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
    info!("=== SEND MESSAGE (WUZAPI) END - Duração: {}ms ===", duration);

    Status {
        status_code,
        json_status,
    }
}

pub async fn create_instance_w(
    client: &Client,
    inst_token: &str,
    inst_name: &str,
    url: &str,
    webhook_url: &str,
    proxy_url: &str,
    wuz_admin_token: &str,
) -> Status {
    let start_time = Instant::now();
    info!("=== CREATE INSTANCE (WUZAPI) START ===");
    info!("Criando instância WuzAPI: {}", inst_name);
    debug!("Token da instância: {}", inst_token);
    debug!("URL da API: {}", url);
    debug!("URL do webhook: {}", webhook_url);
    debug!("URL do proxy: {}", proxy_url);
    debug!("Admin Token: {}", wuz_admin_token);

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
    if wuz_admin_token.is_empty() {
        error!("Token de administrador inválido: wuz_admin_token está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid admin token: wuz_admin_token is empty"}),
        };
    }

    let http_client = reqwest::Client::new();
    let req_url = format!("{}/admin/users", url);
    let mut req_body_json = serde_json::Map::new();
    req_body_json.insert("name".to_string(), json!(inst_name));
    req_body_json.insert("token".to_string(), json!(inst_token));
    if !webhook_url.is_empty() {
        req_body_json.insert("webhook".to_string(), json!(webhook_url));
        req_body_json.insert("events".to_string(), json!("All"));
        debug!("Webhook configurado para a instância");
    }
    if !proxy_url.is_empty() {
        req_body_json.insert("proxyConfig".to_string(), json!({
            "enabled": true,
            "proxyURL": proxy_url
        }));
        debug!("Proxy configurado para a instância");
    }
    let req_body = Value::Object(req_body_json);
    debug!("Criando instância WuzAPI");
    debug!("URL: {}", req_url);
    debug!("Request body: {}", req_body);

    let response = http_client
        .post(&req_url)
        .header("Authorization", wuz_admin_token)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .json(&req_body)
        .send()
        .await;

    let mut status_code;
    let mut json_status;
    let mut http_ok = false;
    let mut response_body = String::new();
    let mut http_code = 0u16;

    match response {
        Ok(resp) => {
            let http_status = resp.status();
            http_code = http_status.as_u16();
            response_body = resp.text().await.unwrap_or_else(|_| "".to_string());
            debug!("Código de resposta HTTP: {}", http_code);
            debug!("HTTP Response: {}", response_body);
            http_ok = http_status.is_success();
            match serde_json::from_str::<serde_json::Value>(&response_body) {
                Ok(mut json) => {
                    if !http_ok {
                        error!("Erro HTTP na criação da instância - Código: {}", http_code);
                        status_code = StatusT::ERR;
                        json_status = json.clone();
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Server returned HTTP error code");
                            json_status = json;
                        }
                        return Status { status_code, json_status };
                    }
                    info!("Instância WuzAPI criada com sucesso: {}", inst_name);
                    status_code = StatusT::OK;
                    json_status = json;
                },
                Err(e) => {
                    error!("Erro ao processar resposta da criação da instância: {}", e);
                    if !http_ok {
                        status_code = StatusT::ERR;
                        json_status = json!({
                            "error": "Error on remote server",
                            "raw_response": response_body
                        });
                    } else {
                        status_code = StatusT::OK;
                        json_status = json!({
                            "raw_response": response_body
                        });
                    }
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            status_code = StatusT::ERR;
            json_status = json!({"error": format!("Request failed: {}", e)});
        }
    }

    info!("Conectando instância após criação...");
    let conn = connect_instance_w(inst_token, url).await;
    if conn.status_code == StatusT::ERR {
        error!("Falha ao conectar instância após criação");
        return conn;
    }

    info!("Buscando QR Code para instância recém-criada...");
    let duration = start_time.elapsed().as_millis();
    info!("=== CREATE INSTANCE (WUZAPI) END - Duração: {}ms ===", duration);
    get_qr_code_w(client, inst_token, url).await
}

pub async fn connect_instance_w(inst_token: &str, url: &str) -> Status {
    let start_time = Instant::now();
    info!("=== CONNECT INSTANCE (WUZAPI) START ===");
    info!("Conectando instância WuzAPI: {}", inst_token);
    debug!("Instance Token: {}", inst_token);
    debug!("URL da API: {}", url);

    if inst_token.is_empty() {
        error!("Token da instância inválido: inst_token está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid instance token: inst_token is empty"}),
        };
    }
    if url.is_empty() {
        error!("URL da API inválida: url está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid API URL: url is empty"}),
        };
    }

    let http_client = reqwest::Client::new();
    let req_url = format!("{}/session/connect", url);
    let req_body = json!({
        "Subscribe": [
            "Message",
            "ReadReceipt",
            "Presence",
            "HistorySync",
            "ChatPresence"
        ],
        "Immediate": true
    });
    debug!("URL da requisição: {}", req_url);
    debug!("Corpo da requisição: {}", req_body);

    let response = http_client
        .post(&req_url)
        .header("token", inst_token)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .json(&req_body)
        .send()
        .await;

    let status_code;
    let mut json_status;
    let http_ok;
    let response_body;
    let http_code;

    match response {
        Ok(resp) => {
            let http_status = resp.status();
            http_code = http_status.as_u16();
            response_body = resp.text().await.unwrap_or_else(|_| "".to_string());
            debug!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", response_body);
            http_ok = http_status.is_success();
            match serde_json::from_str::<serde_json::Value>(&response_body) {
                Ok(mut json) => {
                    if !http_ok {
                        error!("Erro HTTP na conexão da instância - Código: {}", http_code);
                        status_code = StatusT::ERR;
                        json_status = json.clone();
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                            json_status = json;
                        }
                    } else {
                        info!("Instância conectada com sucesso: {}", inst_token);
                        status_code = StatusT::OK;
                        json_status = json;
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta da conexão da instância: {}", e);
                    if !http_ok {
                        status_code = StatusT::ERR;
                        json_status = json!({
                            "error": "Erro no servidor remoto",
                            "raw_response": response_body
                        });
                    } else {
                        status_code = StatusT::OK;
                        json_status = json!({
                            "raw_response": response_body
                        });
                    }
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            status_code = StatusT::ERR;
            json_status = json!({"error": format!("Request failed: {}", e)});
        }
    }

    let duration = start_time.elapsed().as_millis();
    info!("=== CONNECT INSTANCE (WUZAPI) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

pub async fn logout_instance_w(inst_token: &str, url: &str) -> Status {
    let start_time = Instant::now();
    info!("=== LOGOUT INSTANCE (WUZAPI) START ===");
    info!("Desconectando instância WuzAPI: {}", inst_token);
    debug!("Instance Token: {}", inst_token);
    debug!("URL da API: {}", url);

    if inst_token.is_empty() {
        error!("Token da instância inválido: inst_token está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid instance token: inst_token is empty"}),
        };
    }
    if url.is_empty() {
        error!("URL da API inválida: url está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid API URL: url is empty"}),
        };
    }

    let http_client = reqwest::Client::new();
    let req_url = format!("{}/session/disconnect", url);
    debug!("URL da requisição: {}", req_url);

    let response = http_client
        .post(&req_url)
        .header("token", inst_token)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .send()
        .await;

    let status_code;
    let json_status;
    let response_body;
    let http_code;

    match response {
        Ok(resp) => {
            let http_status = resp.status();
            http_code = http_status.as_u16();
            response_body = resp.text().await.unwrap_or_else(|_| "".to_string());
            info!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", response_body);
            match serde_json::from_str::<serde_json::Value>(&response_body) {
                Ok(json) => {
                    info!("Instância desconectada com sucesso: {}", inst_token);
                    status_code = StatusT::OK;
                    json_status = json;
                },
                Err(e) => {
                    warn!("Erro ao processar resposta da desconexão, usando resposta bruta: {}", e);
                    status_code = StatusT::OK;
                    json_status = json!({"raw_response": response_body});
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            status_code = StatusT::ERR;
            json_status = json!({"error": format!("Request failed: {}", e)});
        }
    }

    let duration = start_time.elapsed().as_millis();
    info!("=== LOGOUT INSTANCE (WUZAPI) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

pub async fn delete_instance_w(inst_token: &str, url: &str, wuz_admin_token: &str) -> Status {
    let start_time = Instant::now();
    info!("=== DELETE INSTANCE (WUZAPI) START ===");
    info!("Deletando instância WuzAPI: {}", inst_token);
    debug!("Instance Token: {}", inst_token);
    debug!("URL da API: {}", url);
    debug!("Admin Token: {}", wuz_admin_token);

    if inst_token.is_empty() {
        error!("Token da instância inválido: inst_token está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid instance token: inst_token is empty"}),
        };
    }
    if url.is_empty() {
        error!("URL da API inválida: url está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid API URL: url is empty"}),
        };
    }
    if wuz_admin_token.is_empty() {
        error!("Token de administrador inválido: wuz_admin_token está vazio");
        return Status {
            status_code: StatusT::ERR,
            json_status: json!({"error": "Invalid admin token: wuz_admin_token is empty"}),
        };
    }

    let http_client = reqwest::Client::new();
    let req_url = format!("{}/admin/users/{}", url, inst_token);
    debug!("URL da requisição: {}", req_url);

    let response = http_client
        .delete(&req_url)
        .header("Authorization", wuz_admin_token)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .send()
        .await;

    let status_code;
    let json_status;
    let response_body;
    let http_code;

    match response {
        Ok(resp) => {
            let http_status = resp.status();
            http_code = http_status.as_u16();
            response_body = resp.text().await.unwrap_or_else(|_| "".to_string());
            info!("Código de resposta HTTP: {}", http_code);
            debug!("Resposta HTTP: {}", response_body);
            match serde_json::from_str::<serde_json::Value>(&response_body) {
                Ok(json) => {
                    info!("Instância deletada com sucesso: {}", inst_token);
                    status_code = StatusT::OK;
                    json_status = json;
                },
                Err(e) => {
                    warn!("Erro ao processar resposta da deleção, usando resposta bruta: {}", e);
                    status_code = StatusT::OK;
                    json_status = json!({"raw_response": response_body});
                }
            }
        },
        Err(e) => {
            error!("Erro de requisição: {}", e);
            status_code = StatusT::ERR;
            json_status = json!({"error": format!("Request failed: {}", e)});
        }
    }

    let duration = start_time.elapsed().as_millis();
    info!("=== DELETE INSTANCE (WUZAPI) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}