# WASolution API Documentation

Esta documentação descreve os endpoints disponíveis na API WASolution para gerenciar instâncias de WhatsApp e enviar mensagens.

## Base URL

```
http://{IP}:{PORT}
```

Onde:
- `IP` é o endereço IP configurado (padrão: 192.168.0.180)
- `PORT` é a porta configurada (padrão: 8080)

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
    "status_string": "Instância criada com sucesso"
}
```

**Exemplo de Resposta de Erro:**
```json
{
    "status_code": 1,
    "status_string": "Erro ao criar instância"
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
    "status_string": "Instância conectada com sucesso"
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

**Exemplo de Resposta de Sucesso:**
```json
{
    "status_code": 0,
    "status_string": "Mensagem enviada com sucesso"
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

### 4. Excluir Instância

Remove uma instância existente.

**Endpoint:** `/deleteInstance`  
**Método:** POST  
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
    "status_string": "Instância excluída com sucesso"
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

### 5. Webhook

Endpoint para processar notificações recebidas de uma instância.

**Endpoint:** `/webhook`  
**Método:** POST  
**Content-Type:** application/json

**Parâmetros de Requisição:**

| Campo | Tipo | Obrigatório | Descrição |
|-------|------|-------------|-----------|
| instance_id | String | Sim | Identificador da instância que está gerando o evento |
| ... | ... | ... | Outros campos dependem do tipo de evento |

**Exemplo de Requisição:**
```json
{
    "instance_id": "instance001",
    "event": "message",
    "data": {
        "message": "Olá!",
        "from": "5511999999999",
        "timestamp": 1623456789
    }
}
```

**Exemplo de Resposta de Sucesso:**
```json
{
    "status_code": 0,
    "status_string": "Webhook processado com sucesso"
}
```

**Códigos de Status HTTP:**
- 200 OK: Requisição processada com sucesso
- 400 Bad Request: Parâmetros inválidos ou ausentes
- 500 Internal Server Error: Erro ao processar a requisição

## Configuração do Servidor

O servidor é configurado para executar no IP e porta definidos no código. Por padrão:

- IP: 192.168.0.180
- Porta: 8080

## Tratamento de Erros

Todas as solicitações são validadas e os erros tratados adequadamente. Possíveis erros incluem:

- Parâmetros ausentes ou inválidos
- Instância não encontrada
- Falha na conexão com a API
- Erro ao processar a mensagem

Em caso de erro, a resposta incluirá um código de status HTTP apropriado e um corpo JSON com detalhes do erro.
