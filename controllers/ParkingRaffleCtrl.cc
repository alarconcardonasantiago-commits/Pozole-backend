#include "ParkingRaffleCtrl.h"
#include <algorithm>
#include <random>

ParkingRaffleCtrl::ParkingRaffleCtrl() {
    std::string apts[] = {"101","102","201","202","301","302","401","402","501","502"};
    for (int i = 0; i < 10; ++i) {
        SorteoData sd;
        sd.apartamento = apts[i];
        datosSorteo.push_back(sd);
    }
    for (int i = 1; i <= 20; ++i) {
        Parqueadero p;
        p.num_parqueadero = i;
        parqueaderos.push_back(p);
    }
}

void ParkingRaffleCtrl::inscribir(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string num_apartamento = (*json)["apartamento"].asString();
    Json::Value resJson;
    bool encontrado = false;

    std::lock_guard<std::mutex> lock(mtx_);

    for (auto& dato : datosSorteo) {
        if (dato.apartamento == num_apartamento) {
            encontrado = true;
            if (dato.inscrito) {
                resJson["status"] = "error";
                resJson["message"] = "Ya inscrito";
            } else {
                dato.inscrito = true;
                resJson["status"] = "success";
                resJson["message"] = "Inscrito correctamente";
            }
            break;
        }
    }

    if (!encontrado) {
        resJson["status"] = "error";
        resJson["message"] = "No existe";
    }

    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}

void ParkingRaffleCtrl::sorteo(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::lock_guard<std::mutex> lock(mtx_);
    Json::Value resJson;

    std::vector<int> participantes;
    for (size_t i = 0; i < datosSorteo.size(); ++i) {
        if (datosSorteo[i].inscrito && !datosSorteo[i].ganador) {
            participantes.push_back(i);
        }
    }

    if (participantes.empty()) {
        resJson["status"] = "error";
        resJson["message"] = "No hay inscritos disponibles para sorteo";
        auto resp = HttpResponse::newHttpJsonResponse(resJson);
        callback(resp);
        return;
    }

    int libres = 0;
    for (auto& p : parqueaderos) {
        if (!p.ocupado) libres++;
    }

    if (libres == 0) {
        resJson["status"] = "error";
        resJson["message"] = "No hay parqueaderos disponibles";
        auto resp = HttpResponse::newHttpJsonResponse(resJson);
        callback(resp);
        return;
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(participantes.begin(), participantes.end(), g);

    int ganadores_count = (participantes.size() < libres) ? participantes.size() : libres;
    
    int j = 0;
    Json::Value listaGanadores(Json::arrayValue);

    for (int k = 0; k < ganadores_count; ++k) {
        while (parqueaderos[j].ocupado) {
            j++;
        }

        int a = participantes[k];
        datosSorteo[a].ganador = true;
        datosSorteo[a].parqueadero_asignado = parqueaderos[j].num_parqueadero;
        parqueaderos[j].ocupado = true;

        Json::Value item;
        item["apartamento"] = datosSorteo[a].apartamento;
        item["parqueadero"] = "P-0" + std::to_string(parqueaderos[j].num_parqueadero);
        listaGanadores.append(item);

        j++;
    }

    resJson["status"] = "success";
    resJson["message"] = "Sorteo ejecutado";
    resJson["ganadores"] = listaGanadores;

    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}

void ParkingRaffleCtrl::resultados(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::lock_guard<std::mutex> lock(mtx_);
    Json::Value resJson;
    Json::Value resultadosArr(Json::arrayValue);

    bool hayInscritos = false;

    for (const auto& dato : datosSorteo) {
        if (dato.inscrito) {
            hayInscritos = true;
            Json::Value item;
            item["apartamento"] = dato.apartamento;
            if (dato.ganador) {
                item["resultado"] = "GANADOR P-0" + std::to_string(dato.parqueadero_asignado);
            } else {
                item["resultado"] = "Sin asignacion";
            }
            resultadosArr.append(item);
        }
    }

    if (!hayInscritos) {
        resJson["status"] = "error";
        resJson["message"] = "No hay inscritos aun";
    } else {
        resJson["status"] = "success";
        resJson["data"] = resultadosArr;
    }

    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}

void ParkingRaffleCtrl::estado(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::lock_guard<std::mutex> lock(mtx_);
    Json::Value resJson;
    Json::Value estadoArr(Json::arrayValue);

    for (const auto& p : parqueaderos) {
        Json::Value item;
        item["parqueadero"] = "P-0" + std::to_string(p.num_parqueadero);
        item["estado"] = p.ocupado ? "OCUPADO" : "LIBRE";
        estadoArr.append(item);
    }

    resJson["status"] = "success";
    resJson["data"] = estadoArr;

    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}

void ParkingRaffleCtrl::reset(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
    std::lock_guard<std::mutex> lock(mtx_);
    Json::Value resJson;
    
    for (auto& p : parqueaderos) {
        p.ocupado = false;
    }
    for (auto& d : datosSorteo) {
        d.inscrito = false;
        d.ganador = false;
        d.parqueadero_asignado = -1;
    }

    resJson["status"] = "success";
    resJson["message"] = "Parqueadero reseteado";

    auto resp = HttpResponse::newHttpJsonResponse(resJson);
    callback(resp);
}
