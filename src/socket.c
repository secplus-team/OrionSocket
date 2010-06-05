#include "socket.h"
#include "err.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// Berkeley Sockets
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

void *orionRealloc(void *ptr, size_t size)
{
    if (ptr)
        return realloc(ptr, size);
    else
        return malloc(size);
}

int orionTCPConnect(address addr)
{
    int sockfd;
    
    struct hostent *addr_host;
    struct sockaddr_in target;
    char* ip = NULL;
    addr_host = gethostbyname(addr.domain);
    if (!addr_host)
    {
        switch (h_errno)
        {
        case HOST_NOT_FOUND:
            fprintf(stderr, "[ERROR] gethostbyname(): Host não encontrado.\n");
            break;
        case NO_ADDRESS:
            fprintf(stderr, "[ERROR] gethostbyname(): O nome solicitado é válido mas não possui um endereço de internet...\n");
            break;
        case TRY_AGAIN:
            fprintf(stderr, "[ERROR] gethostbyname(): Problemas ao obter o endereço do Host!!!\n"
                            "Voce pode tentar novamente...\n"
                            "Esse problema é raro...\n"
                            "Não posso lhe ajudar muito...\n"
                            "Esse erro ocorreu na chamada da funcao gethostbyname()\n"
                            "Essa funcao utiliza um banco de dados locais de resolucao de nomes\n"
                            "para encontrar o dominio.\n"
                            "Verifique se possui os arquivos default do resolver, como:\n"
                            "- /etc/hosts\n"
                            "- /etc/networks\n"
                            "- /etc/protocols\n"
                            "- /etc/services\n"
                            "...\n"
                            "Verifique o header '/usr/include/netdb.h'\n");
            break;
        case NO_RECOVERY:
            fprintf(stderr, "[ERROR] gethostbyname(): Erro irrecuperável... Muito explicativo não é? ¬¬\n");
            break;
        default:
            fprintf(stderr, "[ERROR] gethostbyname(): Uwnknow Error Code: %d\n", h_errno);
            break;    
        }
        
        return -1;        
    }
    
    // Converte um endereço de internet (struct in_addr *) em ASCII
    ip = inet_ntoa(*(struct in_addr *)addr_host->h_addr);
    if (*ip == -1)
    {
        fprintf(stderr, "[ERROR] inet_ntoa(): Não foi possivel obter o endereco IP.\n");
        return -1;
    }
    // Cria o socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "[ERROR] Erro ao criar socket.\n");
        switch (errno)
        {
        case EAFNOSUPPORT:
            fprintf(stderr, ">> O endereco especificado na 'Familia de Enderecos' não pode"
                            " ser usado com esse socket...\n");
            break;
        case EMFILE:
            fprintf(stderr, ">> A Tabela de descritores por processo está cheia.\n");
            break;
        case ENOBUFS:
            fprintf(stderr, "O sistema não possui recursos suficientes para completar a chamada...\n");
            break;
        case ESOCKTNOSUPPORT:
            fprintf(stderr, "O socket especificado na familia de enderecos não é siportado.\n");
            break;
        }
        
        return -1;
    }
    
    target.sin_family = AF_INET;
    target.sin_port = htons(addr.port);
    
    // Converte om endereco de internet de ASCII para binário
    if (inet_pton(AF_INET, ip, &target.sin_addr) < 0)
    {
        perror("[ERROR] Erro ao setar o endereço remoto.\n");
        return errno;
    }
    
    memset(target.sin_zero, '\0', 8);
    
    if (connect(sockfd, (struct sockaddr *)&target, sizeof(struct sockaddr)) == -1)
    {
        perror("[ERROR] Erro ao conectar no host.\n");
        
        switch (errno)
        {
        case EADDRINUSE:
            fprintf(stderr, ">> O endereco IP já está em uso...\n");
            break;
        case EADDRNOTAVAIL:
            fprintf(stderr, ">> O endereco especificado não está disponivel nesta maquina local.\n");
            break;
        case EAFNOSUPPORT:
            fprintf(stderr, ">> O endereco na Familia de Enderecos não pode ser usado com esse socket...\n");
            break;
        case EALREADY:
            fprintf(stderr, ">> O socket está setado com O_NONBLOCK  ou O_NDLAY e a conexão anterior ainda não foi completada.\n");
            break;
        case EINTR:
            fprintf(stderr, "The attempt to establish a connection was interrupted by delivery of a signal that was caught; the connection will be established asynchronously.\n");
            break;
        case EACCES:
            fprintf(stderr, "Permissão de busca negada no componente ou acesso de escrita negado no socket.\n");
            break;
        case ENOBUFS:
            fprintf(stderr, "O sistema não possui memória suficiente para a estrutura de dados interna.\n");
            break;
        case EOPNOTSUPP:
            fprintf(stderr, "O socket especificado no parametro 'socket' não suporta a chamada connect.\n");
            break;
        default:
            fprintf(stderr, "Estou com preguiça de escrever um mensagem bonitinha para TODOS os erros que voce comete...\n"
                            "Por favor, crie vergonha na cara e procure pelo codigo de erro %d no arquivo /usr/include/sys/socket.h.\n", errno);
            break;        
        }
        
        return -1;
    }
    
    return sockfd;
}

const char* _getStrMethod(_uint8 method)
{
    switch (method)
    {
    case METHOD_GET:
        return "GET";
    case METHOD_POST:
        return "POST";
    case METHOD_TRACE:
        return "TRACE";
    case METHOD_PUT:
        return "PUT";
    case METHOD_DELETE:
        return "DELETE";
    default:
        return "UNKNOWN";
    }
}

// Monta a Requisição HTTP a partir da estrutura httpRequest
// @param httpRequest*      req
// @param char *            reqBuffer
void assemblyHttpRequest(httpRequest* req, char* reqBuffer)
{
    _uint32 size = 0, i;
    char temp[10];
    bzero(temp, 10);
    
    strcpy(reqBuffer, _getStrMethod(req->method));
    strcat(reqBuffer, " ");
    strcat(reqBuffer, req->path);
    strcat(reqBuffer, " ");
    strcat(reqBuffer, HTTP_PROTOCOL);
    strcat(reqBuffer, "\n");
    
    for (i = 0; i < req->headerLen; i++)
    {
        strcat(reqBuffer, req->header[i].name);
        strcat(reqBuffer, ": ");
        strcat(reqBuffer, req->header[i].value);
        strcat(reqBuffer, "\n");
    }
    
    if (req->method == METHOD_POST)
    {
        strcat(reqBuffer, "Content-Length: ");
        
        for (i = 0; i < req->paramLen; i++)
        {
            size += strlen(req->param[i].name);
            size += strlen(req->param[i].value);
            size += strlen("=");
            if (i != req->paramLen-1)
                size += strlen("&");
        }       
        
        sprintf(temp, "%d", size);
        strcat(reqBuffer, temp);
        
        for (i = 0; i < req->paramLen; i++)
        {
            strcat(reqBuffer, req->param[i].name);
            strcat(reqBuffer, "=");
            strcat(reqBuffer, req->param[i].value);
            if (i != req->paramLen-1)
                strcat(reqBuffer, "&");
        }
    }
    
    strcat(reqBuffer, "\n");
}

uint8_t setHttpHeader(httpRequest *req, const char* name, const char* value)
{
    _uint8 len;
        
    len = req->headerLen;
    
    req->header = (nameValue *) orionRealloc(req->header, sizeof(nameValue)*(len+1));
    if (!req->header)
    {
        fprintf(stderr, "[ERROR] Erro ao alocar memória.\n");
        return ORIONSOCKET_ERR_ALLOC;
    }
    
    req->header[len].name = (char *) malloc(sizeof(char) * strlen(name) + 1);
    req->header[len].value = (char *) malloc(sizeof(char) * strlen(value) + 1);
    if (!req->header[len].name || !req->header[len].value)
    {
        fprintf(stderr, "[ERROR] Erro ao alocar memória.\n");
        return ORIONSOCKET_ERR_ALLOC;
    }
    strcpy(req->header[len].name, name);
    strcpy(req->header[len].value, value);
    
    req->headerLen++;
    
    return ORIONSOCKET_OK;    
}

// Estabelece uma conexão única no host passado em httpRequest *
// Retorna o conteudo da página.
// 
_uint8 orionHTTPRequest(httpRequest *req, char** response)
{
    int sockfd, n = 0;
    char reqBuffer[HTTP_REQUEST_MAXLENGTH], temp[HTTP_BIG_RESPONSE];
    char *localBuffer = NULL;
    
    memset(reqBuffer, '\0', sizeof(char) * HTTP_REQUEST_MAXLENGTH);
    memset(temp, '\0', sizeof(char) * HTTP_RESPONSE_LENGTH);
    
    assemblyHttpRequest(req, reqBuffer);
    
    printf("%s\n", reqBuffer);
    
    sockfd = orionTCPConnect(*(req->victim));
    
    if (sockfd < 0)
    {
        return errno;
    }
    
    if (send(sockfd, reqBuffer, strlen(reqBuffer), 0) < 0)
    {
        perror("[ERROR] Erro ao enviar requisição.\n");
        close(sockfd);
        return errno;
    }
    
    localBuffer = (char *) malloc(sizeof(char) * 1);
    localBuffer[0] = '\0';
    
    while((n = read(sockfd, temp, HTTP_BIG_RESPONSE-1)) > 0)
    {
        localBuffer = (char *) orionRealloc(localBuffer, n + strlen(localBuffer) + 1);
        temp[n-1] = '\0';
        strcat(localBuffer, temp);
        bzero(temp, HTTP_BIG_RESPONSE);       
    }
    
    close(sockfd);
    
    *response = localBuffer;
    
    return ORIONSOCKET_OK;
}

void httpRequestCleanup(httpRequest *req)
{
    int i;
    for (i = 0; i < req->headerLen; i++)
    {
        free(req->header[i].name);
        free(req->header[i].value);
    }
    
    free(req->header);
    
    free(req);
}