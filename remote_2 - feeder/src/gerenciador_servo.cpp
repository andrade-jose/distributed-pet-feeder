#include "gerenciador_servo.h"

ServoControl::ServoControl()
{
    pinoServo = -1;
    ativo = false;
}

void ServoControl::iniciar(int pino)
{
    pinoServo = pino;

    // Configuração simples sem alocação manual de timers
    servo.setPeriodHertz(50);           // 50Hz padrão para servos
    servo.attach(pinoServo);            // Deixa a biblioteca gerenciar os timers automaticamente

    // Inicializa na posição central (90°) para servo 180°
    servo.writeMicroseconds(1500);      // 90° = centro para servo 180°
    delay(500); // Aguarda estabilizar
    
    Serial.printf("🔧 Servo PDI 6221MG (180°) iniciado no pino %d (90° centro)\n", pino);
}

void ServoControl::ativar()
{
    ativo = true;
    Serial.println("⚙️ Servo ativado.");
}

void ServoControl::desativar()
{
    ativo = false;
    Serial.println("🛑 Servo desativado.");
}

void ServoControl::girarHorarioComFlag(int velocidade)
{
    if (!ativo)
    {
        Serial.println("⚠️ Servo inativo. Use ativar() para permitir o movimento.");
        return;
    }

    velocidade = constrain(velocidade, 0, 100);
    int pwm = pwmParaVelocidade(velocidade, true);
    servo.writeMicroseconds(pwm);
    Serial.printf("➡️ Girando horário (%d%%) - PWM %dμs\n", velocidade, pwm);
}

void ServoControl::girarHorario(int velocidade)
{
    if (!ativo)
        return;

    velocidade = constrain(velocidade, 0, 100);
    int pwm = pwmParaVelocidade(velocidade, true);
    servo.writeMicroseconds(pwm);
}

void ServoControl::moverPara90AntiHorario()
{
    Serial.println("↩️ Indo para posição neutra (1520μs, anti-horário).");
    servo.writeMicroseconds(1000); // anti-horário máximo
    delay(300);     // pequeno giro
    parar();
    Serial.println("✅ Parado em neutro (1520μs).");
}

void ServoControl::parar()
{
    servo.writeMicroseconds(1500); // posição central (90°) para servo 180°
    delay(100); // Garante que o comando foi enviado
    Serial.println("⏹️ Servo na posição central (90°).");
}

void ServoControl::pararImediato()
{
    servo.writeMicroseconds(1500); // posição central (90°) sem delay
}

// Novo método para controle posicional por ângulos
void ServoControl::moverParaAngulo(int angulo)
{
    if (!ativo)
    {
        Serial.println("⚠️ Servo inativo. Use ativar() para permitir o movimento.");
        return;
    }
    
    // Limitar ângulo entre 0° e 180°
    angulo = constrain(angulo, 0, 180);
    
    // Mapear ângulo (0-180°) para microsegundos (500-2500μs)
    int micros = map(angulo, 0, 180, 500, 2500);
    
    servo.writeMicroseconds(micros);
    Serial.printf("🔄 Servo movido para %d° (%dμs)\n", angulo, micros);
}

// Método específico para posições do alimentador
void ServoControl::alimentar(int porcao)
{
    if (!ativo)
    {
        Serial.println("⚠️ Servo inativo. Use ativar() para permitir alimentação.");
        return;
    }
    
    int angulo;
    String descricao;
    
    switch(porcao)
    {
        case 1: // Porção pequena
            angulo = 45;
            descricao = "PORÇÃO PEQUENA";
            break;
        case 2: // Porção média  
            angulo = 90;
            descricao = "PORÇÃO MÉDIA";
            break;
        case 3: // Porção grande
            angulo = 135;
            descricao = "PORÇÃO GRANDE";
            break;
        default:
            angulo = 90;
            descricao = "PORÇÃO PADRÃO";
            break;
    }
    
    Serial.printf("🍽️ %s (%d°)\n", descricao.c_str(), angulo);
    moverParaAngulo(angulo);
    delay(1000); // Tempo para alimentar
    
    // Voltar ao centro
    moverParaAngulo(90);
    Serial.println("✅ Alimentação concluída - Servo no centro");
}

void ServoControl::testar()
{
    Serial.println("🧪 Teste: movimento controlado...");

    ativar();
    
    // Teste horário suave
    Serial.println("   ➡️ Testando rotação horária...");
    servo.writeMicroseconds(1600); // Rotação suave horária
    delay(800);
    
    // Para
    servo.writeMicroseconds(1520); // Para
    delay(500);
    
    // Teste anti-horário suave  
    Serial.println("   ⬅️ Testando rotação anti-horária...");
    servo.writeMicroseconds(1400); // Rotação suave anti-horária
    delay(800);
    
    // Para definitivamente
    servo.writeMicroseconds(1520);
    delay(200);

    Serial.println("✅ Teste concluído - Servo estabilizado.");
}

bool ServoControl::estaAtivo()
{
    return ativo;
}

int ServoControl::pwmParaVelocidade(int percentual, bool horario)
{
    // Para servo 1520μs: range 1000-2000μs
    // Neutro em 1520μs, horário 1520-2000μs, anti-horário 1000-1520μs
    if (horario)
    {
        return map(percentual, 0, 100, 1520, 2000); // 1520μs → 2000μs
    }
    else
    {
        return map(percentual, 0, 100, 1520, 1000); // 1520μs → 1000μs
    }
}