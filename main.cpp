#include <drogon/drogon.h>
#include <cstdlib>
#include <string>

/*
 * ============================================================================
 * ARCHIVO: main.cpp (Backend - C++)
 * DESCRIPCIÓN: Punto de entrada principal de la aplicación.
 * HERRAMIENTA PRINCIPAL: Drogon (Framework web de alto rendimiento para C++).
 * FUNCIÓN: Inicia el servidor, define el puerto de escucha y configura las
 * reglas de CORS (Cross-Origin Resource Sharing) para permitir que el frontend
 * (React/Vite en otro dominio) se comunique sin bloqueos de seguridad.
 * ============================================================================
 */

int main() {
    // Definimos el puerto. Usamos la librería estándar <cstdlib> para leer variables de entorno.
    const char* portEnv = std::getenv("PORT");
    int port = portEnv ? std::stoi(std::string(portEnv)) : 8080;

    // Configuración de CORS: Interceptamos peticiones "OPTIONS" (preflight requests)
    // que hacen los navegadores por seguridad antes de peticiones complejas.
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
