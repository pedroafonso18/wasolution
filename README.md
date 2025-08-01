# WASolution

## 📝 Descrição
WASolution é uma API wrapper que unifica as funcionalidades das APIs Evolution e Wuzapi do WhatsApp, fornecendo uma interface padronizada e simplificada para integração. O projeto visa facilitar o processo de unificação entre as duas APIs, oferecendo respostas e saídas padronizadas.

## 🚀 Funcionalidades
- ✅ Integração unificada com Evolution API e Wuzapi
- ✅ Interface padronizada para todas as operações
- ✅ Sistema de webhooks completo e funcional
- ✅ Suporte a múltiplas instâncias simultâneas
- ✅ Operações completas de banco de dados
- ✅ API RESTful completa
- ✅ Sistema de logging detalhado
- ✅ Suporte a diferentes tipos de mídia (texto, imagem, áudio)
- ✅ Gerenciamento completo de instâncias (criar, conectar, desconectar, excluir)
- ✅ Configuração dinâmica de webhooks

## 🛠️ Tecnologias Utilizadas
- C++17
- Boost.Beast (HTTP Server)
- Boost.Asio (Networking)
- nlohmann/json (JSON parsing)
- spdlog (Logging)
- Postgres (Banco de dados)
- libcurl (HTTP client)

## 📋 Pré-requisitos
- Compilador C++ compatível com C++17 ou superior
- Boost Library (versão 1.70 ou superior)
- CMake (versão 3.10 ou superior)
- libcurl
- Postgres

## 🔧 Instalação

### Instalação Tradicional
1. Clone o repositório:
```bash
git clone https://github.com/pedroafonso18/wasolution.git
cd wasolution
```

2. Configure o projeto:
```bash
mkdir build
cd build
cmake ..
```

3. Compile o projeto:
```bash
make
```

### 🐳 Deploy com Docker (Recomendado)
Para uma instalação mais simples e isolada, use Docker:

```bash
# Clone o repositório
git clone https://github.com/pedroafonso18/wasolution.git
cd wasolution

# Configure as variáveis de ambiente
cp .env.example .env
# Edite o arquivo .env com suas configurações

# Build e execute com Docker Compose
docker-compose up --build
```

📖 **Documentação completa do Docker**: [Guia de Deploy com Docker](docs/docker-deploy.md)

## 🚀 Como Usar
1. Configure as variáveis de ambiente necessárias (ver seção de configuração)
2. Inicie o servidor:
```bash
./wasolution
```

3. A API estará disponível em `http://0.0.0.0:8080`

## ⚙️ Configuração

O sistema utiliza variáveis de ambiente para configuração. Crie um arquivo `.env` na raiz do projeto:

```env
# URLs das APIs
EVO_URL=http://localhost:8080
WUZ_URL=http://localhost:3000

# Tokens de autenticação
EVO_TOKEN=seu_token_evolution
WUZ_ADMIN_TOKEN=seu_token_wuzapi

# Configuração do banco de dados
DB_URL=postgres:///wasolution.db

# Configuração do servidor
IP_ADDRESS=0.0.0.0
DEFAULT_WEBHOOK=https://seu-servidor.com/webhook
```

## 📚 Documentação da API
[DOCUMENTAÇÃO COMPLETA](docs/api.md)

### Endpoints Principais

#### Gerenciamento de Instâncias
- `POST /createInstance`: Criar nova instância
- `POST /connectInstance`: Conectar instância existente
- `DELETE /deleteInstance`: Excluir instância
- `DELETE /logoutInstance`: Desconectar instância

#### Mensagens
- `POST /sendMessage`: Enviar mensagem (texto, imagem, áudio)

#### Webhooks
- `POST /setWebhook`: Configurar webhook para instância
- `POST /webhook`: Receber notificações de webhook

### Exemplo de Uso

#### Criar uma instância:
```bash
curl -X POST http://localhost:8080/createInstance \
  -H "Content-Type: application/json" \
  -d '{
    "instance_id": "minha_instancia",
    "instance_name": "Minha Empresa",
    "api_type": "EVOLUTION",
    "webhook_url": "https://meu-servidor.com/webhook"
  }'
```

#### Enviar uma mensagem:
```bash
curl -X POST http://localhost:8080/sendMessage \
  -H "Content-Type: application/json" \
  -d '{
    "instance_id": "minha_instancia",
    "number": "5511999999999",
    "body": "Olá! Esta é uma mensagem de teste.",
    "type": "TEXT"
  }'
```

## 🗄️ Banco de Dados

O sistema utiliza Postgres para armazenar informações das instâncias. O banco é criado automaticamente na primeira execução.

### Estrutura da Tabela de Instâncias:
- `instance_id`: Identificador único da instância
- `instance_name`: Nome da instância
- `instance_type`: Tipo de API (EVOLUTION ou WUZAPI)
- `webhook_url`: URL do webhook configurada

## 📊 Logs e Monitoramento

O sistema gera logs detalhados para facilitar o debugging e monitoramento:

- **Logs de API**: `logs/api.log`
- **Logs de operações**: Logs de criação, conexão e envio de mensagens
- **Logs de erros**: Detalhes de erros e exceções
- **Logs de webhooks**: Processamento de webhooks recebidos

## 🔒 Segurança

- Validação de parâmetros de entrada
- Tratamento de exceções robusto
- Logs de auditoria para operações críticas
- Suporte a proxy para conexões seguras

## 🤝 Contribuindo
Contribuições são sempre bem-vindas! Por favor, leia as diretrizes de contribuição antes de submeter um pull request.

1. Faça um Fork do projeto
2. Crie uma Branch para sua Feature (`git checkout -b feature/AmazingFeature`)
3. Faça o Commit das suas mudanças (`git commit -m 'Add some AmazingFeature'`)
4. Faça o Push para a Branch (`git push origin feature/AmazingFeature`)
5. Abra um Pull Request

## 📄 Licença
Este projeto está sob a licença [MIT](LICENSE). Veja o arquivo `LICENSE` para mais detalhes.

## 📞 Suporte
Para suporte, envie um email para [pedroafonsoprogramador@gmail.com] ou abra uma issue no GitHub.

## 🔮 Roadmap
- [X] Implementação completa do sistema de webhooks
- [X] Suporte a múltiplas instâncias
- [X] Documentação completa da API
- [X] Sistema de logging detalhado
- [X] Suporte a diferentes tipos de mídia
- [ ] Testes automatizados
- [ ] Interface de administração web
- [ ] Sistema de cache
- [ ] Monitoramento e métricas
- [ ] Suporte a migrações de banco de dados
- [ ] API de status e health check
- [ ] Rate limiting
- [X] Autenticação e autorização

## 🙏 Agradecimentos
- Evolution API
- Wuzapi
- Todos os contribuidores do projeto
- Comunidade C++ e Boost
