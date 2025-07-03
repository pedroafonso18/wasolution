use crate::constants::consts::{MediaType, Status, StatusT};
use crate::database::fetch::fetch_instance;
use crate::api::evolution::send_message_e;
use crate::api::wuzapi::send_message_w;
use crate::api::cloud::send_message_c;
use tokio_postgres::Client;
use log::{info, debug, error};

pub async fn send_message(
    client: &Client,
    instance_id: &str,
    number: &str,
    body: &str,
    m_type: &MediaType,
) -> Status {
    info!("Iniciando envio de mensagem para instância: {}", instance_id);
    // Fetch instance from DB
    let inst = match fetch_instance(client, instance_id).await {
        Ok(Some(i)) => i,
        Ok(None) => {
            error!("Instância não encontrada: {}", instance_id);
            return Status {
                status_code: StatusT::ERR,
                json_status: serde_json::json!({"error": "Couldn't find any connections with this name."}),
            };
        },
        Err(e) => {
            error!("Erro ao buscar instância no banco de dados: {}", e);
            return Status {
                status_code: StatusT::ERR,
                json_status: serde_json::json!({"error": format!("Erro ao buscar instância: {}", e)}),
            };
        }
    };
    debug!("Instância encontrada: {}", inst.instance_name);
    let mut snd;
    if inst.instance_type == "EVOLUTION" {
        info!("Enviando mensagem via Evolution API");
        // You may need to fetch env tokens/urls from config here
        // Example: let env = get_env();
        // snd = send_message_e(number, &env.evo_token, &env.evo_url, m_type, body, &inst.instance_name).await;
        error!("Evolution send_message_e call not implemented in handler");
        return Status {
            status_code: StatusT::ERR,
            json_status: serde_json::json!({"error": "Evolution send_message_e call not implemented in handler"}),
        };
    } else if inst.instance_type == "WUZAPI" {
        info!("Enviando mensagem via WuzAPI");
        // Example: let env = get_env();
        // snd = send_message_w(number, &inst.instance_id, &env.wuz_url, m_type, body).await;
        error!("WuzAPI send_message_w call not implemented in handler");
        return Status {
            status_code: StatusT::ERR,
            json_status: serde_json::json!({"error": "WuzAPI send_message_w call not implemented in handler"}),
        };
    } else if inst.instance_type == "CLOUD" {
        info!("Enviando mensagem via CLOUD");
        if let (Some(phone_number_id), Some(access_token)) = (inst.phone_number_id.as_ref(), inst.access_token.as_ref()) {
            snd = send_message(
                &inst.instance_id,
                number,
                body,
                m_type,
                phone_number_id,
                access_token,
                &1.0 // Replace with actual cloud version if needed
            ).await;
        } else {
            error!("Dados de CLOUD ausentes para a instância: {}", instance_id);
            return Status {
                status_code: StatusT::ERR,
                json_status: serde_json::json!({"error": "Missing phone_number_id or access_token for CLOUD instance"}),
            };
        }
        if snd.status_code == StatusT::ERR {
            error!("Erro ao enviar mensagem via CLOUD: {}", snd.json_status);
            return snd;
        } else {
            info!("Mensagem enviada com sucesso via CLOUD");
            return Status {
                status_code: StatusT::OK,
                json_status: snd.json_status,
            };
        }
    }
    error!("Tipo de API desconhecido para instância: {}", instance_id);
    Status {
        status_code: StatusT::ERR,
        json_status: serde_json::json!({"error": "Unknown API, please choose between EVOLUTION and WUZAPI"}),
    }
} 