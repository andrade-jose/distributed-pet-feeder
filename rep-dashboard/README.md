# 🐾 Dashboard Pet Feeder - Versão Segura 2.0

Dashboard web seguro para controle e monitoramento de alimentadores automáticos de pets.

## ✨ Melhorias de Segurança v2.0

### 🔒 Segurança Implementada

- ✅ **Autenticação Server-Side**: Senhas hashadas com bcrypt
- ✅ **Credenciais MQTT Protegidas**: Nunca expostas no cliente
- ✅ **Sessões Seguras**: express-session com cookies httpOnly
- ✅ **Rate Limiting**: Proteção contra força bruta
- ✅ **Helmet.js**: Headers de segurança (CSP, X-Frame-Options, etc)
- ✅ **Validação de Input**: Sanitização de todas as entradas
- ✅ **Variáveis de Ambiente**: Credenciais no servidor, não no código

### 🔐 Problemas Corrigidos

- ❌ ~~Credenciais hardcoded no JavaScript~~
- ❌ ~~Senhas em texto plano~~
- ❌ ~~Senhas no localStorage~~
- ❌ ~~Autenticação apenas client-side~~
- ❌ ~~Sem validação de mensagens MQTT~~

## 🚀 Deploy no Render.com

### 1. Configurar Variáveis de Ambiente

No painel do Render, configure:

```
MQTT_HOST=56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud
MQTT_PORT=8884
MQTT_USERNAME=NewWeb
MQTT_PASSWORD=Senha1234
NODE_ENV=production
SESSION_SECRET=(será gerado automaticamente)
```

### 2. Deploy Automático

O Render detectará o `render.yaml` e fará deploy automaticamente:

```bash
npm install    # Build command
npm start      # Start command
```

### 3. Verificar Deployment

Após o deploy, acesse: `https://pet-feeder-dashboard.onrender.com`

## 🖥️ Desenvolvimento Local

### Pré-requisitos

- Node.js 18+
- npm ou yarn

### Instalação

```bash
# Instalar dependências
npm install

# Copiar arquivo de exemplo
cp .env.example .env

# Editar .env com suas credenciais
# IMPORTANTE: Nunca commitar o arquivo .env!

# Iniciar servidor
npm start
```

O dashboard estará disponível em: `http://localhost:3000`

## 👥 Usuários Padrão

### Operador
- **Usuário**: `operador`
- **Senha**: `operador123`
- **Permissões**: Visualizar apenas

### Desenvolvedor
- **Usuário**: `dev`
- **Senha**: `dev123`
- **Permissões**: Controle total + MQTT

### Admin
- **Usuário**: `admin`
- **Senha**: `admin123`
- **Permissões**: Controle total + MQTT

⚠️ **IMPORTANTE**: Altere estas senhas em produção editando o arquivo `server.js`!

## 🔧 Alterar Senhas

Para alterar as senhas, edite o arquivo `server.js`:

```javascript
const USERS = {
    'operador': {
        passwordHash: bcrypt.hashSync('SUA_NOVA_SENHA', 10),
        role: 'operador'
    },
    // ...
};
```

## 📡 Arquitetura

```
┌─────────────┐      HTTPS        ┌──────────────┐
│   Browser   │ ◄────────────────► │ Node.js      │
│  (Cliente)  │   Sessão Segura    │ Express      │
└─────────────┘                    └──────┬───────┘
                                          │
                                          │ Credenciais
                                          │ protegidas
                                          ▼
                                   ┌──────────────┐
                                   │ HiveMQ Cloud │
                                   │ (MQTT Broker)│
                                   └──────────────┘
```

## 🛠️ Tecnologias

- **Backend**: Node.js + Express
- **Segurança**: Helmet, bcrypt, express-session, express-rate-limit
- **Frontend**: Vanilla JS + MQTT.js
- **Deploy**: Render.com

## 📂 Estrutura de Arquivos

```
rep-dashboard/
├── server.js           # Servidor Node.js seguro
├── index.html          # Interface web
├── script.js           # Lógica do cliente (segura)
├── style.css           # Estilos
├── package.json        # Dependências
├── render.yaml         # Configuração Render
├── .env.example        # Exemplo de variáveis
└── README.md           # Este arquivo
```

## 🔍 API Endpoints

### Autenticação

- `POST /api/login` - Login de usuário
- `POST /api/logout` - Logout de usuário
- `GET /api/session` - Verificar sessão atual

### MQTT (apenas devs autenticados)

- `GET /api/mqtt-config` - Obter credenciais MQTT
- `POST /api/validate-mqtt-message` - Validar mensagem antes de enviar

## 🐛 Troubleshooting

### Erro: "Configuração MQTT não carregada"
- Verifique se você fez login como desenvolvedor
- Verifique as variáveis de ambiente no Render

### Erro: "Muitas tentativas de login"
- Rate limit ativo. Aguarde 15 minutos

### MQTT não conecta
- Verifique as credenciais no painel do Render
- Verifique se o HiveMQ Cloud está acessível

## 📝 Licença

Projeto educacional - Alimentador de Pets Distribuído

## 👨‍💻 Autor

Alexandre - Projeto Alimentador de Pets
