#include "sparkplugb.h"

// ===== SPARKPLUG B PAYLOAD =====

SparkplugBPayload::SparkplugBPayload()
    : timestamp(0), seq(0), hasSeq(false), bdSeq(0), hasBdSeq(false) {
}

void SparkplugBPayload::addMetric(const String& name, int32_t value) {
    metrics.push_back(Metric(name, value));
}

void SparkplugBPayload::addMetric(const String& name, float value) {
    metrics.push_back(Metric(name, value));
}

void SparkplugBPayload::addMetric(const String& name, bool value) {
    metrics.push_back(Metric(name, value));
}

void SparkplugBPayload::addMetric(const String& name, const String& value) {
    metrics.push_back(Metric(name, value));
}

void SparkplugBPayload::setTimestamp(uint64_t ts) {
    timestamp = ts;
}

void SparkplugBPayload::setSeq(uint64_t sequence) {
    seq = sequence;
    hasSeq = true;
}

void SparkplugBPayload::setBdSeq(uint64_t bdSequence) {
    bdSeq = bdSequence;
    hasBdSeq = true;
}

String SparkplugBPayload::toJSON() {
    // Calcular tamanho necessário para JSON
    const size_t capacity = JSON_OBJECT_SIZE(10) + JSON_ARRAY_SIZE(metrics.size()) +
                           metrics.size() * JSON_OBJECT_SIZE(3) + 512;

    DynamicJsonDocument doc(capacity);

    // Timestamp
    doc["timestamp"] = timestamp;

    // Sequência (se presente)
    if (hasSeq) {
        doc["seq"] = seq;
    }

    // Birth/Death Sequência (se presente)
    if (hasBdSeq) {
        doc["bdSeq"] = bdSeq;
    }

    // Métricas
    if (metrics.size() > 0) {
        JsonArray metricsArray = doc.createNestedArray("metrics");

        for (const auto& metric : metrics) {
            JsonObject metricObj = metricsArray.createNestedObject();
            metricObj["name"] = metric.name;

            switch (metric.type) {
                case INT32:
                    metricObj["type"] = "Int32";
                    metricObj["value"] = metric.intValue;
                    break;

                case FLOAT:
                    metricObj["type"] = "Float";
                    metricObj["value"] = metric.floatValue;
                    break;

                case BOOLEAN:
                    metricObj["type"] = "Boolean";
                    metricObj["value"] = metric.boolValue;
                    break;

                case STRING:
                    metricObj["type"] = "String";
                    metricObj["value"] = metric.stringValue;
                    break;
            }
        }
    }

    // Serializar para string
    String output;
    serializeJson(doc, output);
    return output;
}

void SparkplugBPayload::clear() {
    metrics.clear();
    timestamp = 0;
    seq = 0;
    hasSeq = false;
    bdSeq = 0;
    hasBdSeq = false;
}

bool SparkplugBPayload::getMetric(const String& name, int32_t& value) {
    for (const auto& metric : metrics) {
        if (metric.name == name && metric.type == INT32) {
            value = metric.intValue;
            return true;
        }
    }
    return false;
}

bool SparkplugBPayload::getMetric(const String& name, float& value) {
    for (const auto& metric : metrics) {
        if (metric.name == name && metric.type == FLOAT) {
            value = metric.floatValue;
            return true;
        }
    }
    return false;
}

bool SparkplugBPayload::getMetric(const String& name, bool& value) {
    for (const auto& metric : metrics) {
        if (metric.name == name && metric.type == BOOLEAN) {
            value = metric.boolValue;
            return true;
        }
    }
    return false;
}

bool SparkplugBPayload::getMetric(const String& name, String& value) {
    for (const auto& metric : metrics) {
        if (metric.name == name && metric.type == STRING) {
            value = metric.stringValue;
            return true;
        }
    }
    return false;
}

// ===== PARSER DE DCMD =====

SparkplugBPayload SparkplugBParser::parseJSON(const String& json) {
    SparkplugBPayload payload;

    const size_t capacity = JSON_OBJECT_SIZE(10) + JSON_ARRAY_SIZE(20) +
                           20 * JSON_OBJECT_SIZE(3) + 1024;
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.printf("❌ Erro ao parsear JSON SparkplugB: %s\n", error.c_str());
        return payload;
    }

    // Timestamp
    if (doc.containsKey("timestamp")) {
        payload.setTimestamp(doc["timestamp"].as<uint64_t>());
    }

    // Sequência
    if (doc.containsKey("seq")) {
        payload.setSeq(doc["seq"].as<uint64_t>());
    }

    // Birth/Death Sequência
    if (doc.containsKey("bdSeq")) {
        payload.setBdSeq(doc["bdSeq"].as<uint64_t>());
    }

    // Métricas
    if (doc.containsKey("metrics")) {
        JsonArray metricsArray = doc["metrics"].as<JsonArray>();

        for (JsonObject metricObj : metricsArray) {
            String name = metricObj["name"].as<String>();
            String type = metricObj["type"].as<String>();

            if (type == "Int32") {
                payload.addMetric(name, metricObj["value"].as<int32_t>());
            } else if (type == "Float") {
                payload.addMetric(name, metricObj["value"].as<float>());
            } else if (type == "Boolean") {
                payload.addMetric(name, metricObj["value"].as<bool>());
            } else if (type == "String") {
                payload.addMetric(name, metricObj["value"].as<String>());
            }
        }
    }

    return payload;
}
