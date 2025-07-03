use reqwest;
use serde_json::{json, Value};
use crate::constants::consts::{MediaType, Status, StatusT, FbVars, Template, TemplateType, HeaderT, Button};
use log::{info, debug, error};
use std::time::Instant;

async fn _subscribe_to_waba(waba_id: &str, access_token: &str, cloud_version: &f32) -> Status {
    let start_time = Instant::now();
    info!("=== SUBSCRIBE TO WABA (CLOUD) START ===");
    debug!("Waba ID: {}", waba_id);
    let client = reqwest::Client::new();
    let req_url = format!("https://graph.facebook.com/{}/{}/subscribed_apps", cloud_version, waba_id);
    debug!("URL da requisição: {}", req_url);
    let auth = format!("Bearer {}",access_token);
    let response = client.post(req_url)
        .header("Authorization", auth)
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
                        info!("Waba inscrita com sucesso: {}", waba_id);
                        (StatusT::OK, json)
                    } else {
                        error!("Erro HTTP ao se inscrever na waba - Código: {}", http_code);
                        let mut json = json;
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                        }
                        (StatusT::ERR, json)
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta de inscrição: {}", e);
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
    info!("=== SUBSCRIBE TO WABA (CLOUD) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

async fn _get_phone_number_id(waba_id: &str, access_token: &str, cloud_version: &f32) -> Status {
    let start_time = Instant::now();
    info!("=== GET PHONE NUMBER ID (CLOUD) START ===");
    debug!("Waba ID: {}", waba_id);
    let client = reqwest::Client::new();
    let req_url = format!("https://graph.facebook.com/{}/{}/phone_numbers", cloud_version, waba_id);
    debug!("URL da requisição: {}", req_url);
    let auth = format!("Bearer {}",access_token);
    let response = client.get(req_url)
        .header("Authorization", auth)
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
                        info!("Número pego com sucesso: {}", waba_id);
                        (StatusT::OK, json)
                    } else {
                        error!("Erro HTTP ao buscar phone number id - Código: {}", http_code);
                        let mut json = json;
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                        }
                        (StatusT::ERR, json)
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta de fetch: {}", e);
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
    info!("=== GET PHONE NUMBER ID (CLOUD) END - Duração: {}ms ===", duration);
    Status {
        status_code,
        json_status,
    }
}

pub async fn register_phone_number(phone_number_id: &str, access_token: &str, cloud_version: &f32) -> Status {
    info!("Registrando o número na WABA!");
    let client = reqwest::Client::new();
    let req_url = format!("https://graph.facebook.com/{}/{}/register", cloud_version, phone_number_id);
    debug!("URL da requisição: {}", req_url);

    let auth = format!("Bearer {}", access_token);
    let response = client.post(&req_url)
        .header("Authorization", auth)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .send()
        .await;

    let status_code;
    let mut json_status;
    let response_body;
    let http_ok;

    match response {
        Ok(resp) => {
            let http_status = resp.status();
            response_body = resp.text().await.unwrap_or_else(|_| "".to_string());
            http_ok = http_status.is_success();
            debug!("Resposta HTTP: {}", response_body);
            match serde_json::from_str::<serde_json::Value>(&response_body) {
                Ok(mut json) => {
                    if !http_ok {
                        error!("Erro HTTP ao registrar o número na WABA");
                        status_code = StatusT::ERR;
                        json_status = json.clone();
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                            json_status = json;
                        }
                    } else {
                        info!("Instância registrada com sucesso");
                        status_code = StatusT::OK;
                        json_status = json;
                    }
                },
                Err(e) => {
                    error!("Erro ao processar registrar o número na WABA: {}", e);
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

    Status {
        status_code,
        json_status,
    }
}

pub async fn register_number(waba_id: &str, access_token: &str, cloud_version: &f32) -> Status {
    info!("Iniciando registro do número. WABA ID: {}", waba_id);
    debug!("Tentando inscrever na WABA...");
    let response = _subscribe_to_waba(waba_id, access_token, cloud_version).await;
    if response.status_code == StatusT::ERR {
        error!("Falha ao inscrever na WABA: {}", response.json_status);
        return response;
    }
    debug!("Inscrição na WABA bem-sucedida. Obtendo ID do telefone...");
    let number_id_status = _get_phone_number_id(waba_id, access_token, cloud_version).await;
    if number_id_status.status_code == StatusT::ERR {
        error!("Falha ao obter ID do telefone: {}", number_id_status.json_status);
        return number_id_status;
    }
    debug!("ID do telefone obtido com sucesso: {}", number_id_status.json_status);
    if let Some(data) = number_id_status.json_status.get("data") {
        if let Some(array) = data.as_array() {
            if !array.is_empty() {
                if let Some(phone_id_val) = array[0].get("id") {
                    if let Some(phone_id) = phone_id_val.as_str() {
                        info!("ID do telefone encontrado: {}", phone_id);
                        debug!("Registrando o número com o ID: {}", phone_id);
                        let rgstr = register_phone_number(phone_id, access_token, cloud_version).await;
                        if rgstr.status_code == StatusT::OK {
                            info!("Número registrado com sucesso!");
                        } else {
                            error!("Falha ao registrar o número: {}", rgstr.json_status);
                        }
                        return rgstr;
                    }
                }
            }
        }
    }
    error!("Dados do ID do telefone não encontrados na resposta");
    Status {
        status_code: StatusT::ERR,
        json_status: json!({"error": "Couldn't find number_id data."}),
    }
}

pub async fn send_message_c(
    instance_id: &str,
    receiver: &str,
    body: &str,
    m_type: &MediaType,
    phone_number_id: &str,
    access_token: &str,
    cloud_version: &f32,
) -> Status {
    info!("Enviando mensagem com instância: {}", instance_id);
    let client = reqwest::Client::new();
    let req_url = format!("https://graph.facebook.com/{}/{}/messages", cloud_version, phone_number_id);
    let req_body = match m_type {
        MediaType::TEXT => json!({
            "messaging_product": "whatsapp",
            "recipient_type": "individual",
            "to": receiver,
            "type": "text",
            "text": {
                "preview_url": false,
                "body": body
            }
        }),
        MediaType::AUDIO => json!({
            "messaging_product": "whatsapp",
            "recipient_type": "individual",
            "to": receiver,
            "type": "audio",
            "audio": {
                "link": body
            }
        }),
        MediaType::IMAGE => json!({
            "messaging_product": "whatsapp",
            "recipient_type": "individual",
            "to": receiver,
            "type": "image",
            "image": {
                "link": body
            }
        }),
    };
    debug!("URL da requisição: {}", req_url);
    debug!("Corpo da requisição: {}", req_body);

    let auth = format!("Bearer {}", access_token);
    let response = client
        .post(&req_url)
        .header("Authorization", auth)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .json(&req_body)
        .send()
        .await;

    let status_code;
    let mut json_status;
    let response_body;
    let http_ok;

    match response {
        Ok(resp) => {
            let http_status = resp.status();
            response_body = resp.text().await.unwrap_or_else(|_| "".to_string());
            http_ok = http_status.is_success();
            debug!("Resposta HTTP: {}", response_body);
            match serde_json::from_str::<serde_json::Value>(&response_body) {
                Ok(mut json) => {
                    if !http_ok {
                        error!("Erro HTTP ao criar instância");
                        status_code = StatusT::ERR;
                        json_status = json.clone();
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                            json_status = json;
                        }
                    } else {
                        info!("Instância criada com sucesso");
                        status_code = StatusT::OK;
                        json_status = json;
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta da criação: {}", e);
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

    Status {
        status_code,
        json_status,
    }
}

pub async fn send_template(
    instance_id: &str,
    receiver: &str,
    body: &str,
    m_type: &MediaType,
    phone_number_id: &str,
    access_token: &str,
    vars: &[FbVars],
    template_name: &str,
    cloud_version: &f32,
) -> Status {
    info!("Enviando template com instância: {}", instance_id);
    let client = reqwest::Client::new();
    let req_url = format!("https://graph.facebook.com/{}/{}/messages", cloud_version, phone_number_id);
    let mut template = json!({
        "messaging_product": "whatsapp",
        "recipient_type": "individual",
        "to": receiver,
        "type": "template",
        "template": {
            "name": template_name,
            "language": {"code": "pt_BR"},
            "components": []
        }
    });
    // Header for image
    if *m_type == MediaType::IMAGE {
        let header_component = json!({
            "type": "header",
            "parameters": [
                {
                    "type": "image",
                    "image": {"link": body}
                }
            ]
        });
        template["template"]["components"].as_array_mut().unwrap().push(header_component);
    }
    // Body variables
    if !vars.is_empty() {
        let mut body_parameters = Vec::new();
        for var in vars {
            use crate::constants::consts::VariableT;
            match var.var {
                VariableT::TEXT => {
                    body_parameters.push(json!({
                        "type": "text",
                        "text": var.body
                    }));
                },
                VariableT::CURRENCY => {
                    let mut code = "BRL".to_string();
                    let mut amount_1000 = 0;
                    let mut fallback_value = "R$ 0.00".to_string();
                    if let Some(colon_pos) = var.body.find(':') {
                        code = var.body[..colon_pos].to_string();
                        if let Ok(amount) = var.body[colon_pos+1..].parse::<f64>() {
                            amount_1000 = (amount * 1000.0) as i64;
                            fallback_value = format!("${}", &var.body[colon_pos+1..]);
                        }
                    }
                    body_parameters.push(json!({
                        "type": "currency",
                        "currency": {
                            "fallback_value": fallback_value,
                            "code": code,
                            "amount_1000": amount_1000
                        }
                    }));
                },
                VariableT::DATETIME => {
                    let fallback_value: String = var.body.clone();
                    let (mut year, mut month, mut day, mut hour, mut minute) = (0, 0, 0, 0, 0);
                    if var.body.len() >= 10 {
                        year = var.body[0..4].parse().unwrap_or(0);
                        month = var.body[5..7].parse().unwrap_or(0);
                        day = var.body[8..10].parse().unwrap_or(0);
                        if var.body.len() >= 16 {
                            hour = var.body[11..13].parse().unwrap_or(0);
                            minute = var.body[14..16].parse().unwrap_or(0);
                        }
                    }
                    body_parameters.push(json!({
                        "type": "date_time",
                        "date_time": {
                            "fallback_value": fallback_value,
                            "year": year,
                            "month": month,
                            "day_of_month": day,
                            "hour": hour,
                            "minute": minute,
                            "calendar": "GREGORIAN"
                        }
                    }));
                },
            }
        }
        let body_component = json!({
            "type": "body",
            "parameters": body_parameters
        });
        template["template"]["components"].as_array_mut().unwrap().push(body_component);
    }
    debug!("URL da requisição: {}", req_url);
    debug!("Corpo da requisição: {}", template);
    let auth = format!("Bearer {}", access_token);
    let response = client
        .post(&req_url)
        .header("Authorization", auth)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .json(&template)
        .send()
        .await;
    let status_code;
    let mut json_status;
    let response_body;
    let http_ok;
    match response {
        Ok(resp) => {
            let http_status = resp.status();
            response_body = resp.text().await.unwrap_or_else(|_| "".to_string());
            http_ok = http_status.is_success();
            debug!("Resposta HTTP: {}", response_body);
            match serde_json::from_str::<serde_json::Value>(&response_body) {
                Ok(mut json) => {
                    if !http_ok {
                        error!("Erro HTTP ao enviar template");
                        status_code = StatusT::ERR;
                        json_status = json.clone();
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                            json_status = json;
                        }
                    } else {
                        info!("Template enviado com sucesso");
                        status_code = StatusT::OK;
                        json_status = json;
                    }
                },
                Err(e) => {
                    error!("Erro ao processar resposta do envio de template: {}", e);
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
    Status {
        status_code,
        json_status,
    }
}

pub async fn register_template(
    access_token: &str,
    template_: &Template,
    inst_id: &str,
    waba_id: &str,
    cloud_version: &f32,
) -> Status {
    info!("Registrando o template na instância: {}", inst_id);
    let client = reqwest::Client::new();
    let req_url = format!("https://graph.facebook.com/{}/{}/message_templates", cloud_version, waba_id);
    let mut request_json = json!({
        "name": template_.name,
        "language": "pt_BR"
    });
    let category = match template_.t_type {
        TemplateType::AUTH => "AUTHENTICATION",
        TemplateType::MARKETING => "MARKETING",
        TemplateType::UTILITY => "UTILITY",
    };
    request_json["category"] = json!(category);
    let mut components = Vec::new();
    if template_.header.text != "" || template_.header.examples.len() > 0 {
        let mut header_component = json!({"type": "HEADER"});
        if template_.header.text != "" {
            header_component["format"] = json!("TEXT");
            header_component["text"] = json!(template_.header.text);
        } else {
            header_component["format"] = json!("IMAGE");
        }
        components.push(header_component);
    }
    if template_.body.text != "" {
        let mut body_component = json!({
            "type": "BODY",
            "text": template_.body.text
        });
        if !template_.body.examples.is_empty() {
            let example_array = json!([template_.body.examples.clone()]);
            body_component["example"] = json!({"body_text": example_array});
        }
        components.push(body_component);
    }
    if template_.footer != "" {
        let footer_component = json!({
            "type": "FOOTER",
            "text": template_.footer
        });
        components.push(footer_component);
    }
    if !template_.buttons.is_empty() {
        let mut buttons_component = json!({
            "type": "BUTTONS",
            "buttons": []
        });
        for button in &template_.buttons {
            buttons_component["buttons"].as_array_mut().unwrap().push(json!({
                "type": button.b_type,
                "text": button.text
            }));
        }
        components.push(buttons_component);
    }
    request_json["components"] = json!(components);
    debug!("URL da requisição: {}", req_url);
    debug!("Corpo da requisição: {}", request_json);
    let auth = format!("Bearer {}", access_token);
    let response = client
        .post(&req_url)
        .header("Authorization", auth)
        .header("Content-Type", "application/json")
        .header("accept", "application/json")
        .json(&request_json)
        .send()
        .await;
    let status_code;
    let mut json_status;
    let response_body;
    let http_ok;
    match response {
        Ok(resp) => {
            let http_status = resp.status();
            response_body = resp.text().await.unwrap_or_else(|_| "".to_string());
            http_ok = http_status.is_success();
            debug!("Resposta HTTP: {}", response_body);
            match serde_json::from_str::<serde_json::Value>(&response_body) {
                Ok(mut json) => {
                    if !http_ok {
                        error!("Erro HTTP ao registrar o template");
                        status_code = StatusT::ERR;
                        json_status = json.clone();
                        if !json.get("error").is_some() {
                            json["error"] = serde_json::json!("Servidor retornou código de erro HTTP");
                            json_status = json;
                        }
                    } else {
                        info!("Instância registrada com sucesso");
                        status_code = StatusT::OK;
                        json_status = json;
                    }
                },
                Err(e) => {
                    error!("Erro ao processar registrar o template: {}", e);
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
    Status {
        status_code,
        json_status,
    }
}

