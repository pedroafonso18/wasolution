mod router;
mod config;
mod logger;
mod constants;
mod database;
use axum;

#[tokio::main]
pub async fn main() {
    let env_vars = config::config::load_config();

    if let Err(e) = logger::logger::init_logger(&env_vars.log_level) {
        eprintln!("Couldn't setup logger: {}",e);
    }

    let app = router::routes::routing().await;

    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
    axum::serve(listener, app).await.unwrap();
}