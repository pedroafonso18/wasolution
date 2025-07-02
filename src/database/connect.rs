use tokio_postgres::{Client, Error};

pub async fn connect_db(db_url: &str) -> Result<Client, Error> {
    let (client, connection) = tokio_postgres::connect(db_url, tokio_postgres::NoTls).await?;
    tokio::spawn(
        async move {
            if let Err(e) = connection.await {
                eprintln!("Error on connecting to db: {}",e);
            }
        }
    );
    Ok(client)
}