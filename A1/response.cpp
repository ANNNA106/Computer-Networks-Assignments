#include"response.h"
std::string HttpResponse::frameHttpResponse(std::string statuscode, std::string statusmsg, std::map<std::string, std::string> headers, std::string body,std::string mimetype){
    //TODO
    // status code , status msg , headers , body ko response formate me frame krna hai.
    
    headers["content-type"] = mimetype;
    headers["content-length"] = std::to_string(body.length());
    std::ostringstream buffer;
    buffer << "HTTP/1.1 " << statuscode << " " << statusmsg << "\r\n";
    for(auto x : headers){
        buffer << x.first << ": " << x.second << "\r\n";
    }
    buffer << "\r\n" << body;
    return buffer.str();


}