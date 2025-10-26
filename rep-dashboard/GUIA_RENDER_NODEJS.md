# 🚀 Guia Completo: Configurar Dashboard como Web Service Node.js no Render

## ⚠️ PROBLEMA IDENTIFICADO

O serviço atual no Render está configurado como **Static Site** (serve apenas HTML/CSS/JS).
Precisamos configurá-lo como **Web Service** para executar o servidor Node.js e as APIs funcionarem.

---

## ⚡ RESUMO RÁPIDO

1. **Deletar** o serviço atual (Static Site)
2. Criar **novo Web Service** conectado ao repositório `distributed-pet-feeder`
3. Configurar:
   - Build Command: `npm install`
   - Start Command: `npm start`
   - Instance Type: **Free**
4. Adicionar **6 variáveis de ambiente** (NODE_ENV, SESSION_SECRET, MQTT_HOST, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD)
5. Clicar em **Create Web Service**
6. Aguardar deploy (2-3 minutos)

---

## 📋 PASSO A PASSO

### 1️⃣ Deletar o Serviço Atual (Static Site)

1. Acesse [https://dashboard.render.com](https://dashboard.render.com)
2. Clique no serviço `pet-feeder-dashboard` ou `distributed-pet-feeder`
3. Vá em **Settings** (no menu lateral esquerdo)
4. Role até o final da página
5. Clique em **Delete Web Service**
6. Confirme digitando o nome do serviço
7. Clique em **Delete**

---

### 2️⃣ Criar Novo Web Service (Node.js)

1. No dashboard do Render, clique em **New +** (canto superior direito)
2. Selecione **Web Service**
3. Conecte ao repositório:
   - Se já conectou antes: selecione `distributed-pet-feeder` da lista
   - Se não: clique em **Connect GitHub** → autorize → selecione o repositório

---

### 3️⃣ Configurar o Web Service

Preencha os campos com estas informações:

#### **Name** (Nome do serviço)
```
pet-feeder-dashboard
```

#### **Region** (Região)
Selecione: **Oregon (US West)** ou qualquer outra disponível

#### **Branch** (Branch do Git)
```
main
```

#### **Root Directory** (deixe vazio ou configure depois)
O Render vai usar o `render.yaml` que já está configurado com `rootDir: rep-dashboard`

#### **Build Command** (Comando de build)
```
npm install
```
*Ou use `yarn` se preferir*

#### **Start Command** (Comando de start)
```
npm start
```
*Ou use `yarn start` se usou yarn no build*

#### **Instance Type** (Tipo de instância)
Selecione: **Free** (512 MB RAM, 0.1 CPU, $0/month)

---

### 4️⃣ Adicionar Variáveis de Ambiente

Na seção **Environment Variables**, você verá:
- Um campo **NAME_OF_VARIABLE** (nome da variável)
- Um campo **value** (valor da variável)
- Botão **+ Add Environment Variable** para adicionar mais

Adicione as seguintes variáveis uma por uma:

**Variável 1:**
- NAME_OF_VARIABLE: `NODE_ENV`
- value: `production`

**Variável 2:**
- NAME_OF_VARIABLE: `SESSION_SECRET`
- value: `AlimentadorPetSecure2024!@#$`

**Variável 3:**
- NAME_OF_VARIABLE: `MQTT_HOST`
- value: `56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud`

**Variável 4:**
- NAME_OF_VARIABLE: `MQTT_PORT`
- value: `8884`

**Variável 5:**
- NAME_OF_VARIABLE: `MQTT_USERNAME`
- value: `NewWeb`

**Variável 6:**
- NAME_OF_VARIABLE: `MQTT_PASSWORD`
- value: `Senha1234`

> 💡 **Dica:** Clique em **+ Add Environment Variable** após preencher cada par de variável para adicionar a próxima

---

### 5️⃣ Criar o Serviço

1. Role até o final da página
2. Clique em **Create Web Service**
3. Aguarde o deploy (2-3 minutos)

---

## ✅ Como Saber se Funcionou

Após o deploy terminar, teste a API:

### Teste 1: Página inicial
Acesse a URL do Render no navegador. Deve aparecer a tela de login.

### Teste 2: API de sessão
```bash
curl https://seu-app.onrender.com/api/session
```

**Resposta esperada:**
```json
{"authenticated":false}
```

**Se retornar 404:** o serviço ainda está como Static Site ❌

### Teste 3: Login
Abra o dashboard pela URL do Render e tente fazer login:
- Usuário: `dev`
- Senha: `dev123`
- Tipo: Desenvolvedor

**Se funcionar:** ✅ Configurado corretamente!

---

## 🔍 Verificar Logs

Se algo der errado, veja os logs:

1. No dashboard do Render, clique no serviço
2. Vá em **Logs** (menu lateral)
3. Procure por erros em vermelho

---

## 📝 Configuração Automática (Alternativa)

Como agora temos o `render.yaml` na raiz do repositório, você também pode:

1. Deletar o serviço atual
2. No Render dashboard, clique em **New +** → **Blueprint**
3. Conecte ao repositório `distributed-pet-feeder`
4. O Render vai **ler automaticamente** o `render.yaml` e criar o serviço com as configurações corretas!

---

## 🆘 Problemas Comuns

### Erro: "Application failed to respond"
- ✅ Verifique se o `Start Command` é `npm start`
- ✅ Verifique se o `Root Directory` é `rep-dashboard`
- ✅ Veja os logs para erros

### Erro: "Module not found"
- ✅ Rode `npm install` localmente para garantir que package.json está correto
- ✅ Verifique se todas as dependências estão em `dependencies` (não `devDependencies`)

### Erro: "Port already in use"
- ✅ O código já está correto: `process.env.PORT || 3000`
- ✅ O Render define automaticamente a variável `PORT`

---

## 📞 Suporte

Se precisar de ajuda:
- Render Docs: https://render.com/docs/web-services
- Discord Render: https://render.com/discord

---

## ✨ Resultado Final

Após configurar corretamente:

✅ API funcionando: `https://seu-app.onrender.com/api/login`
✅ Dashboard acessível: `https://seu-app.onrender.com`
✅ Login funcional
✅ MQTT conectável (para devs)
✅ Sem erros de JSON vazio!

---

**Última atualização:** 2025-10-26
**Autor:** Configuração automática via `render.yaml`
