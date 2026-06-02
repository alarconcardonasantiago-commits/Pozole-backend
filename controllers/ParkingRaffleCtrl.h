#pragma once
#include <drogon/HttpController.h>
#include <mutex>
#include <string>
#include <vector>

using namespace drogon;

struct Parqueadero {
    int num_parqueadero;
    bool ocupado = false;
};

struct SorteoData {
    std::string apartamento;
    bool inscrito = false;
    bool ganador = false;
    int parqueadero_asignado = -1;
};

class ParkingRaffleCtrl : public drogon::HttpController<ParkingRaffleCtrl> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ParkingRaffleCtrl::inscribir, "/api/parqueadero/inscribir", Post);
    ADD_METHOD_TO(ParkingRaffleCtrl::sorteo, "/api/parqueadero/sorteo", Post);
    ADD_METHOD_TO(ParkingRaffleCtrl::resultados, "/api/parqueadero/resultados", Get);
    ADD_METHOD_TO(ParkingRaffleCtrl::estado, "/api/parqueadero/estado", Get);
    ADD_METHOD_TO(ParkingRaffleCtrl::reset, "/api/parqueadero/reset", Post);
    METHOD_LIST_END

    ParkingRaffleCtrl();

    void inscribir(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void sorteo(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void resultados(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void estado(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);
    void reset(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback);

private:
    std::vector<SorteoData> datosSorteo;
    std::vector<Parqueadero> parqueaderos;
    std::mutex mtx_;
};
