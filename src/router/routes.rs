use axum::{Router, routing::get};

pub async fn routing() -> Router {
    return Router::new()
        .route("/", get(|| async { "Hello World! "}));
}