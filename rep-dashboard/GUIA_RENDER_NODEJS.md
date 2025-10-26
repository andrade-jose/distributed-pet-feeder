# 🚀 Guia Completo: Configurar Dashboard como Web Service Node.js no Render

## ⚠️ PROBLEMA IDENTIFICADO

O serviço atual no Render está configurado como **Static Site** (serve apenas HTML/CSS/JS).
Precisamos configurá-lo como **Web Service** para executar o servidor Node.js e as APIs funcionarem.

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

#### **Root Directory** (Pasta raiz)
```
rep-dashboard
```

#### **Environment** (Ambiente)
```
Node
```

#### **Region** (Região)
```
São Paulo (South America)
```
*Ou a mais próxima de você*

#### **Branch** (Branch do Git)
```
main
```

#### **Build Command** (Comando de build)
```
npm install
```

#### **Start Command** (Comando de start)
```
npm start
```

#### **Instance Type** (Tipo de instância)
```
Free
```

---

### 4️⃣ Adicionar Variáveis de Ambiente

Antes de criar o serviço, role a página até **Environment Variables** e adicione:

| Key | Value |
|-----|-------|
| `NODE_ENV` | `production` |
| `SESSION_SECRET` | `AlimentadorPetSecure2024!@#$` |
| `MQTT_HOST` | `56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud` |
| `MQTT_PORT` | `8884` |
| `MQTT_USERNAME` | `NewWeb` |
| `MQTT_PASSWORD` | `Senha1234` |

Para adicionar cada variável:
1. Clique em **Add Environment Variable**
2. Digite o **Key** (nome da variável)
3. Digite o **Value** (valor da variável)
4. Repita para todas as variáveis acima

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
