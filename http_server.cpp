#include "http_server.hh"
#include <bits/stdc++.h>
#include <time.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

inline vector<string> split(const string &s, char delim)
{
    vector<string> elems;

    stringstream ss(s);
    string item;

    while (getline(ss, item, delim))
    {
        if (!item.empty())
            elems.push_back(item);
    }

    return elems;
}

HTTP_Request::HTTP_Request(string request)
{
    vector<string> lines = split(request, '\n');
    vector<string> first_line = split(lines[0], ' ');

    this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

    if (first_line.size() > 0)
    {
        this->method = first_line[0];
        this->url = first_line[1];
    }

    if (this->method != "GET")
    {
        cerr << "Method '" << this->method << "' not supported" << endl;
        exit(1);
    }
}

HTTP_Response *handle_request(string req)
{

    HTTP_Request *request = new HTTP_Request(req);
    HTTP_Response *response = new HTTP_Response();

    string url = string("html_files") + request->url;

    response->HTTP_version = "1.0";
    char buffer[1000];
    time_t curr_time = time(0);
    struct tm tm = *gmtime(&curr_time);
    strftime(buffer, sizeof buffer, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    response->date = buffer;
    struct stat sb;
    if (stat(url.c_str(), &sb) == 0) // requested path exists
    {
        response->status_code = "200";
        response->status_text = "OK";
        response->content_type = "text/html";
        string body;
        if (S_ISDIR(sb.st_mode))
        {
            /*
                In this case, requested path is a directory.
            */
            if (url[url.size() - 1] == '/')
                url = url + "index.html";
            else
                url = url + "/index.html";
            if (stat(url.c_str(), &sb) != 0) // requested directory not found
            {
                response->status_code = "404";
                response->status_text = "Page not found";
                FILE *fd = fopen("html_files/not_found.html", "r");
                char c = fgetc(fd);
                response->body = "";
                int count = 1;
                while (c != EOF)
                {
                    response->body = response->body + c;
                    c = fgetc(fd);
                    count++;
                }
                fclose(fd);
                response->content_length = to_string(count);
                delete request;
                return response;
            }
        }

        /*
            found the file and return it as response
        */
        FILE *fd = fopen(url.c_str(), "r");
        char c = fgetc(fd);
        response->body = "";
        int count = 1;
        while (c != EOF)
        {
            response->body = response->body + c;
            c = fgetc(fd);
            count++;
        }
        fclose(fd);
        response->content_length = to_string(count);
    }

    else // requested file not found
    {
        response->status_code = "404";
        response->status_text = "Page not found";
        FILE *fd = fopen("html_files/not_found.html", "r");
        char c = fgetc(fd);
        response->body = "";
        int count = 1;
        while (c != EOF)
        {
            response->body = response->body + c;
            c = fgetc(fd);
            count++;
        }
        fclose(fd);

        response->content_length = to_string(count);
    }

    delete request;
    return response;
}

string HTTP_Response::get_string()
{
    string response = "HTTP/" + this->HTTP_version + " " + this->status_code + " " + this->status_text + '\n';
    response = response + "Date: " + this->date + '\n';
    response = response + "Content-Type: " + this->content_type + '\n';
    response = response + "Content-Length :" + this->content_length + '\n';
    response = response + "\r\n";
    response = response + this->body;
    return response;
}
