#include <drogon/drogon.h>
#include <cstdlib>
#include <string>

int main() {
    // Render asigna el puerto mediante la variable de entorno PORT
    const char* portEnv = std::getenv("PORT");
    int port = portEnv ? std::stoi(std::string(portEnv)) : 8080;

    // Interceptar cualquier OPTIONS agresivamente antes de que Drogon busque la ruta
    drogon::app().registerSyncAdvice(
        [](const drogon::HttpRequestPtr& req) -> drogon::HttpResponsePtr {
            if (req->method() == drogon::Options) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->addHeader("Access-Control-Allow-Origin", "*");
                resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With, Bypass-Tunnel-Reminder");
                resp->addHeader("Access-Control-Max-Age", "86400");
                resp->setStatusCode(drogon::k204NoContent);
                return resp; // Responde de inmediato
            }
            return nullptr; // Continúa normal
        }
    );

    // Adjuntar headers CORS a todas las respuestas exitosas o errores
    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr&, const drogon::HttpResponsePtr& resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With, Bypass-Tunnel-Reminder");
        }
    );

    drogon::app()
        .addListener("0.0.0.0", port)
        .run();

    return 0;
}
