# 📱 Especificação das Telas - Sistema de Alimentação

## 🏠 **TELA HOME (Principal)**
```
┌────────────────────┐
│    14:35:22        │  ← Hora atual (hh:mm:ss)
│ > Lista de Remotas │  ← Opção selecionada
│   WiFi             │
│   Config Central   │
└────────────────────┘
```
**Navegação:** UP/DOWN para selecionar, ENTER para entrar

---

## 📶 **TELA WiFi**
```
┌────────────────────┐
│ Rede: MinhaWiFi    │  ← Nome da rede
│ Estado: Conectado  │  ← Conectado/Desconectado
│ Qualidade: Forte   │  ← Forte/Medio/Fraco
│ MQTT: Conectado    │  ← Status HiveMQ broker
└────────────────────┘
```
**Navegação:** Qualquer botão volta para HOME

---

## ⚙️ **TELA Config Central**
```
┌────────────────────┐
│ > Ultimo Boot      │  ← Mostra data/hora
│   Resetar Sistema  │
│   Luz LCD: ON      │  ← ON/OFF
│   Voltar           │
└────────────────────┘
```

### 📅 **Sub-tela: Ultimo Boot**
```
┌────────────────────┐
│ 30/07/2025         │  ← Data do último boot
│ 14:30:15           │  ← Hora do último boot
│                    │
│                    │
└────────────────────┘
```

### 🔄 **Sub-tela: Resetar Sistema**
```
┌────────────────────┐
│ Confirmar reset?   │
│ > Sim              │  ← Seleção
│   Nao              │
│                    │
└────────────────────┘
```

---

## 📡 **TELA Lista de Remotas - Página 1**
```
┌────────────────────┐
│ > Remota 1: OK     │  ← Conectada
│   Remota 2: OFF    │  ← Desconectada
│   Remota 3: OK     │  ← Conectada
│   Voltar           │  ← Indicador
└────────────────────┘
```

---

## 🤖 **TELA Remota Específica**
```
┌────────────────────┐
│ > Refeicao 1: 08:00│  ← Horário configurado
│   Refeicao 2: 14:30│
│   Refeicao 3: 20:00│
│   Voltar           │
└────────────────────┘
```

---

## 🍽️ **TELA Configuração de Refeição**
```
┌────────────────────┐
│ > Hora: 08:00      │  ← Configurar horário
│   Quantidade: 040g │  ← Quantidade em gramas
│   Ultima: 07:45    │  ← Última vez executada
│   Voltar           │
└────────────────────┘
```

### ⏰ **Sub-tela: Editar Hora**
```
┌────────────────────┐
│ Configurar hora:   │
│                    │
│     [08]:[00]      │  ← Hora editável
│                    │
└────────────────────┘
```

### ⚖️ **Sub-tela: Editar Quantidade**
```
┌────────────────────┐
│ Quantidade (gramas)│
│                    │
│      [040]g        │  ← Quantidade editável
│                    │
└────────────────────┘
```

---

## 🗂️ **Estrutura de Navegação**

```
HOME
├── Lista de Remotas
│   ├── Página (Lista de remotas)
│   │   └── Remota X
│   │       └── Refeição Y
│   │           ├── Editar Hora
│   │           └── Editar Quantidade
├── WiFi (Info apenas)
└── Config Central
    ├── Ultimo Boot (Info apenas)
    ├── Resetar Sistema
    └── Luz LCD Toggle
```

---

## 🎮 **Controles de Navegação**

### **Botões:**
- **UP**: Subir no menu/diminuir valor
- **DOWN**: Descer no menu/aumentar valor  
- **ENTER**: Selecionar/confirmar/editar

### **Comportamentos:**
- **Telas de informação**: Qualquer botão volta ou 10 segundos
- **Menus**: UP/DOWN navega, ENTER seleciona
- **Edição**: UP/DOWN altera valor, ENTER confirma apos o ENTER vai para o proximo no ultimo apos o ENTER volta
- **"Voltar"**: Sempre presente em submenus

---

**Após sua validação, implementarei toda essa estrutura de telas!**


