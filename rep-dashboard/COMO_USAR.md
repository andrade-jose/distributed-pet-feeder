# 🐾 Como Usar o Dashboard do Alimentador de Pets

## ⚠️ PROBLEMA: "Erro no login: SyntaxError"

Este erro ocorre quando você abre o `index.html` diretamente no navegador (usando `file://`).

### Solução Rápida:

1. **Abra o arquivo `config.js`**
2. **Substitua a URL** na linha que diz:
   ```javascript
   const RENDER_URL = 'https://seu-app-no-render.onrender.com';
   ```

3. **Cole a URL do seu servidor Render.com**, por exemplo:
   ```javascript
   const RENDER_URL = 'https://pet-feeder-dashboard.onrender.com';
   ```

4. **Salve o arquivo** e **recarregue** a página no navegador

---

## 📋 Alternativas de Uso

### Opção 1: Usar o Dashboard Hospedado no Render
- Acesse diretamente a URL do seu Render (ex: `https://pet-feeder-dashboard.onrender.com`)
- Faça login normalmente
- **Vantagem:** Não precisa configurar nada!

### Opção 2: Rodar Localmente
1. Abra o terminal na pasta `rep-dashboard`
2. Instale as dependências:
   ```bash
   npm install
   ```
3. Inicie o servidor:
   ```bash
   npm start
   ```
4. Acesse `http://localhost:3000` no navegador

---

## 🔑 Credenciais de Login Padrão

| Usuário   | Senha       | Tipo          |
|-----------|-------------|---------------|
| operador  | operador123 | Operador      |
| dev       | dev123      | Desenvolvedor |
| admin     | admin123    | Desenvolvedor |

---

## 🔧 Configuração no Render.com

1. **Variáveis de Ambiente** no Render:
   - `MQTT_HOST`: `56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud`
   - `MQTT_PORT`: `8884`
   - `MQTT_USERNAME`: `NewWeb`
   - `MQTT_PASSWORD`: `Senha1234`
   - `SESSION_SECRET`: (gerado automaticamente)
   - `NODE_ENV`: `production`

2. **Comando de Build**: `npm install`
3. **Comando de Start**: `npm start`

---

## 🚨 Troubleshooting

### Erro: "Servidor não retornou JSON válido"
- ✅ Verifique se a URL no `config.js` está correta
- ✅ Verifique se o servidor Render está online
- ✅ Tente acessar a URL diretamente no navegador

### Erro: "MQTT desconectado"
- ✅ Apenas desenvolvedores podem conectar ao MQTT
- ✅ Faça login como `dev` ou `admin`
- ✅ Clique em "Conectar" após o login

### Erro: "Credenciais inválidas"
- ✅ Verifique se está usando a senha correta
- ✅ Verifique se o "Tipo de Acesso" corresponde ao usuário

---

## 📞 Suporte

Se o problema persistir:
1. Abra o Console do navegador (F12)
2. Verifique os erros na aba "Console"
3. Anote a mensagem de erro completa
4. Verifique se a URL da API está correta em `config.js`
