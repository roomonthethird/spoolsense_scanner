#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#ifndef NATIVE_TEST
#include <WebServer.h>
#endif

class WebServerManager {
public:
    static WebServerManager& getInstance();

    // Call once in setup() after WiFi is connected.
    // Registers routes, starts mDNS as "spoolsense".
    bool begin(uint16_t port = 80);

    // Call from loop() — processes pending HTTP requests.
    void handleClient();

private:
    WebServerManager() = default;
    WebServerManager(const WebServerManager&) = delete;
    WebServerManager& operator=(const WebServerManager&) = delete;

#ifndef NATIVE_TEST
    WebServer _server{80};
    bool _initialized = false;

    // Page handlers
    void handleLanding();
    void handleReader();
    void handleOpenPrintTagWriter();
    void handleTigerTagWriter();
    void handleSharedCSS();
    void handleSharedJS();
    void handleOpenPrintTagLogo();
    void handleTigerTagLogo();

    // API handlers
    void handleApiStatus();
    void handleApiWriteTag();
    void handleApiFormatTag();
    void handleApiWriteTigerTag();

    void sendError(int code, const char* msg);
#endif
};

#endif // WEB_SERVER_MANAGER_H
