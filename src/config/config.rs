use std::error::Error;
use dotenvy::dotenv;
use std::env;

pub struct Env {
    pub evo_url: String,
    pub evo_token: String,
    pub wuz_url: String,
    pub db_url: String,
    pub db_url_wuz: String,
    pub default_webhook: String,
    pub wuz_admin_token: String,
    pub log_level: String,
    pub ip: String,
    pub port: i32,
    pub token: String,
    pub cloud_version: f32,
}

pub fn load_config() -> Env {
    if let Err(e) = dotenv() {
        eprintln!("Warning: .env doesn't exist. {} ", e);
    }
    let evo_url = env::var("EVO_URL").expect("EVO_URL not found in the .env");
    let evo_token = env::var("EVO_TOKEN").expect("EVO_TOKEN not found in the .env");
    let wuz_url = env::var("WUZ_URL").expect("WUZ_URL not found in the .env");
    let db_url = env::var("DB_URL").expect("DB_URL not found in the .env");
    let db_url_wuz = env::var("DB_URL_WUZ").expect("DB_URL_WUZ not found in the .env");
    let default_webhook = env::var("DEFAULT_WEBHOOK").expect("DEFAULT_WEBHOOK not found in the .env");
    let wuz_admin_token = env::var("WUZ_ADMIN_TOKEN").expect("WUZ_ADMIN_TOKEN not found in the .env");
    let log_level = env::var("LOG_LEVEL").unwrap_or("debug".to_string());
    let ip = env::var("IP_ADDRESS").unwrap_or("0.0.0.0".to_string());
    let port = env::var("PORT")
        .ok()
        .and_then(|v| v.parse::<i32>().ok())
        .unwrap_or(8080);
    let token = env::var("TOKEN").unwrap_or("ABCD1234".to_string()); // PLEASE CHANGE THIS.
    let cloud_version = env::var("CLOUD_VERSION")
        .ok()
        .and_then(|v| v.parse::<f32>().ok())
        .unwrap_or(22.0);

    Env{
        evo_url,
        evo_token,
        wuz_url,
        db_url,
        db_url_wuz,
        default_webhook,
        wuz_admin_token,
        log_level,
        ip,
        port,
        token,
        cloud_version
    }
}

