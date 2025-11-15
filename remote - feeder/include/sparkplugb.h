#ifndef SPARKPLUGB_H
#define SPARKPLUGB_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

// ===== TIPOS DE MÉTRICAS SPARKPLUG B =====
enum MetricType {
    INT32,
    FLOAT,
    BOOLEAN,
    STRING
};

// ===== ESTRUTURA DE MÉTRICA =====
struct Metric {
    String name;
    MetricType type;

    // Valores possíveis (union-like approach)
    int32_t intValue;
    float floatValue;
    bool boolValue;
    String stringValue;

    Metric(const String& n, int32_t val)
        : name(n), type(INT32), intValue(val) {}

    Metric(const String& n, float val)
        : name(n), type(FLOAT), floatValue(val) {}

    Metric(const String& n, bool val)
        : name(n), type(BOOLEAN), boolValue(val) {}

    Metric(const String& n, const String& val)
        : name(n), type(STRING), stringValue(val) {}
};

// ===== PAYLOAD SPARKPLUG B =====
class SparkplugBPayload {
private:
    std::vector<Metric> metrics;
    uint64_t timestamp;
    uint64_t seq;
    bool hasSeq;
    uint64_t bdSeq;
    bool hasBdSeq;

public:
    SparkplugBPayload();

    // Adicionar métricas
    void addMetric(const String& name, int32_t value);
    void addMetric(const String& name, float value);
    void addMetric(const String& name, bool value);
    void addMetric(const String& name, const String& value);

    // Configurar timestamp e sequência
    void setTimestamp(uint64_t ts);
    void setSeq(uint64_t sequence);
    void setBdSeq(uint64_t bdSequence);

    // Serializar para JSON (formato simplificado SparkplugB)
    String toJSON();

    // Limpar payload
    void clear();

    // Obter métrica por nome
    bool getMetric(const String& name, int32_t& value);
    bool getMetric(const String& name, float& value);
    bool getMetric(const String& name, bool& value);
    bool getMetric(const String& name, String& value);
};

// ===== PARSER DE DCMD =====
class SparkplugBParser {
public:
    static SparkplugBPayload parseJSON(const String& json);
};

#endif
