use std::vec;

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

pub enum TemplateType {
    AUTH,
    MARKETING,
    UTILITY
}

pub enum HeaderT {
    TEXT,
    IMAGE,
    LOCATION,
    DOCUMENT
}

pub enum VariableT {
    TEXT,
    CURRENCY,
    DATETIME
}

pub struct Header {
    pub text: String,
    pub examples: Vec<String>
}

pub struct Button {
    pub b_type: String,
    pub text: String
}

pub struct Body {
    pub text: String,
    pub examples: Vec<String>
}

pub struct Template {
    pub body: Body,
    pub footer: String,
    pub buttons: Vec<Button>,
    pub header: Header,
    pub t_type: TemplateType,
    pub name: String
}

pub struct FbVars {
    pub var: VariableT,
    pub body: String
}