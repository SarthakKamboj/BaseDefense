#include "json.h"

#include "constants.h"

char json_read_open_brace(FILE* json_file) {
    char c;
    do {
        c = (char)getc(json_file);
    }
    while (!feof(json_file) && c != '{');
    return c;
}

void json_eat_empty_spaces(FILE* json_file) {
    while (!feof(json_file)) {
        char c = json_peek(json_file);
        if (!isspace(c)) {
            break;
        }
        getc(json_file);
    }
}

char json_peek(FILE* json_file) {
    if (!feof(json_file)) {
        char c = (char)getc(json_file);
        ungetc(c, json_file);
        return c;
    }
    return 0;
}

json_string_t* json_read_value(FILE* json_file) {
    char buffer[256]{};
    if (json_peek(json_file) == '\"') {
        getc(json_file);
    }
    int value_len = 0;
    while (true) {
        char c = json_peek(json_file);
        bool ended = isspace(c) || c == ',' || c == '\"';
        if (ended) break;
        *(buffer + value_len) = c;
        value_len++;
        getc(json_file);
    }
    json_eat_empty_spaces(json_file);
    if (json_peek(json_file) == '\"') {
        getc(json_file);
    }
    json_eat_empty_spaces(json_file);
    if (json_peek(json_file) == ',') {
        getc(json_file);
    }
    json_string_t* value = json_create_string(buffer);
    return value;
}

json_string_t* json_read_key(FILE* json_file) {
    char key_name[64]{};
    int key_len = 0;

    // key could have quotation marks
    if (json_peek(json_file) == '\"') {
        getc(json_file);
    }

    while (true) {
        char c = json_peek(json_file);
        if (!isspace(c) && c != ':' && c != '\"') {
            *(key_name + key_len) = c;
            key_len++;
            getc(json_file);
            continue;
        }
        break;
    }
    json_eat_empty_spaces(json_file);

    // key could have quotation marks
    if (json_peek(json_file) == '\"') {
        getc(json_file);
    }
    json_string_t* key_string = json_create_string(key_name); 
    return key_string;
}

static json_node_t* json_read_curly_brace_section(FILE* json_file) {
    json_node_t* node = (json_node_t*)malloc(sizeof(json_node_t));
    json_eat_empty_spaces(json_file);
    char opening_curly_brace = getc(json_file);

    game_assert_msg(opening_curly_brace == '{', "opening curly brace was not {");

    node->children = NULL;
    node->num_children = 0;
    node->value = NULL;
    node->key = NULL;

    // // may be an empty curly brace section
    // if (json_peek(json_file) == '}') {
    //     getc(json_file);
        
    // } 

    json_eat_empty_spaces(json_file);
    while (json_peek(json_file) != '}') {

        json_string_t* child_key_string = json_read_key(json_file);

        node->num_children++;
        if (node->num_children == 1) {
            node->children = (json_node_t**)malloc(sizeof(json_node_t*));
        } else {
            node->children = (json_node_t**)realloc(node->children, node->num_children * sizeof(json_node_t*));
        }

        json_eat_empty_spaces(json_file);
        // while (json_peek(json_file) != ':') {
        //     getc(json_file);
        // }
        char colon = getc(json_file);
        game_assert_msg(colon == ':', "colon not found");
        json_eat_empty_spaces(json_file);

        // get to the starting character of the value
        char c = json_peek(json_file);
        json_node_t* child_node = NULL;
        if (isalnum(c) || c == '\"') {
            // reading an actual value and not a sub-dictionary
            child_node = (json_node_t*)malloc(sizeof(json_node_t));
            child_node->children = NULL;
            child_node->num_children = 0;
            child_node->value = json_read_value(json_file); 
        } else if (c == '{') {
            // see an opening curly brace
            child_node = json_read_curly_brace_section(json_file);
        } else {
            // see and opening square brace
            getc(json_file);

            child_node = (json_node_t*)malloc(sizeof(json_node_t));
            child_node->children = NULL;
            child_node->num_children = 0;
            child_node->key = NULL;
            child_node->value = NULL;
            while (json_peek(json_file) != ']') {
                child_node->num_children++;
                if (child_node->num_children == 1) {
                    child_node->children = (json_node_t**)malloc(sizeof(json_node_t*));
                } else {
                    child_node->children = (json_node_t**)realloc(child_node->children, child_node->num_children * sizeof(json_node_t*));
                }
                json_node_t* array_element = json_read_curly_brace_section(json_file);
                child_node->children[child_node->num_children - 1] = array_element;
                json_eat_empty_spaces(json_file);
                if (json_peek(json_file) == ',') {
                    getc(json_file);
                    json_eat_empty_spaces(json_file);
                }
            }
            char ending_bracket = getc(json_file);
            game_assert_msg(ending_bracket == ']', "ending bracket not found at this point");
        }

        child_node->key = child_key_string;
        node->children[node->num_children-1] = child_node;
        json_eat_empty_spaces(json_file);
    }

    // read the } character
    getc(json_file);

    json_eat_empty_spaces(json_file);
    if (json_peek(json_file) == ',') {
        getc(json_file);
        json_eat_empty_spaces(json_file);
    }

    // char closing_brace = '}';
    // json_eat_empty_spaces(json_file);
    // while (json_peek(json_file) != closing_brace) {
    //     // expecting children nodes
    //     json_node_t* child_node = json_read_curly_brace_section(json_file, true);
    //     if (!node->children) {
    //         node->children = (json_node_t**)malloc(sizeof(json_node_t*));
    //         node->children[0] = child_node;
    //         node->num_children = 1;
    //     } else {
    //         node->num_children++;
    //         node->children = (json_node_t**)realloc(node->children, node->num_children * sizeof(json_node_t*));
    //         node->children[node->num_children-1] = child_node;
    //     }
    //     json_eat_empty_spaces(json_file);
    //     // this should be a comma
    //     getc(json_file);
    //     json_eat_empty_spaces(json_file);
    // }
    // // reached end of node, next character should be closing brace
    // getc(json_file);
    return node;
}

json_string_t* json_create_string(const char* buffer) {
    json_string_t* str = (json_string_t*)malloc(sizeof(json_string_t));
    str->buffer = (char*)calloc(strlen(buffer) + 1, sizeof(char));
    game_assert_msg(str->buffer, "buffer for json string not made");
    memcpy(str->buffer, buffer, strlen(buffer));
    str->length = strlen(str->buffer);
    return str;
}

json_document_t* json_read_document(const char* path) {
    FILE* json_file = fopen(path, "r");
    if (!json_file) {
        char error[256]{};
        sprintf("%s not found\n", path);
        game_assert_msg(json_file, error);
    }

    json_document_t* document = (json_document_t*)malloc(sizeof(json_document_t));
    document->json_path = json_create_string(path);
    document->root_node = json_read_curly_brace_section(json_file);
    document->root_node->key = json_create_string("json_parser_root");

    return document;
}

void json_free_string(json_string_t* string) {
    if (string == NULL || string->buffer == NULL) return;
    free(string->buffer);
    free(string);
}

void json_free_node(json_node_t* node) {
    if (node == NULL) return;
    json_free_string(node->key);
    json_free_string(node->value);
    if (node->num_children > 0) {
        for (int i = 0; i < node->num_children; i++) {
            json_free_node(node->children[i]);
        }
        free(node->children);
    }
    free(node);
}

void json_free_document(json_document_t* document) {
    json_free_string(document->json_path);
    json_free_node(document->root_node);
    free(document);
}