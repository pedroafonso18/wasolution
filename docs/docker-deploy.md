# Deploy da WASolution com Docker

Este guia explica como fazer o deploy da aplicação WASolution usando Docker e Docker Compose.

## 📋 Pré-requisitos

- Docker Engine 20.10 ou superior
- Docker Compose 2.0 ou superior
- Pelo menos 2GB de RAM disponível
- Pelo menos 5GB de espaço em disco

## 🚀 Deploy Rápido

### 1. Clone o Repositório

```bash
git clone https://github.com/pedroafonso18/wasolution.git
cd wasolution
```

### 2. Configure as Variáveis de Ambiente

Crie um arquivo `.env` na raiz do projeto com as seguintes variáveis:

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

### 3. Build e Inicie os Serviços

```bash
# Build e start em primeiro plano
docker-compose up --build

# Ou execute em background
docker-compose up -d --build
```

### 4. Verifique se Está Funcionando

A aplicação estará disponível em `http://localhost:8080`

## 🔧 Comandos Úteis

### Visualizar Logs

```bash
# Todos os serviços
docker-compose logs

# Serviço específico
docker-compose logs wasolution

# Acompanhar logs em tempo real
docker-compose logs -f wasolution
```

### Parar Serviços

```bash
# Parar serviços
docker-compose down

# Parar e remover volumes (CUIDADO: apaga todos os dados)
docker-compose down -v
```

### Rebuild após Mudanças

```bash
# Rebuild completo
docker-compose up --build

# Rebuild apenas da aplicação
docker-compose up --build wasolution
```

### Acessar Container

```bash
# Acessar container da aplicação
docker-compose exec wasolution bash

# Acessar logs em tempo real
docker-compose logs -f wasolution
```

## 🐛 Solução de Problemas

### Problemas de Build

**Erro: "Out of memory"**
```bash
# Aumente a memória do Docker ou limpe o cache
docker system prune -a
```

**Erro: "Package not found"**
```bash
# Verifique se todas as dependências estão instaladas
docker-compose down
docker-compose up --build
```

### Problemas de Conexão

**Aplicação não responde na porta 8080**
```bash
# Verifique se o container está rodando
docker-compose ps

# Verifique os logs
docker-compose logs wasolution

# Verifique se a porta está exposta
docker port wasolution-app
```

**Erro de conexão com banco de dados**
```bash
# Verifique se o PostgreSQL está saudável
docker-compose logs postgres

# Teste a conexão
docker-compose exec postgres pg_isready -U wasolution_user
```

### Problemas de Configuração

**Variáveis de ambiente não carregadas**
```bash
# Verifique se o arquivo .env existe
ls -la .env

# Verifique o conteúdo do arquivo
cat .env

# Teste as variáveis no container
docker-compose exec wasolution env | grep EVO
```

## 🔒 Deploy em Produção

### 1. Configuração de Segurança

Crie um arquivo `.env.production` com configurações seguras:

```env
# URLs das APIs (use HTTPS em produção)
EVO_URL=https://sua-api-evolution.com
WUZ_URL=https://sua-api-wuzapi.com

# Tokens seguros
EVO_TOKEN=token_seguro_evolution
WUZ_ADMIN_TOKEN=token_seguro_wuzapi

# Configuração do banco de dados
DB_URL=postgres://usuario:senha@host:porta/banco

# Configuração do servidor
IP_ADDRESS=0.0.0.0
DEFAULT_WEBHOOK=https://seu-servidor-producao.com/webhook
```

### 2. Docker Compose para Produção

Crie um `docker-compose.prod.yml`:

```yaml
version: '3.8'

services:
  wasolution:
    build:
      context: .
      dockerfile: Dockerfile
    container_name: wasolution-prod
    env_file:
      - .env.production
    ports:
      - "8080:8080"
    volumes:
      - ./logs:/app/logs
    restart: unless-stopped
    # Configurações de produção
    deploy:
      resources:
        limits:
          memory: 1G
          cpus: '0.5'
        reservations:
          memory: 512M
          cpus: '0.25'
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      interval: 30s
      timeout: 10s
      retries: 3
```

### 3. Deploy em Produção

```bash
# Use o arquivo de produção
docker-compose -f docker-compose.prod.yml up -d --build

# Verifique o status
docker-compose -f docker-compose.prod.yml ps

# Monitore os logs
docker-compose -f docker-compose.prod.yml logs -f
```

## 📊 Monitoramento

### Health Checks

Adicione health checks ao seu docker-compose:

```yaml
healthcheck:
  test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
  interval: 30s
  timeout: 10s
  retries: 3
  start_period: 40s
```

### Logs Estruturados

Configure logs estruturados:

```yaml
logging:
  driver: "json-file"
  options:
    max-size: "10m"
    max-file: "3"
```

### Métricas

Para monitoramento avançado, considere usar:

- **Prometheus**: Para métricas da aplicação
- **Grafana**: Para visualização de dashboards
- **ELK Stack**: Para análise de logs

## 🔄 Atualizações

### Atualizar a Aplicação

```bash
# 1. Pare os serviços
docker-compose down

# 2. Pull das últimas mudanças
git pull origin main

# 3. Rebuild e start
docker-compose up -d --build

# 4. Verifique os logs
docker-compose logs -f wasolution
```

### Rollback

```bash
# 1. Volte para a versão anterior
git checkout <commit-hash>

# 2. Rebuild
docker-compose up -d --build

# 3. Verifique se está funcionando
docker-compose logs wasolution
```

## 🛠️ Personalização

### Adicionar Redis

Se precisar de cache, adicione Redis:

```yaml
redis:
  image: redis:7-alpine
  container_name: wasolution-redis
  ports:
    - "6379:6379"
  volumes:
    - redis_data:/data
  networks:
    - wasolution-network
```

### Configurar Proxy Reverso

Para usar com Nginx:

```nginx
server {
    listen 80;
    server_name seu-dominio.com;

    location / {
        proxy_pass http://localhost:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

### Backup Automático

Configure backup automático do banco:

```bash
#!/bin/bash
# backup.sh
docker-compose exec postgres pg_dump -U wasolution_user wasolution > backup_$(date +%Y%m%d_%H%M%S).sql
```

## 📞 Suporte

Se encontrar problemas:

1. **Verifique os logs**: `docker-compose logs wasolution`
2. **Consulte a documentação**: [README.md](../README.md)
3. **Abra uma issue**: [GitHub Issues](https://github.com/pedroafonso18/wasolution/issues)
4. **Entre em contato**: pedroafonsoprogramador@gmail.com

## 🔗 Links Úteis

- [Documentação da API](../docs/api.md)
- [README Principal](../README.md)
- [Docker Documentation](https://docs.docker.com/)
- [Docker Compose Documentation](https://docs.docker.com/compose/)
