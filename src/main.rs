mod router;
use axum;

#[tokio::main]
pub async fn main() {
    let app = router::routes::routing().await;

    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await.unwrap();
    axum::serve(listener, app).await.unwrap();
}