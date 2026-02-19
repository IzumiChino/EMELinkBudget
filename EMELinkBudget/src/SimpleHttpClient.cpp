#include "SimpleHttpClient.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <cstring>

// Callback function for writing response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool SimpleHttpClient::fetchUrl(const std::string& url, std::string& response) {
    int statusCode = 0;
    std::string errorMsg;
    return fetchUrlWithStatus(url, response, statusCode, errorMsg);
}

bool SimpleHttpClient::fetchUrlWithStatus(const std::string& url, std::string& response, int& statusCode, std::string& errorMsg) {
    response.clear();
    statusCode = 0;
    errorMsg.clear();

    CURL* curl = curl_easy_init();
    if (!curl) {
        errorMsg = "curl_easy_init failed";
        return false;
    }

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Set user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mutsumi Wakaba / 01.14");

    // Set timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    // Follow redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Set write callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // SSL verification options
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        errorMsg = "curl_easy_perform failed: ";
        errorMsg += curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        return false;
    }

    // Get HTTP status code
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    statusCode = static_cast<int>(http_code);

    curl_easy_cleanup(curl);

    if (statusCode != 200) {
        errorMsg = "HTTP status code: " + std::to_string(statusCode);
        return false;
    }

    if (response.empty()) {
        errorMsg = "Empty response received";
        return false;
    }

    return true;
}
