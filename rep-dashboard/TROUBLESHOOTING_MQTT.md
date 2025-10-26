# 🔧 Troubleshooting MQTT - Dashboard Pet Feeder

## 🚨 Problema: "Conexão MQTT fechada"

Se você está vendo logs assim:
```
[Sistema] Conectando a 56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud...
[Sistema] ⚠️ Conexão MQTT fechada
[Sistema] 🔄 Tentando reconectar...
```

### Causas Comuns e Soluções

#### 1. **Credenciais Inválidas** ❌

**Sintoma**: Conexão fecha imediatamente, sem mensagem de erro específica

**Solução**:
```bash
# Verificar se as variáveis de ambiente estão corretas
# No terminal do servidor local:
node -e "require('dotenv').config(); console.log({
    host: process.env.MQTT_HOST,
    port: process.env.MQTT_PORT,
    username: process.env.MQTT_USERNAME,
    password: '***'
});"
```

**No Render.com**:
1. Ir em Dashboard → Seu serviço → Environment
2. Verificar se todas as variáveis estão definidas:
   - `MQTT_HOST`
   - `MQTT_PORT`
   - `MQTT_USERNAME`
   - `MQTT_PASSWORD`

#### 2. **HiveMQ Cloud Inativo** 🌐

**Sintoma**: Conexão não estabelece, timeout

**Solução**:
1. Acessar [HiveMQ Cloud Console](https://console.hivemq.cloud/)
2. Verificar se o cluster está **RUNNING**
3. Verificar se o IP do seu servidor está na allowlist

**Adicionar IP no HiveMQ**:
1. Console → Seu cluster → Access Management
2. Clicar em "Add IP"
3. Adicionar `0.0.0.0/0` (todos os IPs) para teste

⚠️ **Produção**: Use apenas IPs específicos!

#### 3. **Porta Incorreta** 🔌

**Sintoma**: Conexão tenta WSS mas falha

**Verificar portas**:
- **WebSocket Seguro (WSS)**: `8884`
- **WebSocket (WS)**: `8083` (não recomendado)
- **MQTT**: `8883` (não funciona no browser)

**No código deve ser**:
```javascript
MQTT_PORT=8884  // ✅ Correto para WSS
```

#### 4. **CSP Bloqueando WSS** 🛡️

**Sintoma**: No console do browser aparece erro de CSP

**Solução**: Verificar [server.js](server.js) linha ~16:

```javascript
contentSecurityPolicy: {
    directives: {
        connectSrc: ["'self'", "wss://*.hivemq.cloud"],  // ✅ Deve incluir isso
    }
}
```

#### 5. **Cliente MQTT.js Desatualizado** 📦

**Sintoma**: Erro `mqtt.connect is not a function`

**Solução**: Verificar versão no [index.html](index.html):

```html
<!-- Usar versão atualizada -->
<script src="https://unpkg.com/mqtt@5.3.5/dist/mqtt.min.js"></script>
```

## 🧪 Teste Manual de Conexão

### Opção 1: Teste via Browser Console

Abra o DevTools (F12) e execute:

```javascript
// Teste direto no browser
const testClient = mqtt.connect('wss://56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud:8884/mqtt', {
    username: 'NewWeb',
    password: 'Senha1234',
    clientId: 'test_' + Math.random().toString(16).substr(2, 8)
});

testClient.on('connect', () => {
    console.log('✅ TESTE: Conectado!');
    testClient.end();
});

testClient.on('error', (err) => {
    console.error('❌ TESTE: Erro -', err);
});
```

### Opção 2: Teste via MQTT Explorer

1. Baixar [MQTT Explorer](http://mqtt-explorer.com/)
2. Configurar:
   - **Host**: `56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud`
   - **Port**: `8883` (MQTT normal)
   - **Protocol**: `mqtts://`
   - **Username**: `NewWeb`
   - **Password**: `Senha1234`
3. Clicar em "Connect"

Se conectar aqui, o problema está no código do dashboard!

### Opção 3: Teste via cURL/WebSocat

```bash
# Instalar websocat
# Windows (Chocolatey): choco install websocat
# Linux: apt install websocat
# Mac: brew install websocat

# Testar WebSocket
websocat wss://56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud:8884/mqtt
```

## 📊 Logs Detalhados

Com as melhorias que fizemos, agora você verá:

```
[Debug] URL: wss://...
[Debug] ClientID: web_feeder_abc123
[Erro MQTT] Connection refused: Not authorized
[Erro] Código: ECONNREFUSED
```

### Interpretando Erros

| Erro | Significado | Solução |
|------|-------------|---------|
| `Connection refused: Not authorized` | Credenciais erradas | Verificar username/password |
| `ECONNREFUSED` | Servidor não acessível | Verificar se HiveMQ está rodando |
| `ETIMEDOUT` | Timeout de conexão | Verificar firewall/proxy |
| `WebSocket connection failed` | WSS bloqueado | Verificar CSP/CORS |

## 🔍 Checklist de Diagnóstico

Execute este checklist:

- [ ] Variáveis de ambiente estão corretas (`.env` local ou Render)
- [ ] HiveMQ Cloud está RUNNING
- [ ] Porta é `8884` (WSS)
- [ ] URL é `wss://HOST:8884/mqtt` (não `mqtts://`)
- [ ] CSP permite `wss://*.hivemq.cloud`
- [ ] MQTT.js está carregado no HTML
- [ ] Login feito como "Desenvolvedor"
- [ ] Console do browser não mostra erros JavaScript

## 🆘 Ainda não funciona?

### Teste Mínimo Isolado

Crie um arquivo `test-mqtt.html`:

```html
<!DOCTYPE html>
<html>
<head>
    <title>Teste MQTT</title>
    <script src="https://unpkg.com/mqtt@5.3.5/dist/mqtt.min.js"></script>
</head>
<body>
    <h1>Teste MQTT</h1>
    <div id="log"></div>
    <script>
        function log(msg) {
            document.getElementById('log').innerHTML += msg + '<br>';
            console.log(msg);
        }

        log('Iniciando teste...');

        const client = mqtt.connect('wss://56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud:8884/mqtt', {
            username: 'NewWeb',
            password: 'Senha1234',
            clientId: 'test_' + Date.now()
        });

        client.on('connect', () => {
            log('✅ CONECTADO!');
            client.subscribe('test/topic');
            client.publish('test/topic', 'Hello from test');
        });

        client.on('message', (topic, message) => {
            log(`📨 Mensagem: ${message.toString()}`);
        });

        client.on('error', (err) => {
            log(`❌ Erro: ${err.message || err}`);
        });

        client.on('close', () => {
            log('⚠️ Conexão fechada');
        });
    </script>
</body>
</html>
```

Abra este arquivo no navegador. Se funcionar aqui, o problema está na integração com o servidor Node.js.

## 📞 Contato de Suporte

Se nada disso resolver:

1. **Verificar logs do servidor**:
   - Render: Dashboard → Logs
   - Local: Olhar terminal onde `npm start` está rodando

2. **Exportar logs**:
   - Copiar os logs completos
   - Incluir:
     - Mensagens de erro completas
     - URL exata de conexão
     - Variáveis de ambiente (SEM senhas!)

3. **Criar issue**:
   - Incluir versão do Node.js: `node --version`
   - Incluir sistema operacional
   - Incluir navegador e versão

---

**Boa sorte! 🍀**
