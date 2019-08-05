#include "agent_wget.h"
#include "common.h"
#include "httpparser/httpresponseparser.h"
#include "httpparser/response.h"
#include "iasrequest.h"
#include <bits/stdc++.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace std;
using namespace httpparser;

#include <string>
#include <vector>

#define CHUNK_SZ 8192
#define WGET_NO_ERROR 0
#define WGET_SERVER_ERROR 8

string AgentWget::name = "wget";

int AgentWget::request(string const& url, string const& post, Response& response, int type)
{
    HttpResponseParser parser;
    int pipefd[2];
    pid_t pid;
    string arg;
    static vector<string> wget_args;
    string sresponse;
    int status;
    char buffer[CHUNK_SZ];
    size_t bread;
    int repeat;
    int rv = 1;
    char tmpfile[] = "/tmp/wgetpostXXXXXX";
    int postdata = 0;

    if (post.length() > 0) {
        int fd = mkstemp(tmpfile);
        size_t bwritten, rem;
        const char* bp = post.c_str();

        fprintf(stderr, "+++ POST data written to %s\n", tmpfile);

        postdata = 1;

        if (fd == -1) {
            perror("mkstemp");
            return 0;
        }

        rem = post.length();
        while (rem) {
        retry_write:
            bwritten = write(fd, bp, rem);
            if (bwritten == -1) {
                if (errno == EINTR)
                    goto retry_write;
                else {
                    perror("write");
                    close(fd);
                    unlink(tmpfile);
                    return 0;
                }
            }
            rem -= bwritten;
            bp += bwritten;
        }

        close(fd);
    }

    wget_args.clear();
    wget_args.push_back("wget");

    // Output options

    if (type == REQUEST_MSG3) {
        wget_args.push_back("--header=\"content-type:application/json\"");
    }

    wget_args.push_back("--output-document=-");
    wget_args.push_back("--save-headers");
    wget_args.push_back("--content-on-error");
    wget_args.push_back("--no-http-keep-alive");
    //TODO:debug mode
    //wget_args.push_back("--no-check-certificate");

    arg = "--certificate=";
    arg += conn->client_cert_file();
    wget_args.push_back(arg);

    arg = "--certificate-type=";
    arg += conn->client_cert_type();
    wget_args.push_back(arg);

    arg = conn->client_key_file();
    if (arg != "") {
        arg = "--private-key=" + arg;
        wget_args.push_back(arg);

        // Sanity assumption: the same type for both cert and key
        arg = "--private-key-type=";
        arg += conn->client_cert_type();
        wget_args.push_back(arg);
    }

    arg = conn->proxy_server();
    // Override environment
    if (arg != "") {
        string proxy_url = "http://";
        proxy_url += arg;
        if (conn->proxy_port() != 80) {
            proxy_url += ":";
            proxy_url += to_string(conn->proxy_port());
        }
        proxy_url += "/";

        setenv("https_proxy", proxy_url.c_str(), 1);
    }

    if (conn->proxy_mode() == IAS_PROXY_NONE) {
        wget_args.push_back("--no-proxy");
    } else if (conn->proxy_mode() == IAS_PROXY_FORCE) {
        unsetenv("no_proxy");
    }

    char argv[4 * 1024];
    size_t sz;

    // Add instance-specific options

    if (postdata) {
        arg = "--post-file=";
        arg += tmpfile;
        wget_args.push_back(arg);
    }

    // Add the url

    wget_args.push_back(url.c_str());

    sz = wget_args.size();

    // Create the argument list

    string argvStr;
    for (int i = 0; i < sz; i++) {
        argvStr.append(wget_args[i]);
        argvStr.append(" ");
    }

    FILE* fp;
    fp = popen(argvStr.c_str(), "r");
    char result_buf[102400];
    if (NULL != fp) {
        while (fgets(result_buf, 102400, fp) != NULL) {
            sresponse.append(result_buf, strlen(result_buf));
        }
    } else {
        cerr << "Error to call system function" << endl;
    }
    pclose(fp);

    HttpResponseParser::ParseResult result;
    result = parser.parse(response, sresponse.c_str(),
        sresponse.c_str() + sresponse.length());
    rv = (result == HttpResponseParser::ParsingCompleted);

    return rv;
}
