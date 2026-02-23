#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>

#define LINE_MAX 4096
#define FIELD_MAX 1024

// Trim leading/trailing whitespace (simple)
static void trim(char *s) {
if (!s) return;

// trim leading
while (*s && isspace((unsigned char)*s)) {
memmove(s, s + 1, strlen(s));
}

// trim trailing
int n = (int)strlen(s);
while (n > 0 && isspace((unsigned char)s[n - 1])) {
s[n - 1] = '\0';
n--;
}
}

// Extract field value for a given key from a line.
// Supports:
// Key:"quoted value with spaces"
// Key:unquotedValue
static int get_field(const char *line, const char *key, char *out, size_t out_sz) {
if (!line || !key || !out || out_sz == 0) return 0;

char pattern[128];
snprintf(pattern, sizeof(pattern), "%s:", key);

const char *p = strstr(line, pattern);
if (!p) return 0;

p += strlen(pattern); // move past "Key:"
while (*p && isspace((unsigned char)*p)) p++;

// Quoted
if (*p == '"') {
p++; // skip first quote
const char *end = strchr(p, '"');
if (!end) return 0;

size_t len = (size_t)(end - p);
if (len >= out_sz) len = out_sz - 1;

strncpy(out, p, len);
out[len] = '\0';
return 1;
}

// Unquoted: read until space or end
const char *end = p;
while (*end && !isspace((unsigned char)*end)) end++;

size_t len = (size_t)(end - p);
if (len >= out_sz) len = out_sz - 1;

strncpy(out, p, len);
out[len] = '\0';
trim(out);
return 1;
}

int main(int argc, char *argv[]) {
if (argc != 4) {
fprintf(stderr, "Usage: %s <server_ip> <port> <input_file>\n", argv[0]);
return 1;
}

const char *server_ip = argv[1];
int port = atoi(argv[2]);
const char *input_file = argv[3];

if (port <= 0) {
fprintf(stderr, "Invalid port.\n");
return 1;
}

FILE *fp = fopen(input_file, "r");
if (!fp) {
perror("fopen");
return 1;
}

int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
if (sockfd < 0) {
perror("socket");
fclose(fp);
return 1;
}

struct sockaddr_in server_addr;
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons((uint16_t)port);

if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) {
fprintf(stderr, "Invalid server IP: %s\n", server_ip);
close(sockfd);
fclose(fp);
return 1;
}

char line[LINE_MAX];

while (fgets(line, sizeof(line), fp)) {
trim(line);
if (line[0] == '\0') continue;

char file_name[FIELD_MAX] = "";
char file_size[FIELD_MAX] = "";
char file_type[FIELD_MAX] = "";
char date_created[FIELD_MAX] = "";
char description[FIELD_MAX] = "";

// Pull fields (if missing, they stay empty string)
get_field(line, "File_Name", file_name, sizeof(file_name));
get_field(line, "File_Size", file_size, sizeof(file_size));
get_field(line, "File_Type", file_type, sizeof(file_type));
get_field(line, "Date_Created", date_created, sizeof(date_created));
get_field(line, "Description", description, sizeof(description));

// Build JSON using only your listed cJSON methods
cJSON *root = cJSON_CreateObject();
if (!root) {
fprintf(stderr, "cJSON_CreateObject failed\n");
continue;
}

cJSON_AddStringToObject(root, "File_Name", file_name);
cJSON_AddStringToObject(root, "File_Size", file_size);
cJSON_AddStringToObject(root, "File_Type", file_type);
cJSON_AddStringToObject(root, "Date_Created", date_created);
cJSON_AddStringToObject(root, "Description", description);

char *json_str = cJSON_PrintUnformatted(root);
if (!json_str) {
fprintf(stderr, "cJSON_PrintUnformatted failed\n");
cJSON_Delete(root);
continue;
}

if (sendto(sockfd, json_str, strlen(json_str), 0,
(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
perror("sendto");
}

free(json_str);
cJSON_Delete(root);
}

close(sockfd);
fclose(fp);
return 0;
}
