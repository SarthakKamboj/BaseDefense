#pragma once

#include <stdlib.h>
#include <stdio.h>

struct json_string_t {
    char* buffer = NULL; 
    int length = 0;
};

struct json_node_t {
    json_string_t* key = NULL;
    json_string_t* value = NULL;
    json_node_t** children = NULL;
    int num_children = 0;
};

struct json_document_t {
    json_string_t* json_path = NULL;
    json_node_t* root_node = NULL;
};

char json_peek(FILE* json_file);
void json_eat_empty_spaces(FILE* json_file);
json_document_t* json_read_document(const char* path);
void json_free_document(json_document_t* document);
void json_free_node(json_node_t* node);
void json_free_string(json_string_t* string);
json_string_t* json_create_string(const char* buffer);
char json_read_open_brace(FILE* json_file);
json_string_t* json_read_value(FILE* json_file);
json_string_t* json_read_key(FILE* json_file);