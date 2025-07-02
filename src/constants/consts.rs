use serde_json::Value;
use std::fmt;

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

#[derive(Debug)]
pub enum StatusT {
    OK,
    ERR
}

#[derive(Debug)]
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


impl PartialEq for MediaType {
    fn eq(&self, other: &Self) -> bool {
        matches!(
            (self, other),
            (MediaType::IMAGE, MediaType::IMAGE)
                | (MediaType::AUDIO, MediaType::AUDIO)
                | (MediaType::TEXT, MediaType::TEXT)
        )
    }
}

impl PartialEq for ApiType {
    fn eq(&self, other: &Self) -> bool {
        matches!(
            (self, other),
            (ApiType::EVOLUTION, ApiType::EVOLUTION)
                | (ApiType::WUZAPI, ApiType::WUZAPI)
                | (ApiType::CLOUD, ApiType::CLOUD)
        )
    }
}

impl PartialEq for StatusT {
    fn eq(&self, other: &Self) -> bool {
        matches!(
            (self, other),
            (StatusT::OK, StatusT::OK)
                | (StatusT::ERR, StatusT::ERR)
        )
    }
}

impl PartialEq for TemplateType {
    fn eq(&self, other: &Self) -> bool {
        matches!(
            (self, other),
            (TemplateType::AUTH, TemplateType::AUTH)
                | (TemplateType::MARKETING, TemplateType::MARKETING)
                | (TemplateType::UTILITY, TemplateType::UTILITY)
        )
    }
}

impl PartialEq for HeaderT {
    fn eq(&self, other: &Self) -> bool {
        matches!(
            (self, other),
            (HeaderT::TEXT, HeaderT::TEXT)
                | (HeaderT::IMAGE, HeaderT::IMAGE)
                | (HeaderT::LOCATION, HeaderT::LOCATION)
                | (HeaderT::DOCUMENT, HeaderT::DOCUMENT)
        )
    }
}

impl PartialEq for VariableT {
    fn eq(&self, other: &Self) -> bool {
        matches!(
            (self, other),
            (VariableT::TEXT, VariableT::TEXT)
                | (VariableT::CURRENCY, VariableT::CURRENCY)
                | (VariableT::DATETIME, VariableT::DATETIME)
        )
    }
}

impl std::fmt::Display for Status {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Status: {:?}", self.status_code)
    }
}

impl std::error::Error for Status {}
