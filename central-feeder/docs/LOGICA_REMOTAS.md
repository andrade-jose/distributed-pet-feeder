# LÓGICA DAS REMOTAS - ESPECIFICAÇÃO TÉCNICA

## 📋 VISÃO GERAL

Sistema de gerenciamento de remotas baseado em **MQTT** com lógica de "sinal de vida" para detectar remotas conectadas. Interface otimizada para navegação em LCD 20x4.

---

## 🖥️ INTERFACE DE TELAS

### 1. TELA LISTA DE REMOTAS

#### 📌 **Cenário 1: Nenhuma Remota Conectada**
```
┌──────────────────────┐
│                      │
│ Nenhuma remota       │
│ conectada            │
│                      │
│ > Voltar             │
└──────────────────────┘
```

#### 📌 **Cenário 2: 1-3 Remotas Conectadas**
```
┌──────────────────────┐
│ > Remota 1: Jardim   │
│   Remota 2: Sala     │
│   Remota 3: Quintal  │
│   Voltar             │
└──────────────────────┘
```

#### 📌 **Cenário 3: 4-6 Remotas (Paginação)**
**Página 1 de 2:**
```
┌──────────────────────┐
│ > Remota 1: Jardim   │
│   Remota 2: Sala     │
│   Remota 3: Quintal  │
│ [Página 1 de 3]      │
└──────────────────────┘
```

**Página 2 de 2:**
```
┌──────────────────────┐
│ > Remota 4: Varanda  │
│   Remota 5: Cozinha  │
│   Remota 6: Lavabo   │
│ [Página 2 de 3]      │
└──────────────────────┘
```
**Página 2 de 2:**
```
┌──────────────────────┐
│   Voltar             │
│                      │
│                      │
│ [Página 3 de 3]      │
└──────────────────────┘
```

### 2. TELA DE REFEIÇÕES (Mantida Igual)
```
┌──────────────────────┐
│ Remota 1: Jardim     │
│ > Refeição 1         │
│   Refeição 2         │
│   Refeição 3         │
│   Voltar             │
└──────────────────────┘
```

### 3. NOVA TELA: HORA E QUANTIDADE
```
┌──────────────────────┐
│ Refeição 1 - Jardim  │
│ > Horário: 08:00     │
│   Quantidade: 040g   │
│   Voltar             │
└──────────────────────┘
```

---

## ⚙️ LÓGICA DE EDIÇÃO

### 🕐 **EDIÇÃO DE HORÁRIO**

#### Fluxo de Navegação:
1. **Entrada**: Cursor inicia em "Horário: 08:00"
2. **ENTER**: Entra no modo de edição
3. **Navegação**: UP/DOWN altera valor, ENTER avança casa
4. **Sequência**: Hora (dezena) → Hora (unidade) → Minuto (dezena) → Minuto (unidade) → OK
5. **Saída**: Após OK, volta para tela Hora e Quantidade

#### Estados de Edição:
```
Estado 1: [0]8:00  ← Editando dezena da hora
Estado 2: 0[8]:00  ← Editando unidade da hora  
Estado 3: 08:[0]0  ← Editando dezena do minuto
Estado 4: 08:0[0]  ← Editando unidade do minuto
Estado 5: [OK]     ← Confirmar alterações
```

#### Validações:
- **Hora**: 00-23 (dezena: 0-2, unidade: 0-9 com restrições)
- **Minuto**: 00-59 (dezena: 0-5, unidade: 0-9)

### 📊 **EDIÇÃO DE QUANTIDADE**

#### Formato de Exibição:
- **Display**: "000g" (3 dígitos + unidade)
- **Range**: 000g - 990g
- **Incremento**: 10g por passo

#### Fluxo de Navegação:
1. **Entrada**: Cursor em "Quantidade: 040g"
2. **ENTER**: Entra no modo de edição
3. **UP/DOWN**: Altera em passos de 10g
4. **ENTER**: Confirma valor
5. **Saída**: Volta para tela Hora e Quantidade

#### Conversão Interna:
- **1 grama = 1 segundo de acionamento**
- **Exemplo**: 040g = 40 segundos de motor vai ficar em uma posiçao(mas so salve esse valor)

---

## 🏗️ ARQUITETURA DE DADOS

### 📁 **Nova Estrutura de Arquivos**

#### 1. **include/scheduling/meal_scheduler.h**
```cpp
struct MealTime {
    int hour;
    int minute;
    bool isActive;
};

struct MealData {
    MealTime schedule;
    int quantity_grams;
    int duration_seconds;  // Convertido automaticamente
    bool isScheduled;
};

struct RemoteData {
    int id;
    String name;
    bool isConnected;
    unsigned long lastHeartbeat;
    MealData meals[3];  // 3 refeições por remota
};

class MealScheduler {
public:
    static void init();
    static void update();
    static bool checkMealTime(int remoteId, int mealId);
    static void setMealSchedule(int remoteId, int mealId, int hour, int minute);
    static void setMealQuantity(int remoteId, int mealId, int grams);
    static void triggerFeeding(int remoteId, int mealId);
    
private:
    static RemoteData remotes[MAX_REMOTAS];
    static bool compareTimes(MealTime meal, TimeData current);
    static int gramsToSeconds(int grams);
};
```

#### 2. **src/scheduling/meal_scheduler.cpp**
- Implementação da lógica de comparação de horários
- Conversão gramas → segundos
- Gerenciamento de flags de acionamento
- Persistência em Preferences

### 📡 **Integração MQTT (Futura)**

#### Tópicos Planejados:
```
alimentador/remota/{id}/heartbeat    → Sinal de vida
alimentador/remota/{id}/feed         → Comando de alimentação
alimentador/remota/{id}/status       → Status da remota
alimentador/central/schedule         → Sincronização de horários
```

#### Detecção de Remotas:
- **Heartbeat**: A cada 30 segundos
- **Timeout**: 2 minutos sem sinal = remota desconectada
- **Auto-discovery**: Novas remotas aparecem automaticamente na lista

---

## 🎛️ ALTERAÇÕES NA INTERFACE

### ❌ **ITENS REMOVIDOS**
- **"Buscar Remota"**: Removido do menu (detecção automática via MQTT)
- **Tela "Último Boot"**: Substituída pela tela "Hora e Quantidade da ultima alimentaçao registrada pela remota n"

### ✅ **ITENS MODIFICADOS**

#### Menu Principal:
```
┌──────────────────────┐
│        14:25         │
│ > Lista de Remotas   │
│   WiFi               │
│   Config Central     │
└──────────────────────┘
```

#### Navegação de Remotas:
- **Paginação**: Automática quando > 3 remotas
- **Indicador**: "[Página X de Y]" na linha 4
- **Botão Voltar**: Sempre na a uma linha de distacia da ultima remota mesmo que tenah que ficar em outra pagina

---

## 💾 PERSISTÊNCIA DE DADOS

### Preferences Namespaces:
```cpp
// Namespace para cada remota
#define PREFS_REMOTE_PREFIX "remote_"

// Chaves para horários
#define PREFS_MEAL_HOUR "meal_h_"
#define PREFS_MEAL_MINUTE "meal_m_"
#define PREFS_MEAL_QUANTITY "meal_q_"

// Exemplo: remote_1_meal_h_0 = hora da refeição 0 da remota 1
```

---

## 🔄 FLUXO DE FUNCIONAMENTO

### 1. **Inicialização**
```
Sistema Liga → Carrega Remotas Salvas → Inicia MQTT → Aguarda Heartbeats
```

### 2. **Detecção de Remotas**
```
Heartbeat MQTT → Atualiza Lista → Marca como Conectada → Atualiza Interface
```

### 3. **Agendamento de Refeições**
```
RTC Atualiza → Compara Horários → Flag Acionamento → Envia Comando MQTT
```

### 4. **Edição de Configurações**
```
Interface → Valida Entrada → Salva em Preferences → Atualiza Scheduler
```

---

## 🎯 VALIDAÇÕES NECESSÁRIAS

### ✅ **Confirmações Solicitadas:**

1. **Interface de remotas** está adequada? não precisa se precupara
2. **Lógica de paginação** está clara? sim
3. **Fluxo de edição de horários** (casa por casa) está correto? sim
4. **Edição de quantidade** (10g por passo) está adequada? sim
5. **Conversão 10g = 1s** está correta? sim
6. **Estrutura de arquivos** `meal_scheduler.h/cpp` está apropriada? sim
7. **Remoção do "Buscar Remota"** está OK? sim
8. **Remoção da tela "Último Boot"** está OK? sim
8. **Validar como a remota mandar para o broker tem no arquivo hivemq_config"** está OK? sim

### 🚀 **Próximos Passos:**
1. ✅ Validação deste documento
2. ⏳ Implementação da estrutura MealScheduler
3. ⏳ Atualização das telas de interface
4. ⏳ Implementação da lógica de edição
5. ⏳ Testes e validação

---

**📝 Status**: Aguardando validação para iniciar implementação
**📅 Criado**: 01/08/2025
**🔄 Versão**: 1.0
