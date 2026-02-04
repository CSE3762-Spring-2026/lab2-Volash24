#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>

#define BUF_SIZE 4096

static void print_border(void) {
    printf("****************************************************************\n");
}

static void print_header(void) {
    print_border();
    printf("* %-20s * %-40s*\n", "Name", "Value");
    print_border();
}

static void print_row(const char *name, const char *value) {
    // Keep value safe if NULL
    if (!value) value = "";
    printf("* %-20s * %-40s*\n", name, value);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port.\n");
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    printf("Server listening on UDP port %d...\n", port);

    char buffer[BUF_SIZE];

    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0,
                             (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        buffer[n] = '\0';

        cJSON *root = cJSON_Parse(buffer);
        if (!root) {
            fprintf(stderr, "JSON parse error near: %s\n", cJSON_GetErrorPtr());
            continue;
        }

        // Print formatted table
        print_header();

        cJSON *item = NULL;
        cJSON_ArrayForEach(item, root) {
            if (cJSON_IsString(item)) {
                print_row(item->string, item->valuestring);
            } else if (cJSON_IsNumber(item)) {
                char numbuf[64];
                snprintf(numbuf, sizeof(numbuf), "%g", item->valuedouble);
                print_row(item->string, numbuf);
            } else if (cJSON_IsBool(item)) {
                print_row(item->string, cJSON_IsTrue(item) ? "true" : "false");
            } else {
                // Fallback for unexpected types
                print_row(item->string, "[unsupported type]");
            }
        }

        print_border();
        cJSON_Delete(root);
    }

    close(sockfd);
    return 0;
}
