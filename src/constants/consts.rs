use serde_json::Value;

pub enum MediaType {
    IMAGE,
    AUDIO,
    TEXT
}

pub enum ApiType {
    EVOLUTION,
    WUZAPI,
    CLOUD
}

pub enum StatusT {
    OK,
    ERR
}

pub struct Status {
    pub status_code: StatusT,
    pub json_status: Value
}