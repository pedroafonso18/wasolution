use tokio_postgres::{Client, Error};
use log;

pub struct Instance {
    pub instance_id: String,
    pub instance_name: String,
    pub instance_type: String,
    pub is_active: bool,
    pub webhook_url: Option<String>,
    pub waba_id: Option<String>,
    pub access_token: Option<String>,
    pub phone_number_id: Option<String>
}

pub async fn fetch_instance(client: &Client, instance_id: &str) -> Result<Option<Instance>, Error> {
    let row = client.query("SELECT instance_id, name, instance_type, is_active, webhook_url, waba_id, access_token FROM instances WHERE instance_id = $1 LIMIT 1", &[&instance_id]).await?;
    if row.is_empty() {
        log::error!("No instance on the database matches that id!");
        return Ok(None);
    }
    let row = &row[0];
    Ok(Some(Instance {
        instance_id: row.get("instance_id"),
        instance_name: row.get("name"),
        instance_type: row.get("instance_type"),
        is_active: row.get("is_active"),
        webhook_url: row.get("webhook_url"),
        waba_id: row.get("waba_id"),
        access_token: row.get("access_token"),
        phone_number_id: row.get("phone_number_id")
    }))
}

pub async fn get_qr_code(client: &Client, token: &str) -> Result<String, Error> {
    let result = client.query("SELECT qrcode FROM users WHERE id = $1 OR token = $1 LIMIT 1", &[&token]).await?;
    if result.is_empty() {
        return Ok(String::new());
    }
    let row = &result[0];
    Ok(row.get("qrcode"))
}

pub async fn retrieve_instances(client: &Client) -> Result<Vec<Instance>, Error> {
    let mut inst_vec: Vec<Instance> = Vec::new();
    let rows = client.query("SELECT instance_id, name, instance_type, is_active, webhook_url, waba_id, access_token, phone_number_id FROM instances", &[]).await?;
    if rows.is_empty() {
        log::error!("Erro: Nenhuma instância no banco de dados.");
        return Ok(inst_vec);
    }
    for row in rows {
        let instance = Instance {
            instance_id: row.get("instance_id"),
            instance_name: row.get("name"),
            instance_type: row.get("instance_type"),
            is_active: row.get("is_active"),
            webhook_url: row.get("webhook_url"),
            waba_id: row.get("waba_id"),
            access_token: row.get("access_token"),
            phone_number_id: row.get("phone_number_id"),
        };
        inst_vec.push(instance);
    }
    Ok(inst_vec)
}