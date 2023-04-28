#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <map>
#include <iostream>
#include <algorithm>

#define PORT 8007
#define MAX_FILE_SIZE 20480

std::string head, doc;
std::map<std::string, std::string> options;
bool Req_Error;
std::string Req_Estr;

void handle_connect(int newsockfd);
void parse(FILE *fp, std::map<std::string, std::string> &options, std::string &doc);
void  ASErrorHandler(int errorNumber, const char* errorMessage);
char*  ASMemoryAlloc(unsigned long memoryNeeded);
extern "C" char*  AStyleMain(const char* sourceIn,
                                    const char* optionsIn,
                                    void (* fpError)(int, const char*),
                                    char* (* fpAlloc)(unsigned long));


int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        /* error */
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }
    bzero((void *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    { /* error */
        std::cerr << "Failed to bind server to socket" << std::endl;
        return 1;
    }
    listen(sockfd, 5);
    for (;;)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd,
                           (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        { /* error */
        std::cerr << "Failed to accept socket" << std::endl;
        return 1;
        }
        handle_connect(newsockfd);
    }
}


void handle_connect(int newsockfd)
{
    FILE *fp = fdopen(newsockfd, "r+");

    try {
        parse(fp, options, doc);
    }
    catch(std::runtime_error& e) {
        fprintf(fp, "%s", e.what());
        fclose(fp);
        return;
    }

    std::string parameter = "";
    for (auto const& [key, val] : options) {
        if (key == "SIZE")
            continue;
        
        std::string temp = key + "=" + val + "\n";
        parameter.append(temp);
    }

    Req_Error = false;
    Req_Estr = "";
    
    char* textOut = AStyleMain(doc.c_str(), parameter.c_str(), ASErrorHandler, ASMemoryAlloc);
    // if an error occurred create error message and send msg to client
    if (Req_Error) {
        std::string ret_message("ERR\nSIZE=" + std::to_string(Req_Estr.size()) + "\n\n" + Req_Estr.c_str());
        fprintf(fp, "%s", ret_message.c_str());
    } else {
        std::string ret_message("OK\nSIZE=" + std::to_string(strlen(textOut)) + "\n\n" + textOut);
        fprintf(fp, "%s", ret_message.c_str());
    }

    fclose(fp);
}

void ASErrorHandler(int errorNumber, const char* errorMessage) {   
    std::cout << "astyle error " << errorNumber << "\n"
         << errorMessage << std::endl;
    Req_Error = true;
    Req_Estr += errorMessage + std::string("\n");
}

char* ASMemoryAlloc(unsigned long memoryNeeded) {   // error condition is checked after return from AStyleMain
    char* buffer = new (std::nothrow) char [memoryNeeded];
    return buffer;
}


void parse(FILE *fp, std::map<std::string, std::string> &options, std::string &doc) {
    char buffer[1024];
    std::string head = fgets(buffer, 1024, fp);
    head.pop_back();

    std::string input;
    int size;

    if (head == "\0") {
        throw std::runtime_error("ERROR\n\nUnexpected end of file when reading header\n");
        return;
    }

    if (head != "ASTYLE") {
        throw std::runtime_error("ERROR\n" + head + "\n\nExpected header ASTYLE, but got " + head +"\n");
        return;
    }

    for (;;) {
        input = fgets(buffer, 1024, fp);

        if (input == "\n") {
            break;
        } else if (input == "\0") {
            throw std::runtime_error("ERROR\n" + input + "\n\nControl+C detected\n");
            return;
        }


        input.erase(std::remove_if(input.begin(), input.end(), ::isspace), input.end());
        size_t middle_pos = input.find("=");
        std::string leftOption, rightOption;

        if (middle_pos != std::string::npos && middle_pos > 0 && middle_pos < input.length() - 1) {
            leftOption = input.substr(0, middle_pos);
            rightOption = input.substr(middle_pos + 1);

            if (leftOption!="SIZE" && leftOption != "mode" && leftOption != "style") 
                throw std::runtime_error("ERROR\n"+leftOption+"\n\nBad option\n");
        } else {
            throw std::runtime_error("ERROR\n("+input+"\n\nBad option\n");
        }

        if (leftOption == "SIZE") {
            try {
                size = stoi(rightOption);
            } catch (const std::invalid_argument &e) {
                throw std::runtime_error("ERROR\n("+input+"\n\nBad option\n");
                std::cerr << e.what() << std::endl;
            }
        }

        options[leftOption] = rightOption;
    }

    if (size < 0 || size > MAX_FILE_SIZE) {
        throw std::runtime_error("\nERROR\n"+input+"\n\nBad code size\n");
    }

    char* docBuffer = new (std::nothrow) char [size];
    fread(docBuffer, size+1, 1, fp);
    doc = docBuffer;
}