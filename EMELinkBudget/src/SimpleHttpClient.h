#pragma once

#include <string>

class SimpleHttpClient {
public:
    static bool fetchUrl(const std::string& url, std::string& response);
    static bool fetchUrlWithStatus(const std::string& url, std::string& response, int& statusCode, std::string& errorMsg);
};
