#include "c2_server.h"

ServerConfig *load_config(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[!] Failed to open config file: %s\n", filename);
        return NULL;
    }

    json_error_t error;
    json_t *root = json_loadf(fp, 0, &error);
    fclose(fp);

    if (!root) {
        fprintf(stderr, "[!] Config JSON error: %s\n", error.text);
        return NULL;
    }

    ServerConfig *cfg = malloc(sizeof(ServerConfig));
    if (!cfg) {
        json_decref(root);
        return NULL;
    }

    memset(cfg, 0, sizeof(ServerConfig));

    const char* required_fields[] = {"valid_ua", "api_endpoint"};
    
    int missing_fields = 0;
    for (int i = 0; i < sizeof(required_fields)/sizeof(required_fields[0]); i++) {
        json_t *obj = json_object_get(root, required_fields[i]);
        if (!obj || !json_is_string(obj)) {
            fprintf(stderr, "[!] Missing or invalid field: %s\n", required_fields[i]);
            missing_fields = 1;
        }
    }
    
    if (missing_fields) {
        free(cfg);
        json_decref(root);
        return NULL;
    }

    cfg->valid_ua = _strdup(json_string_value(json_object_get(root, "valid_ua")));
    cfg->api_endpoint = _strdup(json_string_value(json_object_get(root, "api_endpoint")));
    cfg->aes_key = _strdup(json_string_value(json_object_get(root, "aes_key")));

    json_decref(root);
    return cfg;
}

void free_config(ServerConfig *cfg) {
    if (cfg) {
        free(cfg->valid_ua);
        free(cfg->api_endpoint);
        free(cfg->aes_key); // ÃÌº”’‚––
        free(cfg);
    }
}