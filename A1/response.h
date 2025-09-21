#ifndef RESPONSE_H
#define RESPONSE_H
#include<string>
#include<map>
#include<fstream>
#include<sstream>
#include<iostream>
class HttpResponse{ 
    std::string statuscode;
    std::string statusmsg;
    std::map<std::string, std::string> headers;
    std::string body;
    public:
    std::string frameHttpResponse(std::string statuscode, std::string statusmsg, std::map<std::string, std::string> headers, std::string body,std::string mimetype);
        
    

};


#endif