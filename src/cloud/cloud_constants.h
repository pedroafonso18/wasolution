#pragma once

/* --- CHANGEABLE DATA BELOW --- */

#define CLOUD_VERSION 22.0

/* --- END OF CHANGEABLE DATA --- */

enum class TemplateType{
    AUTH,
    MARKETING,
    UTILITY
};

enum class HEADER_T {
    TEXT,
    IMAGE,
    LOCATION,
    DOCUMENT
};

enum class VARIABLE_T {
    TEXT,
    CURRENCY,
    DATE_TIME
};

typedef struct {
    HEADER_T header_type;
    std::string header_content;
} HEADER;

typedef struct {
    std::string type;
    std::string text;
} BUTTON;

typedef struct {
    std::string text;
    std::vector<std::string> examples;
} BODY;

typedef struct {
    BODY body;
    std::string FOOTER;
    std::vector<BUTTON> BUTTONS;
    HEADER header;
    TemplateType type;
    std::string name;
} Template;

typedef struct {
    VARIABLE_T var;
    std:: string body;
} FB_VARS;
