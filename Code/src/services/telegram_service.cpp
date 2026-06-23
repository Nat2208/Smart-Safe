// =====================================================
// telegram_service.cpp
// FINAL PRODUCTION VERSION
// =====================================================

#include <Arduino.h>

#include "telegram_service.h"

#include "core/globals.h"


// =====================================================
// EXTERN
// =====================================================
extern HardwareSerial simSerial;

extern SemaphoreHandle_t
simMutex;


// =====================================================
// TELEGRAM CONFIG
// =====================================================
#if __has_include("credentials.h")
#include "credentials.h"
#else
#define TELEGRAM_BOT_TOKEN "YOUR_BOT_TOKEN_HERE"
#define TELEGRAM_CHAT_ID "YOUR_CHAT_ID_HERE"
#endif

String BOT_TOKEN = TELEGRAM_BOT_TOKEN;
String CHAT_ID = TELEGRAM_CHAT_ID;


// =====================================================
// TELEGRAM TIMER
// =====================================================
extern unsigned long
lastTelegramTime;


// =====================================================
// READ SIM RESPONSE
// =====================================================
String readSIMResponse(
    int timeout
)
{
    String response = "";

    unsigned long start =
        millis();

    while (
        millis() - start
        < timeout
    )
    {
        while (
            simSerial.available()
        )
        {
            char c =
                simSerial.read();

            response += c;
        }

        // IMPORTANT
        vTaskDelay(
            10 / portTICK_PERIOD_MS
        );
    }

    return response;
}


// =====================================================
// URL ENCODE
// =====================================================
String encodeURL(
    String text
)
{
    text.replace(
        " ",
        "%20"
    );

    text.replace(
        "\n",
        "%0A"
    );

    text.replace(
        ":",
        "%3A"
    );

    text.replace(
        "/",
        "%2F"
    );

    return text;
}


// =====================================================
// SEND TELEGRAM
// =====================================================
bool sendTelegram(
    String text
)
{
    // =============================================
    // ANTI SPAM
    // =============================================
    if (
        millis() - lastTelegramTime
        < 1000
    )
    {
        Serial.println(
            "TELEGRAM SKIPPED"
        );

        return false;
    }

    // =============================================
    // TAKE MUTEX
    // =============================================
    if (
        xSemaphoreTake(
            simMutex,
            5000 /
            portTICK_PERIOD_MS
        ) != pdTRUE
    )
    {
        Serial.println(
            "SIM MUTEX FAIL"
        );

        return false;
    }

    Serial.println();

    Serial.println(
        "================================="
    );

    Serial.println(
        "SEND TELEGRAM START"
    );

    // =============================================
    // URL ENCODE
    // =============================================
    text =
        encodeURL(text);

    // =============================================
    // URL
    // =============================================
    String url =
        "https://api.telegram.org/bot" +
        BOT_TOKEN +
        "/sendMessage?chat_id=" +
        CHAT_ID +
        "&text=" +
        text;

    Serial.println();

    Serial.println(url);

    // =============================================
    // CLEAR BUFFER
    // =============================================
    while (
        simSerial.available()
    )
    {
        simSerial.read();
    }

    // =============================================
    // FORCE HTTP CLOSE
    // =============================================
    simSerial.println(
        "AT+HTTPTERM"
    );

    vTaskDelay(
        2000 /
        portTICK_PERIOD_MS
    );

    readSIMResponse(
        3000
    );

    // =============================================
    // HTTP INIT
    // =============================================
    simSerial.println(
        "AT+HTTPINIT"
    );

    String response =
        readSIMResponse(
            3000
        );

    Serial.println(
        response
    );

    // =============================================
    // CHECK INIT
    // =============================================
    if (
        response.indexOf("OK")
        == -1
    )
    {
        Serial.println(
            "HTTP INIT FAILED"
        );

        simSerial.println(
            "AT+HTTPTERM"
        );

        vTaskDelay(
            2000 /
            portTICK_PERIOD_MS
        );

        readSIMResponse(
            3000
        );

        xSemaphoreGive(
            simMutex
        );

        return false;
    }

    // =============================================
    // SET URL
    // =============================================
    simSerial.println(
        "AT+HTTPPARA=\"URL\",\"" +
        url +
        "\""
    );

    response =
        readSIMResponse(
            3000
        );

    Serial.println(
        response
    );

    // =============================================
    // CHECK URL
    // =============================================
    if (
        response.indexOf("OK")
        == -1
    )
    {
        Serial.println(
            "URL SET FAILED"
        );

        simSerial.println(
            "AT+HTTPTERM"
        );

        vTaskDelay(
            2000 /
            portTICK_PERIOD_MS
        );

        readSIMResponse(
            3000
        );

        xSemaphoreGive(
            simMutex
        );

        return false;
    }

    // =============================================
    // HTTP ACTION
    // =============================================
    simSerial.println(
        "AT+HTTPACTION=0"
    );

    vTaskDelay(
        500 /
        portTICK_PERIOD_MS
    );

    response =
        readSIMResponse(
            8000
        );

    Serial.println(
        response
    );

    // =============================================
    // SUCCESS
    // =============================================
    if (
        response.indexOf(
            "+HTTPACTION: 0,200"
        ) != -1
    )
    {
        Serial.println();

        Serial.println(
            "TELEGRAM SUCCESS"
        );

        // SAVE TIME
        lastTelegramTime =
            millis();
    }
    else
    {
        Serial.println();

        Serial.println(
            "TELEGRAM FAILED"
        );
    }

    // =============================================
    // CLOSE HTTP
    // =============================================
    simSerial.println(
        "AT+HTTPTERM"
    );

    vTaskDelay(
        2000 /
        portTICK_PERIOD_MS
    );

    readSIMResponse(
        3000
    );

    Serial.println();

    Serial.println(
        "SEND TELEGRAM DONE"
    );

    // =============================================
    // RELEASE MUTEX
    // =============================================
    xSemaphoreGive(
        simMutex
    );

    return true;
}