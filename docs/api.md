# WASolution API Documentation

Esta documentação descreve os endpoints disponíveis na API WASolution para gerenciar instâncias de WhatsApp e enviar mensagens.

## Base URL

```
http://{IP}:{PORT}
```

Onde:
- `IP` é o endereço IP configurado (padrão: 0.0.0.0)
- `PORT` é a porta configurada (padrão: 8080)

## Autenticação

Todas as requisições para a API devem incluir um token de autenticação no cabeçalho HTTP:

```
Authorization: Bearer {TOKEN}
```

Onde `{TOKEN}` é o token de acesso configurado no sistema. Requisições sem o token correto receberão um erro 401 (Unauthorized).

## Formatos de Resposta

Todas as respostas são retornadas no formato JSON com os seguintes campos comuns:

- `status_code`: Código numérico que indica o resultado da operação (0 para sucesso, números diferentes para erro)
- `status_string`: Descrição textual do resultado da operação

Em caso de erro, a resposta pode incluir um campo adicional `error` com detalhes sobre o problema.

## Endpoints

### 1. Criar Instância

Cria uma nova instância do WhatsApp para gerenciamento.

**Endpoint:** `/createInstance`  
**Método:** POST  
**Content-Type:** application/json

**Parâmetros de Requisição:**

| Campo | Tipo | Obrigatório | Descrição |
|-------|------|-------------|-----------|
| instance_id | String | Sim | Identificador único para a instância |
| instance_name | String | Sim | Nome para exibição da instância |
| api_type | String | Sim | Tipo de API a ser utilizada ("EVOLUTION" ou "WUZAPI") |
| webhook_url | String | Não | URL para receber notificações de webhook (opcional) |
| proxy_url | String | Não | URL do proxy para conexão (opcional) |

**Exemplo de Requisição:**
```json
{
    "instance_id": "instance001",
    "instance_name": "Cliente A",
    "api_type": "EVOLUTION",
    "webhook_url": "https://seu-servidor.com/webhook",
    "proxy_url": ""
}
```

**Exemplo de Resposta de Sucesso:**
```json
{
    "status_code": 0,
    "status_string": {
        "message": "Instance created successfully!",
        "api_response": "Resposta da API específica"
    }
}
```

**Exemplo de Resposta de Erro:**
```json
{
    "status_code": 1,
    "status_string": {
        "error": "Erro ao criar instância"
    }
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

### 2. Conectar Instância

Inicia a conexão de uma instância existente.

**Endpoint:** `/connectInstance`  
**Método:** POST  
**Content-Type:** application/json

**Parâmetros de Requisição:**

| Campo | Tipo | Obrigatório | Descrição |
|-------|------|-------------|-----------|
| instance_id | String | Sim | Identificador da instância a ser conectada |

**Exemplo de Requisição:**
```json
{
    "instance_id": "instance001"
}
```

**Exemplo de Resposta de Sucesso:**
```json
{
    "status_code": 0,
    "status_string": {
        "message": "Instance connected successfully!",
        "api_response": "Resposta da API específica"
    }
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

### 3. Enviar Mensagem

Envia uma mensagem para um contato específico.

**Endpoint:** `/sendMessage`  
**Método:** POST  
**Content-Type:** application/json

**Parâmetros de Requisição:**

| Campo | Tipo | Obrigatório | Descrição |
|-------|------|-------------|-----------|
| instance_id | String | Sim | Identificador da instância que enviará a mensagem |
| number | String | Sim | Número de telefone do destinatário (formato internacional) |
| body | String | Sim | Conteúdo da mensagem ou URL do arquivo de mídia |
| type | String | Não | Tipo de mídia ("TEXT", "IMAGE", "AUDIO"). Padrão: "TEXT" |

**Exemplo de Requisição (Texto):**
```json
{
    "instance_id": "instance001",
    "number": "5511999999999",
    "body": "Olá! Esta é uma mensagem de teste.",
    "type": "TEXT"
}
```

**Exemplo de Requisição (Imagem):**
```json
{
    "instance_id": "instance001",
    "number": "5511999999999",
    "body": "https://exemplo.com/imagem.jpg",
    "type": "IMAGE"
}
```

**Exemplo de Requisição (Áudio):**
```json
{
    "instance_id": "instance001",
    "number": "5511999999999",
    "body": "https://exemplo.com/audio.mp3",
    "type": "AUDIO"
}
```

**Exemplo de Resposta de Sucesso:**
```json
{
    "status_code": 0,
    "status_string": {
        "message": "Successfully sent the message!",
        "api_response": "Resposta da API específica"
    }
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

### 4. Excluir Instância

Remove uma instância existente.

**Endpoint:** `/deleteInstance`  
**Método:** DELETE  
**Content-Type:** application/json

**Parâmetros de Requisição:**

| Campo | Tipo | Obrigatório | Descrição |
|-------|------|-------------|-----------|
| instance_id | String | Sim | Identificador da instância a ser excluída |

**Exemplo de Requisição:**
```json
{
    "instance_id": "instance001"
}
```

**Exemplo de Resposta de Sucesso:**
```json
{
    "status_code": 0,
    "status_string": {
        "message": "Instance deleted successfully!",
        "api_response": "Resposta da API específica"
    }
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

### 5. Desconectar Instância

Desconecta uma instância sem excluí-la.

**Endpoint:** `/logoutInstance`  
**Método:** DELETE  
**Content-Type:** application/json

**Parâmetros de Requisição:**

| Campo | Tipo | Obrigatório | Descrição |
|-------|------|-------------|-----------|
| instance_id | String | Sim | Identificador da instância a ser desconectada |

**Exemplo de Requisição:**
```json
{
    "instance_id": "instance001"
}
```

**Exemplo de Resposta de Sucesso:**
```json
{
    "status_code": 0,
    "status_string": {
        "message": "Instance deleted successfully!",
        "api_response": "Resposta da API específica"
    }
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

### 6. Configurar Webhook

Configura ou atualiza a URL do webhook para uma instância existente.

**Endpoint:** `/setWebhook`  
**Método:** POST  
**Content-Type:** application/json

**Parâmetros de Requisição:**

| Campo | Tipo | Obrigatório | Descrição |
|-------|------|-------------|-----------|
| instance_id | String | Sim | Identificador da instância a ser configurada |
| webhook_url | String | Sim | Nova URL do webhook para receber notificações |

**Exemplo de Requisição:**
```json
{
    "instance_id": "instance001",
    "webhook_url": "https://seu-servidor.com/novo-webhook"
}
```

**Exemplo de Resposta de Sucesso:**
```json
{
    "status_code": 0,
    "status_string": {
        "message": "Instance deleted successfully!",
        "api_response": "Resposta da API específica"
    }
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

### 7. Webhook

Endpoint para processar notificações recebidas de uma instância.

**Endpoint:** `/webhook`  
**Método:** POST  
**Content-Type:** application/json

**Parâmetros de Requisição:**

O formato do webhook varia dependendo da API utilizada:

**Para Evolution API:**
```json
{
    "event": "message",
    "instance": "instance001",
    "data": {
        "instanceId": "instance001",
        "message": "Olá!",
        "from": "5511999999999",
        "timestamp": 1623456789
    }
}
```

**Para WuzAPI:**
```json
{
    "type": "message",
    "token": "instance001",
    "instance_name": "Cliente A",
    "data": {
        "message": "Olá!",
        "from": "5511999999999"
    }
}
```

**Exemplo de Resposta de Sucesso:**
```json
{
    "status_code": 0,
    "status_string": {
        "webhook": "Webhook sent successfully!",
        "api_response": "Resposta da API específica"
    }
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

### 8. Listar Instâncias

Retorna a lista de todas as instâncias cadastradas no sistema.

**Endpoint:** `/retrieveInstances`  
**Método:** GET  
**Content-Type:** application/json

**Sem parâmetros de requisição**

**Exemplo de Resposta de Sucesso:**
```json
{
  "status": "success",
  "count": 2,
  "instances": [
    {
      "instance_id": "instance001",
      "instance_name": "Cliente A",
      "instance_type": "EVOLUTION",
      "is_active": true,
      "webhook_url": "https://exemplo.com/webhook"
    },
    {
      "instance_id": "instance002",
      "instance_name": "Cliente B",
      "instance_type": "WUZAPI",
      "is_active": false,
      "webhook_url": "https://exemplo.com/webhook2"
    }
  ]
}
```

**Exemplo de Resposta de Erro:**
```json
{
  "status": "error",
  "message": "Failed to retrieve instances",
  "error": "Database connection error"
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 401 Unauthorized: Token de autenticação ausente ou inválido
- 500 Internal Server Error: Erro ao processar a requisição

## Configuração do Servidor

O servidor é configurado para executar no IP e porta definidos no código. Por padrão:

- IP: 0.0.0.0 (aceita conexões de qualquer endereço)
- Porta: 8080

## Tipos de Mídia Suportados

A API suporta os seguintes tipos de mídia:

- **TEXT**: Mensagens de texto simples
- **IMAGE**: Imagens (URLs de imagens)
- **AUDIO**: Arquivos de áudio (URLs de áudio)

## Tipos de API Suportados

- **EVOLUTION**: Evolution API
- **WUZAPI**: WuzAPI

## Tratamento de Erros

Todas as solicitações são validadas e os erros tratados adequadamente. Possíveis erros incluem:

- Parâmetros ausentes ou inválidos
- Instância não encontrada
- Falha na conexão com a API
- Erro ao processar a mensagem
- Tipo de API inválido
- Tipo de mídia inválido

## Tratamento Automático de Webhooks

O sistema inclui suporte completo para tratamento automático de webhooks, permitindo o encaminhamento de eventos recebidos das APIs para URLs configuradas pelo usuário. Ao criar uma instância com um `webhook_url`, os eventos de mensagens e status serão automaticamente encaminhados para esta URL.

O formato dos webhooks encaminhados segue o padrão da API utilizada (EVOLUTION ou WUZAPI), permitindo uma integração transparente com sistemas existentes.

## Logs e Monitoramento

O sistema utiliza logging detalhado para monitoramento e debugging:

- Logs de requisições recebidas
- Logs de operações de instâncias
- Logs de envio de mensagens
- Logs de erros e exceções
- Logs de webhooks processados

Os logs são salvos em arquivos separados para facilitar o troubleshooting e monitoramento do sistema.
