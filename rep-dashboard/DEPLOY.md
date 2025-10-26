# 🚀 Guia de Deploy - Dashboard Pet Feeder

## Passos para Deploy no Render.com

### 1️⃣ Preparar o Repositório

Certifique-se de que todos os arquivos estão commitados:

```bash
git add .
git commit -m "feat: implementar dashboard seguro v2.0"
git push origin main
```

### 2️⃣ Criar Serviço no Render

1. Acesse [Render.com](https://render.com)
2. Clique em **"New +"** → **"Web Service"**
3. Conecte seu repositório GitHub
4. O Render detectará automaticamente o `render.yaml`

### 3️⃣ Configurar Variáveis de Ambiente

No painel do Render, vá em **Environment** e adicione:

| Chave | Valor | Descrição |
|-------|-------|-----------|
| `MQTT_HOST` | `56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud` | Host do broker HiveMQ |
| `MQTT_PORT` | `8884` | Porta WSS do HiveMQ |
| `MQTT_USERNAME` | `NewWeb` | Usuário MQTT |
| `MQTT_PASSWORD` | `Senha1234` | Senha MQTT |
| `NODE_ENV` | `production` | Ambiente |
| `SESSION_SECRET` | *(auto-gerado)* | Deixe o Render gerar |

✅ Você já tem essas variáveis configuradas conforme a imagem!

### 4️⃣ Deploy Automático

O Render vai:
1. ✅ Detectar `render.yaml`
2. ✅ Executar `npm install`
3. ✅ Iniciar com `npm start`
4. ✅ Configurar HTTPS automaticamente

### 5️⃣ Verificar Deploy

Após alguns minutos, acesse:
- **URL**: `https://pet-feeder-dashboard.onrender.com`
- **Status**: Dashboard → Logs

### 6️⃣ Testar Funcionalidades

1. **Login como Desenvolvedor**:
   - Usuário: `dev`
   - Senha: `dev123`
   - Tipo: Desenvolvedor

2. **Verificar MQTT**:
   - Clicar em "Conectar"
   - Ver log: "Conectado ao MQTT!"
   - Ver log: "Configuração MQTT carregada com segurança"

3. **Testar Descoberta de Remotas**:
   - Aguardar heartbeats das remotas
   - Ver alimentadores aparecerem automaticamente

## 🔧 Alterar Senhas em Produção

**IMPORTANTE**: Antes de usar em produção, altere as senhas!

### Método 1: Via Hash (Recomendado)

1. Gere o hash da nova senha localmente:

```bash
node -e "console.log(require('bcryptjs').hashSync('NOVA_SENHA', 10))"
```

2. Copie o hash gerado (ex: `$2a$10$...`)

3. Edite `server.js` e substitua:

```javascript
const USERS = {
    'dev': {
        passwordHash: '$2a$10$SEU_HASH_AQUI',
        role: 'dev'
    },
    // ...
};
```

4. Commit e push:

```bash
git add server.js
git commit -m "chore: atualizar senhas de produção"
git push origin main
```

5. O Render fará redeploy automaticamente

### Método 2: Variáveis de Ambiente (Futuro)

Para máxima segurança, considere mover usuários para banco de dados.

## 🐛 Troubleshooting

### Deploy falhou

**Erro**: `Cannot find module 'express'`
- **Solução**: Verificar `package.json` está commitado

**Erro**: `Port already in use`
- **Solução**: Render define `PORT` automaticamente, não hardcode

### MQTT não conecta

**Erro**: "Configuração MQTT não carregada"
- **Solução**: Verificar variáveis de ambiente no Render
- Fazer logout e login novamente como dev

**Erro**: "WebSocket connection failed"
- **Solução**: Verificar se HiveMQ Cloud está ativo
- Testar credenciais localmente primeiro

### Rate Limit atingido

**Erro**: "Muitas tentativas de login"
- **Solução**: Aguardar 15 minutos
- Ou reiniciar serviço no Render (resetará contador)

### Sessão expira rapidamente

**Problema**: Logout automático frequente
- **Solução**: Aumentar `maxAge` em `server.js`:

```javascript
cookie: {
    maxAge: 7 * 24 * 60 * 60 * 1000 // 7 dias
}
```

## 📊 Monitoramento

### Logs em Tempo Real

No Render:
1. Dashboard → Seu serviço
2. Aba **Logs**
3. Ver conexões, erros, etc

### Métricas

No Render:
1. Dashboard → Seu serviço
2. Aba **Metrics**
3. Ver CPU, Memória, Requisições

## 🔄 Atualizações Futuras

Para atualizar o dashboard:

```bash
# Fazer alterações locais
git add .
git commit -m "feat: nova funcionalidade"
git push origin main
```

O Render fará **redeploy automático**!

## ✅ Checklist Pós-Deploy

- [ ] Dashboard acessível via HTTPS
- [ ] Login funciona (operador e dev)
- [ ] MQTT conecta com sucesso
- [ ] Remotas são descobertas automaticamente
- [ ] Comandos são enviados corretamente
- [ ] Logs aparecem em tempo real
- [ ] Senhas padrão foram alteradas
- [ ] Variáveis de ambiente estão corretas

## 🎉 Pronto!

Seu dashboard está no ar de forma **SEGURA** e **PROFISSIONAL**!

---

**Dúvidas?** Consulte o [README.md](README.md) para mais informações.
