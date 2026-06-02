#include "ThermalSecurityCtrl.h"
#include <cmath>
#include <random>

/*
 * ============================================================================
 * ARCHIVO: ThermalSecurityCtrl.cc (Backend - C++)
 * DESCRIPCIÓN: Simulador del sistema de seguridad térmica.
 * HERRAMIENTAS: Drogon (API REST), <random> (Generación de temperaturas aleatorias),
 * jsoncpp (Formateo de datos a formato JSON).
 * FUNCIÓN: Crea temperaturas falsas para 5 zonas, las analiza y devuelve un
 * listado de sensores y alertas al frontend en formato JSON.
 * ============================================================================
 */

ThermalSecurityCtrl::ThermalSecurityCtrl() {
    std::string zonas[] = {"Piso 1", "Piso 2", "Piso 3", "Piso 4", "Piso 5"};
    for (int i = 0; i < 5; ++i) {
        SensorTermico st;
        st.id_sensor = i + 1;
        st.zona_monitoreada = zonas[i];
        sensores.push_back(st);
    }
}

void ThermalSecurityCtrl::guardarHistorial(const std::string& zona, float temp, const std::string& tipo) {
    if (registro.size() >= 100) {
        registro.erase(registro.begin()); // Mantener max 100
    }
    Historial h = {zona, temp, tipo};
    registro.push_back(h);
}

Json::Value ThermalSecurityCtrl::generarAlerta(SensorTermico& sensor) {
    sensor.alerta_generada = false;
    float cambio = std::abs(sensor.temperatura_detectada - sensor.temperatura_anterior);
    Json::Value resAlert(Json::arrayValue);

    if (cambio >= 5.0f) {
        guardarHistorial(sensor.zona_monitoreada, sensor.temperatura_detectada, "Cambio brusco");
        resAlert.append("Cambio brusco - Variacion de " + std::to_string(cambio) + " C");
    }

    if (sensor.temperatura_detectada >= 35.0f && sensor.temperatura_detectada <= 40.0f) {
        sensor.presencia_detectada = true;
        sensor.alerta_generada = true;
        guardarHistorial(sensor.zona_monitoreada, sensor.temperatura_detectada, "Persona detectada");
        resAlert.append("Presencia de persona detectada");
    } else if (sensor.temperatura_detectada >= 28.0f && sensor.temperatura_detectada < 35.0f) {
        sensor.presencia_detectada = true;
        sensor.alerta_generada = true;
        guardarHistorial(sensor.zona_monitoreada, sensor.temperatura_detectada, "Posible plaga");
        resAlert.append("Posible plaga detectada");
    } else if (sensor.temperatura_detectada > 40.0f) {
        sensor.presencia_detectada = false;
        sensor.alerta_generada = true;
        guardarHistorial(sensor.zona_monitoreada, sensor.temperatura_detectada, "Temperatura anormal");
        resAlert.append("Temperatura anormal detectada");
    } else {
        sensor.presencia_detectada = false;
        guardarHistorial(sensor.zona_monitoreada, sensor.temperatura_detectada, "Sin actividad");
        resAlert.append("Sin actividad");
    }

    sensor.temperatura_anterior = sensor.temperatura_detectada;
    return resAlert;
}

void ThermalSecurityCtrl::autorizar(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::lock_guard<std::mutex> lock(mtx_);
    acceso_autorizado = true;
    Json::Value resJson;
    resJson["status"] = "success";
    resJson["message"] = "Acceso autorizado correctamente.";
    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}

void ThermalSecurityCtrl::monitorearZona(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::lock_guard<std::mutex> lock(mtx_);
    Json::Value resJson;

    if (!acceso_autorizado) {
        resJson["status"] = "error";
        resJson["message"] = "Acceso denegado. Debe autorizarse primero.";
        auto resp = HttpResponse::newHttpJsonResponse(resJson);
        callback(resp);
        return;
    }

    int id_zona = (*json)["id_zona"].asInt();
    float temperatura = (*json)["temperatura"].asFloat();

    if (id_zona < 1 || id_zona > 5) {
        resJson["status"] = "error";
        resJson["message"] = "Zona no valida.";
    } else {
        sensores[id_zona - 1].temperatura_detectada = temperatura;
        Json::Value alertas = generarAlerta(sensores[id_zona - 1]);
        
        resJson["status"] = "success";
        resJson["message"] = "Monitoreo ejecutado";
        resJson["alertas"] = alertas;
    }

    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}

void ThermalSecurityCtrl::escanearGeneral(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::lock_guard<std::mutex> lock(mtx_);
    Json::Value resJson;

    if (!acceso_autorizado) {
        resJson["status"] = "error";
        resJson["message"] = "Acceso denegado. Debe autorizarse primero.";
        auto resp = HttpResponse::newHttpJsonResponse(resJson);
        callback(resp);
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(15.0, 50.0);

    Json::Value resultadosArr(Json::arrayValue);

    for (int i = 0; i < 5; ++i) {
        sensores[i].temperatura_detectada = static_cast<float>(dis(gen));
        Json::Value alertas = generarAlerta(sensores[i]);
        
        Json::Value r;
        r["id_zona"] = sensores[i].id_sensor;
        r["zona"] = sensores[i].zona_monitoreada;
        r["temperatura"] = sensores[i].temperatura_detectada;
        r["alertas"] = alertas;
        resultadosArr.append(r);
    }

    resJson["status"] = "success";
    resJson["message"] = "Escaneo general completado";
    resJson["resultados"] = resultadosArr;

    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}

void ThermalSecurityCtrl::estado(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::lock_guard<std::mutex> lock(mtx_);
    Json::Value resJson;
    Json::Value estadoArr(Json::arrayValue);

    for (const auto& s : sensores) {
        Json::Value item;
        item["id_sensor"] = s.id_sensor;
        item["zona"] = s.zona_monitoreada;
        item["temperatura_detectada"] = s.temperatura_detectada;
        item["temperatura_anterior"] = s.temperatura_anterior;
        item["presencia"] = s.presencia_detectada;
        item["alerta"] = s.alerta_generada;
        estadoArr.append(item);
    }

    resJson["status"] = "success";
    resJson["data"] = estadoArr;

    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}

void ThermalSecurityCtrl::historial(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::lock_guard<std::mutex> lock(mtx_);
    Json::Value resJson;

    if (registro.empty()) {
        resJson["status"] = "success";
        resJson["message"] = "No hay detecciones registradas aun.";
        resJson["data"] = Json::arrayValue;
    } else {
        Json::Value histArr(Json::arrayValue);
        for (const auto& h : registro) {
            Json::Value item;
            item["zona"] = h.zona;
            item["temperatura"] = h.temperatura;
            item["tipo_alerta"] = h.tipo_alerta;
            histArr.append(item);
        }
        resJson["status"] = "success";
        resJson["data"] = histArr;
    }

    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}
