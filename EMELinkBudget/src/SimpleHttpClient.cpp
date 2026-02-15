#include "SimpleHttpClient.h"
#include <windows.h>
#include <winhttp.h>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

bool SimpleHttpClient::fetchUrl(const std::string& url, std::string& response) {
    response.clear();

    size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) {
        return false;
    }

    std::string protocol = url.substr(0, protocolEnd);
    std::string remainder = url.substr(protocolEnd + 3);

    size_t hostEnd = remainder.find('/');
    std::string host = remainder.substr(0, hostEnd);
    std::string path = (hostEnd != std::string::npos) ? remainder.substr(hostEnd) : "/";

    std::wstring wHost(host.begin(), host.end());
    std::wstring wPath(path.begin(), path.end());

    HINTERNET hSession = WinHttpOpen(
        L"Mutsumi Wakaba / 01.14",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession) {
        return false;
    }

    DWORD dwFlags = (protocol == "https") ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        wHost.c_str(),
        (protocol == "https") ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT,
        0
    );

    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        wPath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        dwFlags
    );

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    BOOL bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (!bResults) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    bResults = WinHttpReceiveResponse(hRequest, NULL);

    if (bResults) {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        std::ostringstream oss;

        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                break;
            }

            if (dwSize == 0) {
                break;
            }

            char* pszOutBuffer = new char[dwSize + 1];
            ZeroMemory(pszOutBuffer, dwSize + 1);

            if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                delete[] pszOutBuffer;
                break;
            }

            oss.write(pszOutBuffer, dwDownloaded);
            delete[] pszOutBuffer;

        } while (dwSize > 0);

        response = oss.str();
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return !response.empty();
}
