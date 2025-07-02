use crate::constants::consts::{ApiType, Status, StatusT};
use tokio_postgres::{Client, Error};
use tokio_postgres::types::ToSql;
use serde_json::json;
use log;

pub async fn insert_instance(
    client: &Client,
    inst_id: &str,
    inst_name: &str,
    inst_type: &ApiType,
    webhook_url: Option<String>,
    waba_id: Option<String>,
    token: Option<String>,
    phone_number_id: Option<String>
) -> Status {
    let mut columns = vec!["instance_id", "name", "instance_type", "is_active"];

    let instance_type: String;
    if *inst_type == ApiType::EVOLUTION {
        instance_type = "EVOLUTION".to_string();
    } else if *inst_type == ApiType::WUZAPI {
        instance_type = "WUZAPI".to_string();
    } else if *inst_type == ApiType::CLOUD {
        instance_type = "CLOUD".to_string();
    } else {
        instance_type = "UNKNOWN".to_string();
    }

    let mut values: Vec<&(dyn ToSql + Sync)> = vec![&inst_id, &inst_name, &instance_type, &true];

    if let Some(ref url) = webhook_url {
        columns.push("webhook_url");
        values.push(url);
    }
    if let Some(ref waba) = waba_id {
        columns.push("waba_id");
        values.push(waba);
    }
    if let Some(ref t) = token {
        columns.push("access_token");
        values.push(t);
    }
    if let Some(ref phone) = phone_number_id {
        columns.push("phone_number_id");
        values.push(phone);
    }

    let placeholders: Vec<String> = (1..=values.len()).map(|i| format!("${}", i)).collect();

    let sql = format!(
        "INSERT INTO instances ({}) VALUES ({}) RETURNING instance_id",
        columns.join(", "),
        placeholders.join(", ")
    );

    let result = client.query_one(&sql, &values).await;

    match result {
        Ok(_) => Status {
            status_code: StatusT::OK,
            json_status: json!({
                "code": "200",
                "status": "Instância inserida com sucesso!"
            }),
        },
        Err(e) => Status {
            status_code: StatusT::ERR,
            json_status: json!({
                "code": "500",
                "error": format!("Erro ao inserir instância: {}", e)
            }),
        },
    }
}

pub async fn insert_log(log_level: &str, log_text: &str, client: &Client) -> Status {
    let result = client.query_one("INSERT INTO logs (log_level, log_text) VALUES ($1, $2)", &[&log_text, &log_level]).await;

    match result {
        Ok(_) => Status {
            status_code: StatusT::OK,
            json_status: json!({
                "code": "200",
                "status": "Log inserido com sucesso!"
            }),
        },
        Err(e) => Status {
            status_code: StatusT::ERR,
            json_status: json!({
                "code": "500",
                "error": format!("Erro ao inserir log: {}", e)
            }),
        },
    }
}
