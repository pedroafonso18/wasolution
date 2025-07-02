use serde_json::Value;
use std::fmt;


#[derive(PartialEq)]
pub enum MediaType {
    IMAGE,
    AUDIO,
    TEXT
}

#[derive(PartialEq)]
pub enum ApiType {
    EVOLUTION,
    WUZAPI,
    CLOUD
}


#[derive(Debug, PartialEq)]
pub enum StatusT {
    OK,
    ERR
}

#[derive(Debug, PartialEq)]
pub struct Status {
    pub status_code: StatusT,
    pub json_status: Value,
}

impl std::fmt::Display for Status {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Status: {:?}, json_status: {}", self.status_code, self.json_status)
    }
}

#[derive(PartialEq)]
pub enum TemplateType {
    AUTH,
    MARKETING,
    UTILITY
}

#[derive(PartialEq)]
pub enum HeaderT {
    TEXT,
    IMAGE,
    LOCATION,
    DOCUMENT
}

#[derive(PartialEq)]
pub enum VariableT {
    TEXT,
    CURRENCY,
    DATETIME
}


#[derive(PartialEq)]
pub struct Header {
    pub text: String,
    pub examples: Vec<String>
}

#[derive(PartialEq)]
pub struct Button {
    pub b_type: String,
    pub text: String
}

#[derive(PartialEq)]
pub struct Body {
    pub text: String,
    pub examples: Vec<String>
}

#[derive(PartialEq)]
pub struct Template {
    pub body: Body,
    pub footer: String,
    pub buttons: Vec<Button>,
    pub header: Header,
    pub t_type: TemplateType,
    pub name: String
}

#[derive(PartialEq)]
pub struct FbVars {
    pub var: VariableT,
    pub body: String
}

impl std::error::Error for Status {}
