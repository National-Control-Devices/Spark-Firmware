#include "HttpClient.h"
#include "dnsclient.h"

//#define LOGGING
static const uint16_t TIMEOUT = 5000; // Allow maximum 5s between data packets.

IPAddress hAddress;

/**
* Constructor.
*/
HttpClient::HttpClient()
{

}

bool HttpClient::getDNSHost(String dnsHostName){
	DNSClient dns;
	IPAddress dnsServerIP(8,8,8,8);
	int ret = 0;
	dns.begin(dnsServerIP);
	ret = dns.getHostByName(dnsHostName.c_str(), hAddress);
	if(ret == 1){
		Serial.println("Got Host");
		return true;
	}else{
		Serial.println("Could not get host");
		return false;
	}
}

/**
* Method to send a header, should only be called from within the class.
*/
void HttpClient::sendHeader(const char* aHeaderName, const char* aHeaderValue)
{
    client.print(aHeaderName);
    client.print(": ");
    client.println(aHeaderValue);

    #ifdef LOGGING
    Serial.print(aHeaderName);
    Serial.print(": ");
    Serial.println(aHeaderValue);
    #endif
}

void HttpClient::sendHeader(const char* aHeaderName, const int aHeaderValue)
{
    client.print(aHeaderName);
    client.print(": ");
    client.println(aHeaderValue);

    #ifdef LOGGING
    Serial.print(aHeaderName);
    Serial.print(": ");
    Serial.println(aHeaderValue);
    #endif
}

void HttpClient::sendHeader(const char* aHeaderName)
{
    client.println(aHeaderName);

    #ifdef LOGGING
    Serial.println(aHeaderName);
    #endif
}

/**
* Method to send an HTTP Request. Allocate variables in your application code
* in the aResponse struct and set the headers and the options in the aRequest
* struct.
*/
void HttpClient::request(http_request_t &aRequest, http_response_t &aResponse, http_header_t headers[], const char* aHttpMethod)
{
	long startSend = millis();

    aResponse.status = -1;

    bool connected = false;
//    Serial.println("Connecting to server");
    connected = client.connect(hAddress, aRequest.port);
    if(!connected){
    	Serial.println("Connect failed");
    	return;
    }else{
//    	Serial.println("Connected to server");
    }


    #ifdef LOGGING
    if (connected) {
        if(aRequest.hostname!=NULL) {
            Serial.print("HttpClient>\tConnecting to: ");
            Serial.print(aRequest.hostname);
        } else {
            Serial.print("HttpClient>\tConnecting to IP: ");
            Serial.print(aRequest.ip);
        }
        Serial.print(":");
        Serial.println(aRequest.port);
    } else {
        Serial.println("HttpClient>\tConnection failed.");
        client.stop();
        return;
    }
    #endif

    //
    // Send HTTP Headers
    //

    // Send initial headers (only HTTP 1.0 is supported for now).
    client.print(aHttpMethod);
    client.print(" ");
    client.print(aRequest.path);
    client.print(" HTTP/1.0\r\n");

    #ifdef LOGGING
    Serial.println("HttpClient>\tStart of HTTP Request.");
    Serial.print(aHttpMethod);
    Serial.print(" ");
    Serial.print(aRequest.path);
    Serial.print(" HTTP/1.0\r\n");
    #endif

    // Send General and Request Headers.
    sendHeader("Connection", "close"); // Not supporting keep-alive for now.
    if(aRequest.hostname!=NULL) {
        sendHeader("HOST", aRequest.hostname.c_str());
    }

    //Send Entity Headers
    // TODO: Check the standard, currently sending Content-Length : 0 for empty
    // POST requests, and no content-length for other types.
    if (aRequest.body != NULL) {
        sendHeader("Content-Length", (aRequest.body).length());
    } else if (strcmp(aHttpMethod, HTTP_METHOD_POST) == 0) { //Check to see if its a Post method.
        sendHeader("Content-Length", 0);
    }

    if (headers != NULL)
    {
        int i = 0;
        while (headers[i].header != NULL)
        {
            if (headers[i].value != NULL) {
                sendHeader(headers[i].header, headers[i].value);
            } else {
                sendHeader(headers[i].header);
            }
            i++;
        }
    }

    // Empty line to finish headers
    client.println();
    client.flush();

    //
    // Send HTTP Request Body
    //

    if (aRequest.body != NULL) {
    	//TODO: convert client.println to client.write(buffer, size)
        client.println(aRequest.body);

        #ifdef LOGGING
        Serial.println(aRequest.body);
        #endif
    }

    Serial.print("Send complete after: ");
    long sendElapsedTime = millis() - startSend;
    Serial.print(sendElapsedTime);
    Serial.println(" ms");

    #ifdef LOGGING
    Serial.println("HttpClient>\tEnd of HTTP Request.");
    #endif

    // clear response buffer
    memset(buffer, 0, sizeof(buffer));


    //
    // Receive HTTP Response
    //
    // The first value of client.available() might not represent the
    // whole response, so after the first chunk of data is received instead
    // of terminating the connection there is a delay and another attempt
    // to read data.
    // The loop exits when the connection is closed, or if there is a
    // timeout or an error.

    unsigned int bufferPosition = 0;
    unsigned long lastRead = millis();
    unsigned long firstRead = millis();
    bool error = false;
    bool timeout = false;

    long startReceive = millis();

    do {
        #ifdef LOGGING
        int bytes = client.available();
        if(bytes) {
            Serial.print("\r\nHttpClient>\tReceiving TCP transaction of ");
            Serial.print(bytes);
            Serial.println(" bytes.");
        }
        #endif

        while (client.available()) {
            char c = client.read();
//            Serial.print(c);
            #ifdef LOGGING
            Serial.print(c);
            #endif
            lastRead = millis();

            if (c == -1) {
                error = true;
                Serial.println("Error thrown during response read");

                #ifdef LOGGING
                Serial.println("HttpClient>\tError: No data available.");
                #endif

                break;
            }

            // Check that received character fits in buffer before storing.
            if (bufferPosition < sizeof(buffer)-1) {
                buffer[bufferPosition] = c;
            } else if ((bufferPosition == sizeof(buffer)-1)) {
                buffer[bufferPosition] = '\0'; // Null-terminate buffer
                client.stop();
                error = true;

                #ifdef LOGGING
                Serial.println("HttpClient>\tError: Response body larger than buffer.");
                #endif
            }
            bufferPosition++;
        }
//        buffer[bufferPosition] = '\0'; // Null-terminate buffer

        #ifdef LOGGING
        if (bytes) {
            Serial.print("\r\nHttpClient>\tEnd of TCP transaction.");
        }
        #endif

        // Check that there hasn't been more than 5s since last read.
        timeout = millis() - lastRead > TIMEOUT;

        // Unless there has been an error or timeout wait 200ms to allow server
        // to respond or close connection.
        if (!error && !timeout) {
            delay(50);
        }
    } while (client.connected() && !timeout && !error);

    Serial.print("Receive Complete after ");
    long elapsedReceiveTime = millis() - startReceive;
    Serial.print(elapsedReceiveTime);
    Serial.println(" ms");

    if(timeout){
    	Serial.println("Timeout waiting for response.");
    	Serial.print("Received bytes: ");
    	Serial.println(bufferPosition);
    }

    #ifdef LOGGING
    if (timeout) {
        Serial.println("\r\nHttpClient>\tError: Timeout while reading response.");
    }
    Serial.print("\r\nHttpClient>\tEnd of HTTP Response (");
//    Serial.print(millis() - firstRead);
    Serial.println("ms).");
    #endif
    client.stop();

    String raw_response(buffer);

    // Not super elegant way of finding the status code, but it works.
    String statusCode = raw_response.substring(9,12);

    #ifdef LOGGING
    Serial.print("HttpClient>\tStatus Code: ");
    Serial.println(statusCode);
    #endif

    int bodyPos = raw_response.indexOf("\r\n\r\n");
    if (bodyPos == -1) {
        #ifdef LOGGING
        Serial.println("HttpClient>\tError: Can't find HTTP response body.");
        #endif

        return;
    }
    // Return the entire message body from bodyPos+4 till end.
//    aResponse.body = "";
    int size = bufferPosition-(bodyPos +4);
    char returnData[1024];
//    Serial.print("Response.body: ");
    for(int i = 0; i < size; i++){
    	aResponse.body[i] = buffer[i+(bodyPos +4)];
//    	Serial.print(buffer[i+(bodyPos +4)]);
    }
//    aResponse.body = returnData;
    aResponse.responseLength = size;
    aResponse.status = atoi(statusCode.c_str());
}
