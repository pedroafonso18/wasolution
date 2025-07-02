use tokio_postgres::{Client};
use serde_json::json;
use crate::constants::consts::{Status, StatusT};

pub async fn delete_instance(instance_id: &str, client: &Client) -> Status {
    let result = client.query_one("DELETE FROM instances WHERE instance_id = $1", &[&instance_id]).await;

    match result {
        Ok(_) => Status {
            status_code: StatusT::OK,
            json_status: json!({
                "code": "200",
                "status": "Instância deletada com sucesso!"
            }),
        },
        Err(e) => Status {
            status_code: StatusT::ERR,
            json_status: json!({
                "code": "500",
                "error": format!("Erro ao deletar instância: {}", e)
            }),
        },
    }
}
