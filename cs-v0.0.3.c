#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h> 
#include <stddef.h> 
#include <limits.h> // For INT_MAX, INT_MIN in string_to_int

// --- Yapılandırma ---
#define MAX_SOURCE_SIZE 10240
#define MAX_TOKENS 8192 
#define MAX_IDENT_LEN 64
#define MAX_STRING_LEN 256
#define MAX_VARIABLES 512 
#define MAX_LOOP_NESTING 10
#define MAX_ARRAY_DIMENSIONS 1 
#define MAX_IMPORTS 10           
#define MAX_FILENAME_LEN 256    
#define MAX_FUNCTIONS 100
#define MAX_PARAMETERS 10
#define MAX_CALL_STACK_DEPTH 100
#define MAX_SCOPE_DEPTH 100 


// --- Token Türleri ---
typedef enum {
    TOKEN_EOF, TOKEN_ERROR,
    TOKEN_IDENTIFIER, TOKEN_INT_LITERAL, TOKEN_STRING_LITERAL, TOKEN_FLOAT_LITERAL,
    TOKEN_VAR, TOKEN_INT_TYPE, TOKEN_STRING_TYPE, TOKEN_FLOAT_TYPE, TOKEN_BOOLEAN_TYPE, TOKEN_VOID_TYPE, 
    TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_FOR,
    TOKEN_OUT, TOKEN_DISPLAY, TOKEN_USER, 
    TOKEN_TRUE, TOKEN_FALSE,
    TOKEN_FUN, TOKEN_RETURN, 
    TOKEN_BREAK, TOKEN_CONTINUE,
    TOKEN_IMPORT, 
    TOKEN_ASSIGN, TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE, TOKEN_MODULO,
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_LBRACKET, TOKEN_RBRACKET, 
    TOKEN_COLON, TOKEN_SEMICOLON, TOKEN_DOT, TOKEN_COMMA,
    TOKEN_GT, TOKEN_LT, TOKEN_GTE, TOKEN_LTE, TOKEN_EQ, TOKEN_NEQ,
    TOKEN_AND, TOKEN_OR, TOKEN_NOT
} TokenType;

const char* token_type_names[] = {
    "EOF", "ERROR", "IDENTIFIER", "INT_LITERAL", "STRING_LITERAL", "FLOAT_LITERAL",
    "VAR", "INT_TYPE", "STRING_TYPE", "FLOAT_TYPE", "BOOLEAN_TYPE", "VOID_TYPE", 
    "IF", "ELSE", "WHILE", "FOR",
    "OUT", "DISPLAY", "USER",
    "TRUE", "FALSE",
    "FUN", "RETURN",
    "BREAK", "CONTINUE",
    "IMPORT", 
    "ASSIGN", "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "MODULO",
    "LPAREN", "RPAREN", "LBRACE", "RBRACE",
    "LBRACKET", "RBRACKET",
    "COLON", "SEMICOLON", "DOT", "COMMA",
    "GT", "LT", "GTE", "LTE", "EQ", "NEQ",
    "AND", "OR", "NOT"
};

// --- Değişken Tipi ve Değer Yapıları ---
typedef enum {
    VAL_NULL, VAL_INT, VAL_FLOAT, VAL_STRING, VAL_BOOLEAN, VAL_ARRAY_REF
} ValueType;

typedef enum {
    VAR_NULL_TYPE, 
    VAR_INT, VAR_STRING, VAR_FLOAT, VAR_BOOLEAN,
    VAR_ARRAY,
    VAR_VOID 
} VarType;

const char* var_type_names_debug[] = {
    "NULL_TYPE", "INT", "STRING", "FLOAT", "BOOLEAN", "ARRAY", "VOID"
};

typedef struct {
    TokenType type;
    char lexeme[MAX_STRING_LEN]; 
    int int_value;
    char string_value[MAX_STRING_LEN]; 
    double float_value;
    bool bool_value; 
    int line;
} Token;

struct Variable; 
typedef struct Value {
    ValueType type;
    union {
        int int_val;
        double float_val;
        char string_val[MAX_STRING_LEN];
        bool bool_val;
        struct Variable* array_var; 
    } as;
} Value;

typedef struct Variable {
    char name[MAX_IDENT_LEN];
    VarType type; 
    bool is_defined;
    bool is_loop_var; 
    int scope_level; 
    
    union {
        int int_value;
        char string_value[MAX_STRING_LEN];
        double float_value;
        bool bool_value;
        struct {
            void* data; 
            VarType element_type; 
            int size;             
        } array;
    } value;
} Variable;

typedef struct {
    char name[MAX_IDENT_LEN];
    VarType type; 
} Parameter;

typedef struct {
    char name[MAX_IDENT_LEN];
    Parameter params[MAX_PARAMETERS];
    int num_params;
    VarType return_type; 
    int body_start_token_idx;
    // int body_end_token_idx; // Potentially useful for faster skipping in 2nd pass
} FunctionDefinition;

typedef struct {
    int return_address_token_idx;     
    int symbol_table_scope_start_idx; 
    int prev_loop_depth;              
    int prev_for_loop_var_stack_ptr;  
    const FunctionDefinition* func_def; 
} CallFrame;


// --- Global Yorumlayıcı Durumu ---
char source_code[MAX_SOURCE_SIZE];
Token tokens[MAX_TOKENS]; 
int num_tokens = 0;
int current_token_idx = 0;
int current_line = 1;

Variable symbol_table[MAX_VARIABLES];
int num_variables = 0;

FunctionDefinition function_table[MAX_FUNCTIONS];
int num_functions = 0;

CallFrame call_stack[MAX_CALL_STACK_DEPTH];
int call_stack_ptr = -1; 

int scope_stack[MAX_SCOPE_DEPTH]; 
int scope_stack_ptr = -1;       

char for_loop_vars_stack[MAX_LOOP_NESTING][MAX_IDENT_LEN];
int for_loop_var_stack_ptr = -1; 

int loop_depth = 0; 

Value g_return_value_holder; 
bool g_return_flag = false;    

char imported_files[MAX_IMPORTS][MAX_FILENAME_LEN];
int num_imported_files = 0;
char current_file_path_for_errors[MAX_FILENAME_LEN];
char* g_importer_saved_source_code = NULL;
Token* g_importer_saved_tokens = NULL;


// --- Fonksiyon İleri Bildirimleri ---
void parse_statement_list(bool execute, bool* break_flag, bool* continue_flag, bool in_function_body);
void parse_statement(bool execute, bool* break_flag, bool* continue_flag, bool in_function_body);
void parse_block(bool execute, bool* break_flag, bool* continue_flag, bool in_function_body);
Value evaluate_expression(bool execute);
Value parse_assignment_rhs(VarType expected_lhs_type, bool execute);
void parse_import_statement(bool execute); 
void interpret_current_file_tokens(const char* filepath_display_name);
void parse_fun_declaration(); 
VarType parse_type_specifier(); 
void error(const char* message); 
// Value execute_function_call(const FunctionDefinition* func_def, Value args[], int num_args_passed, bool execute_flag); // OLD
Value execute_function_call(const FunctionDefinition* func_def, Value args[], int num_args_passed);


// --- Kapsam Yönetimi Yardımcıları ---
void enter_scope() {
    if (scope_stack_ptr + 1 >= MAX_SCOPE_DEPTH) error("Maksimum kapsam derinliği aşıldı.");
    scope_stack_ptr++;
    scope_stack[scope_stack_ptr] = num_variables; 
}

void exit_scope() {
    if (scope_stack_ptr < 0) {
        // This can happen if exit_scope is called more times than enter_scope,
        // e.g. after an error during function call setup.
        // It's not necessarily a fatal interpreter bug in itself if error handling cleans up.
        // For now, let's just ensure num_variables doesn't go below zero.
        // fprintf(stderr, "Uyarı: exit_scope çağrıldığında kapsam yığını zaten boş.\n");
        return;
    }
    int scope_start_idx = scope_stack[scope_stack_ptr];
    for (int i = num_variables - 1; i >= scope_start_idx; i--) {
        if (symbol_table[i].type == VAR_ARRAY && symbol_table[i].value.array.data) {
            free(symbol_table[i].value.array.data);
            symbol_table[i].value.array.data = NULL;
        }
    }
    num_variables = scope_start_idx; 
    scope_stack_ptr--;
}

int get_current_scope_level() {
    return scope_stack_ptr; 
}

// --- Hata Yönetimi ---
void error(const char* message) {
    fprintf(stderr, "Hata (dosya: %s, satır %d, token %d '%s'): %s\n",
            current_file_path_for_errors,
            (current_token_idx < num_tokens && current_token_idx >=0) ? tokens[current_token_idx].line : current_line,
            current_token_idx,
            (current_token_idx < num_tokens && current_token_idx >=0) ? tokens[current_token_idx].lexeme : "YOK",
            message);
    
    if (g_importer_saved_source_code) { free(g_importer_saved_source_code); g_importer_saved_source_code = NULL; }
    if (g_importer_saved_tokens) { free(g_importer_saved_tokens); g_importer_saved_tokens = NULL; }
    
    // Genel bir temizleme, olası tüm dizi belleklerini serbest bırakmaya çalışır
    // This might be redundant if scope exit handles it, but good for abrupt termination.
    for (int i = 0; i < num_variables; ++i) {
        if (symbol_table[i].type == VAR_ARRAY && symbol_table[i].value.array.data != NULL) {
            free(symbol_table[i].value.array.data);
            symbol_table[i].value.array.data = NULL;
        }
    }
    exit(1);
}

// --- Tip Yardımcıları --- 
const char* value_type_to_string(ValueType type) {
    switch(type) {
        case VAL_INT: return "tamsayı"; case VAL_FLOAT: return "ondalıklı sayı";
        case VAL_STRING: return "metin"; case VAL_BOOLEAN: return "mantıksal";
        case VAL_ARRAY_REF: return "dizi referansı"; case VAL_NULL: return "boş";
        default: return "bilinmeyen değer tipi";
    }
}
const char* var_type_to_string_user(VarType type) { 
    switch(type) {
        case VAR_INT: return "int"; case VAR_STRING: return "string";
        case VAR_FLOAT: return "float"; case VAR_BOOLEAN: return "boolean";
        case VAR_ARRAY: return "array"; case VAR_VOID: return "void";
        case VAR_NULL_TYPE: return "null_type_internal";
        default: return "bilinmeyen değişken tipi";
    }
}
size_t get_sizeof_element_type(VarType type) {
    switch (type) {
        case VAR_INT: return sizeof(int); case VAR_FLOAT: return sizeof(double);
        case VAR_BOOLEAN: return sizeof(bool); case VAR_STRING: return MAX_STRING_LEN; 
        default: error("get_sizeof_element_type: Desteklenmeyen veya uygulanamayan dizi eleman tipi."); return 0;
    }
}

// --- Lexer (Token Üretici) ---
bool is_keyword(const char* s, const char* keyword) { return strcmp(s, keyword) == 0; }
Token create_token(TokenType type, const char* lexeme_val) {
    Token t; t.type = type;
    if (lexeme_val) { strncpy(t.lexeme, lexeme_val, MAX_STRING_LEN - 1); t.lexeme[MAX_STRING_LEN - 1] = '\0'; }
    else { t.lexeme[0] = '\0'; }
    t.line = current_line; t.int_value = 0; t.float_value = 0.0; t.bool_value = false;
    strcpy(t.string_value, ""); 
    return t;
}

void tokenize() { 
    int i = 0; num_tokens = 0; current_line = 1;
    while (source_code[i] != '\0' && num_tokens < MAX_TOKENS) {
        if (isspace(source_code[i])) { if (source_code[i] == '\n') current_line++; i++; continue; }
        if ((source_code[i] == '/' && source_code[i+1] == '/')) { while (source_code[i]!='\n'&&source_code[i]!='\0')i++; if(source_code[i]=='\n'){i++; current_line++;} continue; }
        if (source_code[i] == '#') { while (source_code[i]!='\n'&&source_code[i]!='\0')i++; if(source_code[i]=='\n'){i++; current_line++;} continue; }
        if (source_code[i] == '/' && source_code[i+1] == '*') { 
            i+=2; int csl=current_line; while(source_code[i]!='\0'&&(source_code[i]!='*'||source_code[i+1]!='/')){if(source_code[i]=='\n')current_line++;i++;}
            if(source_code[i]=='*'&&source_code[i+1]=='/'){i+=2;}else{current_line=csl;error("Kapatılmamış blok yorumu");} continue;
        }
        char lexeme_buffer[MAX_STRING_LEN]; int k = 0;
        if (isalpha(source_code[i]) || source_code[i] == '_') {
            while (isalnum(source_code[i]) || source_code[i] == '_') { if(k<MAX_IDENT_LEN-1)lexeme_buffer[k++]=source_code[i++];else {i++; error("Tanımlayıcı çok uzun.");}} // Added error for too long identifier
            lexeme_buffer[k]='\0'; Token t=create_token(TOKEN_IDENTIFIER,lexeme_buffer);
            if (is_keyword(lexeme_buffer,"var"))t.type=TOKEN_VAR; else if(is_keyword(lexeme_buffer,"int"))t.type=TOKEN_INT_TYPE;
            else if(is_keyword(lexeme_buffer,"string"))t.type=TOKEN_STRING_TYPE; else if(is_keyword(lexeme_buffer,"float"))t.type=TOKEN_FLOAT_TYPE;
            else if(is_keyword(lexeme_buffer,"boolean"))t.type=TOKEN_BOOLEAN_TYPE; else if(is_keyword(lexeme_buffer,"void"))t.type=TOKEN_VOID_TYPE; 
            else if(is_keyword(lexeme_buffer,"if"))t.type=TOKEN_IF; else if(is_keyword(lexeme_buffer,"else"))t.type=TOKEN_ELSE;
            else if(is_keyword(lexeme_buffer,"while"))t.type=TOKEN_WHILE; else if(is_keyword(lexeme_buffer,"for"))t.type=TOKEN_FOR;
            else if(is_keyword(lexeme_buffer,"out"))t.type=TOKEN_OUT; else if(is_keyword(lexeme_buffer,"display"))t.type=TOKEN_DISPLAY;
            else if(is_keyword(lexeme_buffer,"user"))t.type=TOKEN_USER; else if(is_keyword(lexeme_buffer,"true")){t.type=TOKEN_TRUE;t.bool_value=true;}
            else if(is_keyword(lexeme_buffer,"false")){t.type=TOKEN_FALSE;t.bool_value=false;} else if(is_keyword(lexeme_buffer,"fun"))t.type=TOKEN_FUN;
            else if(is_keyword(lexeme_buffer,"return"))t.type=TOKEN_RETURN; else if(is_keyword(lexeme_buffer,"break"))t.type=TOKEN_BREAK;
            else if(is_keyword(lexeme_buffer,"continue"))t.type=TOKEN_CONTINUE; else if(is_keyword(lexeme_buffer,"import"))t.type=TOKEN_IMPORT;
            tokens[num_tokens++]=t; continue;
        }
        if (isdigit(source_code[i])||(source_code[i]=='.'&&isdigit(source_code[i+1]))){
            bool isf=false; k=0; if(source_code[i]=='.'){isf=true;if(k<MAX_STRING_LEN-1)lexeme_buffer[k++]=source_code[i++];else {i++; error("Sayı literali çok uzun.");}}
            while(isdigit(source_code[i])){if(k<MAX_STRING_LEN-1)lexeme_buffer[k++]=source_code[i++];else {i++; error("Sayı literali çok uzun.");}}
            if(source_code[i]=='.'){if(!isf){isf=true;if(k<MAX_STRING_LEN-1)lexeme_buffer[k++]=source_code[i++];else {i++; error("Sayı literali çok uzun.");}}
            while(isdigit(source_code[i])){if(k<MAX_STRING_LEN-1)lexeme_buffer[k++]=source_code[i++];else {i++; error("Sayı literali çok uzun.");}}}
            lexeme_buffer[k]='\0'; Token t; if(isf){t=create_token(TOKEN_FLOAT_LITERAL,lexeme_buffer);t.float_value=atof(lexeme_buffer);}
            else{t=create_token(TOKEN_INT_LITERAL,lexeme_buffer);t.int_value=atoi(lexeme_buffer);} tokens[num_tokens++]=t; continue;
        }
        if(source_code[i]=='"'){
            i++;k=0; while(source_code[i]!='"'&&source_code[i]!='\0'){ // Removed k < MAX_STRING_LEN -1 to allow error for too long string
                if (k >= MAX_STRING_LEN -1) error("String literali çok uzun.");
                if(source_code[i]=='\\'&&source_code[i+1]!='\0'){i++;
                    switch(source_code[i]){case'n':lexeme_buffer[k++]='\n';break; case't':lexeme_buffer[k++]='\t';break;
                        case'"':lexeme_buffer[k++]='"';break; case'\\':lexeme_buffer[k++]='\\';break;
                        default:lexeme_buffer[k++]=source_code[i];break;} i++;
                }else{lexeme_buffer[k++]=source_code[i++];}} lexeme_buffer[k]='\0';
                if(source_code[i]=='"')i++;else error("Kapatılmamış string literali"); Token t=create_token(TOKEN_STRING_LITERAL,NULL);
                strncpy(t.string_value,lexeme_buffer,MAX_STRING_LEN-1);t.string_value[MAX_STRING_LEN-1]='\0';
            if(strlen(lexeme_buffer)>MAX_IDENT_LEN-3){snprintf(t.lexeme,MAX_STRING_LEN,"\"%.*s...\"",MAX_IDENT_LEN-6,lexeme_buffer);}
            else{snprintf(t.lexeme,MAX_STRING_LEN,"\"%s\"",lexeme_buffer);} tokens[num_tokens++]=t; continue;
        }
        k=0; lexeme_buffer[k++]=source_code[i]; TokenType type=TOKEN_ERROR;
        switch(source_code[i]){
            case'=':if(source_code[i+1]=='='){type=TOKEN_EQ;i++;lexeme_buffer[k++]='=';}else type=TOKEN_ASSIGN;break;
            case'+':type=TOKEN_PLUS;break; case'-':type=TOKEN_MINUS;break; case'*':type=TOKEN_MULTIPLY;break;
            case'/':type=TOKEN_DIVIDE;break; case'%':type=TOKEN_MODULO;break; case'(':type=TOKEN_LPAREN;break;
            case')':type=TOKEN_RPAREN;break; case'{':type=TOKEN_LBRACE;break; case'}':type=TOKEN_RBRACE;break;
            case'[':type=TOKEN_LBRACKET;break; case']':type=TOKEN_RBRACKET;break; case':':type=TOKEN_COLON;break;
            case';':type=TOKEN_SEMICOLON;break; case'.':type=TOKEN_DOT;break; case',':type=TOKEN_COMMA;break;
            case'>':if(source_code[i+1]=='='){type=TOKEN_GTE;i++;lexeme_buffer[k++]='=';}else type=TOKEN_GT;break;
            case'<':if(source_code[i+1]=='='){type=TOKEN_LTE;i++;lexeme_buffer[k++]='=';}else type=TOKEN_LT;break;
            case'!':if(source_code[i+1]=='='){type=TOKEN_NEQ;i++;lexeme_buffer[k++]='=';}else type=TOKEN_NOT;break;
            case'&':if(source_code[i+1]=='&'){type=TOKEN_AND;i++;lexeme_buffer[k++]='&';}else error("Beklenmeyen '&', '&&' mi demek istediniz?");break;
            case'|':if(source_code[i+1]=='|'){type=TOKEN_OR;i++;lexeme_buffer[k++]='|';}else error("Beklenmeyen '|', '||' mi demek istediniz?");break;
            default:sprintf(lexeme_buffer,"Bilinmeyen karakter: '%c'",source_code[i]);error(lexeme_buffer);
        } lexeme_buffer[k]='\0'; tokens[num_tokens++]=create_token(type,lexeme_buffer); i++;
    } tokens[num_tokens++]=create_token(TOKEN_EOF,"EOF");
}

// --- Sembol Tablosu Yönetimi --- 
Variable* find_variable(const char* name) { 
    for (int i = num_variables - 1; i >= 0; --i) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return &symbol_table[i];
        }
    }
    return NULL;
}
Variable* declare_variable(const char* name, VarType type, bool is_loop_var_decl, VarType array_element_type_param, int array_size_param) {
    if (num_variables >= MAX_VARIABLES) error("Çok fazla değişken tanımlandı (sembol tablosu dolu)");
    
    int current_scope_start_idx = (scope_stack_ptr >= 0) ? scope_stack[scope_stack_ptr] : 0;
    for (int i = num_variables - 1; i >= current_scope_start_idx; --i) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            char err[MAX_IDENT_LEN + 100];
            sprintf(err, "'%s' adlı değişken bu kapsamda zaten tanımlı.", name);
            error(err);
        }
    }
    
    Variable* new_var = &symbol_table[num_variables];
    strncpy(new_var->name, name, MAX_IDENT_LEN - 1); new_var->name[MAX_IDENT_LEN-1] = '\0';
    new_var->type = type; new_var->is_defined = false; new_var->is_loop_var = is_loop_var_decl;
    new_var->scope_level = get_current_scope_level();
    
    if (type == VAR_ARRAY) {
        if (array_element_type_param==VAR_NULL_TYPE||array_size_param<=0)error("Geçersiz dizi eleman tipi/boyutu.");
        new_var->value.array.element_type = array_element_type_param; new_var->value.array.size = array_size_param;
        size_t element_size = get_sizeof_element_type(array_element_type_param);
        if (element_size == 0) error("Dizi için eleman boyutu sıfır olamaz."); // Should be caught by get_sizeof_element_type
        new_var->value.array.data = calloc(array_size_param, element_size);
        if(!new_var->value.array.data)error("Dizi için bellek ayrılamadı."); new_var->is_defined=true; // Array itself is defined, elements are default-initialized
        if(array_element_type_param==VAR_STRING){for(int k_arr=0;k_arr<array_size_param;k_arr++){((char*)new_var->value.array.data+k_arr*MAX_STRING_LEN)[0]='\0';}}
    }
    
    if (is_loop_var_decl) { 
        if (for_loop_var_stack_ptr + 1 >= MAX_LOOP_NESTING)error("'for' değişkeni için max iç içe geçme aşıldı");
        for_loop_var_stack_ptr++; strncpy(for_loop_vars_stack[for_loop_var_stack_ptr], name, MAX_IDENT_LEN -1);
        for_loop_vars_stack[for_loop_var_stack_ptr][MAX_IDENT_LEN-1]='\0';
    }
    num_variables++;
    return new_var;
}
void remove_variable_by_name_from_symbol_table(const char* name_to_remove) { 
    for (int i = num_variables - 1; i >= 0; i--) { 
        if (strcmp(symbol_table[i].name, name_to_remove) == 0 && symbol_table[i].is_loop_var) {
            if(symbol_table[i].type==VAR_ARRAY&&symbol_table[i].value.array.data)free(symbol_table[i].value.array.data);
            // Shift remaining elements, not efficient but ok for small number of loop vars
            for(int j=i;j<num_variables-1;j++)symbol_table[j]=symbol_table[j+1];
            num_variables--; return; 
        }
    }
}

// --- Fonksiyon Tablosu Yönetimi ---
FunctionDefinition* find_function(const char* name) {
    for (int i = 0; i < num_functions; ++i) {
        if (strcmp(function_table[i].name, name) == 0) {
            return &function_table[i];
        }
    }
    return NULL;
}

// --- Parser Yardımcıları --- 
Token consume_token(TokenType expected_type) {
    if (current_token_idx >= num_tokens) { char err[100]; sprintf(err,"EOF beklenmedik şekilde oluştu, beklenen: %s",token_type_names[expected_type]); error(err); }
    Token t = tokens[current_token_idx];
    if (t.type != expected_type) { char err[MAX_STRING_LEN+100];sprintf(err,"Beklenen %s ama %s ('%s') geldi",token_type_names[expected_type],token_type_names[t.type],t.lexeme);error(err);}
    current_token_idx++; return t;
}
Token peek_token() { if (current_token_idx >= num_tokens) return create_token(TOKEN_EOF, "EOF"); return tokens[current_token_idx];}
Token peek_next_token() { if(current_token_idx+1>=num_tokens)return create_token(TOKEN_EOF,"EOF"); return tokens[current_token_idx + 1]; }

// --- İfade Çözümleme --- 
Value create_value_int(int v){Value val={VAL_INT};val.as.int_val=v;return val;}
Value create_value_float(double v){Value val={VAL_FLOAT};val.as.float_val=v;return val;}
Value create_value_bool(bool v){Value val={VAL_BOOLEAN};val.as.bool_val=v;return val;}
Value create_value_string(const char* v){Value val={VAL_STRING};if(v){strncpy(val.as.string_val,v,MAX_STRING_LEN-1);val.as.string_val[MAX_STRING_LEN-1]='\0';}else{val.as.string_val[0]='\0';}return val;}
Value create_value_null(){Value val={VAL_NULL};return val;}
Value create_value_array_ref(Variable* v){if(!v||v->type!=VAR_ARRAY)error("create_value_array_ref: geçersiz değişken veya değişken array değil.");Value val={VAL_ARRAY_REF};val.as.array_var=v;return val;}

Value execute_function_call(const FunctionDefinition* func_def, Value args[], int num_args_passed) {
    if (num_args_passed != func_def->num_params) {
        char err[200]; sprintf(err, "'%s' fonksiyonu %d parametre bekliyor ama %d argüman verildi.", func_def->name, func_def->num_params, num_args_passed);
        error(err);
    }
    
    if (call_stack_ptr + 1 >= MAX_CALL_STACK_DEPTH) error("Çağrı yığını taştı (Maksimum iç içe fonksiyon).");
    call_stack_ptr++;
    CallFrame* frame = &call_stack[call_stack_ptr];
    frame->return_address_token_idx = current_token_idx; 
    frame->symbol_table_scope_start_idx = num_variables; 
    frame->prev_loop_depth = loop_depth; loop_depth = 0; 
    frame->prev_for_loop_var_stack_ptr = for_loop_var_stack_ptr; for_loop_var_stack_ptr = -1; 
    frame->func_def = func_def; 
    
    enter_scope(); 
    
    for (int i = 0; i < func_def->num_params; ++i) {
        Variable* param_var = declare_variable(func_def->params[i].name, func_def->params[i].type, false, VAR_NULL_TYPE, 0); // Last two args irrelevant for non-arrays
        Value arg_val = args[i];
        bool type_match = false;
        if (param_var->type == VAR_INT && arg_val.type == VAL_INT) { param_var->value.int_value = arg_val.as.int_val; type_match=true;}
        else if (param_var->type == VAR_FLOAT && arg_val.type == VAL_FLOAT) { param_var->value.float_value = arg_val.as.float_val; type_match=true;}
        else if (param_var->type == VAR_FLOAT && arg_val.type == VAL_INT) { param_var->value.float_value = (double)arg_val.as.int_val; type_match=true;} 
        else if (param_var->type == VAR_STRING && arg_val.type == VAL_STRING) { strncpy(param_var->value.string_value, arg_val.as.string_val, MAX_STRING_LEN-1);param_var->value.string_value[MAX_STRING_LEN-1]='\0'; type_match=true;}
        else if (param_var->type == VAR_BOOLEAN && arg_val.type == VAL_BOOLEAN) { param_var->value.bool_value = arg_val.as.bool_val; type_match=true;}
        // Arrays are not directly passable by value in this design, only by reference (which isn't implemented as a parameter type yet)
        
        if (!type_match) {
            char err[250]; sprintf(err, "'%s' fonksiyonunun '%s' parametresine tip uyuşmazlığı: beklenen %s, verilen %s",
                                   func_def->name, func_def->params[i].name, var_type_to_string_user(param_var->type), value_type_to_string(arg_val.type));
            // Cleanup before erroring:
            // Since parameters are declared in a new scope, exit_scope will clean them.
            // But we need to pop the call frame manually.
            exit_scope(); 
            call_stack_ptr--; 
            error(err);
        }
        param_var->is_defined = true;
    }
    
    int previous_body_start_token_idx = current_token_idx; // Save current token index before jumping
    current_token_idx = func_def->body_start_token_idx;
    g_return_flag = false;
    g_return_value_holder = create_value_null(); 
    
    bool dummy_b = false, dummy_c = false; 
    // Function body execution scope is managed by enter_scope above and exit_scope below.
    // parse_block itself should not create another scope if in_function_body is true.
    // The LBRACE of the function body is at func_def->body_start_token_idx.
    // We need to parse the block starting from LBRACE.
    if (tokens[current_token_idx].type != TOKEN_LBRACE) {
        // This should not happen if parse_fun_declaration is correct
        error("Fonksiyon gövdesi başlangıcında LBRACE ({) bekleniyordu.");
    }
    parse_block(true, &dummy_b, &dummy_c, true); // true for in_function_body
    
    Value return_val_from_func = g_return_value_holder; 
    if(g_return_flag == false && func_def->return_type != VAR_VOID){ 
        char err[200]; sprintf(err, "'%s' fonksiyonu değer döndürmeliydi (%s) ama return ifadesi bulunamadı (veya gövde sonuna ulaşıldı).", func_def->name, var_type_to_string_user(func_def->return_type));
        exit_scope(); 
        loop_depth = frame->prev_loop_depth; 
        for_loop_var_stack_ptr = frame->prev_for_loop_var_stack_ptr;
        call_stack_ptr--;
        error(err);
    }
    
    exit_scope(); 
    
    current_token_idx = frame->return_address_token_idx; // Restore token index to after the call
    loop_depth = frame->prev_loop_depth;
    for_loop_var_stack_ptr = frame->prev_for_loop_var_stack_ptr;
    call_stack_ptr--;
    
    if (func_def->return_type == VAR_VOID) {
        if (g_return_flag && return_val_from_func.type != VAL_NULL) { 
            error("Void fonksiyon değer döndüremez (return ifadesiyle bir değer döndürmeye çalıştı).");
        }
        return create_value_null(); 
    } else { 
        bool ret_type_match = false;
        if (func_def->return_type == VAR_INT && return_val_from_func.type == VAL_INT) ret_type_match = true;
        else if (func_def->return_type == VAR_FLOAT && return_val_from_func.type == VAL_FLOAT) ret_type_match = true;
        else if (func_def->return_type == VAR_FLOAT && return_val_from_func.type == VAL_INT) { 
            return_val_from_func = create_value_float((double)return_val_from_func.as.int_val);
            ret_type_match = true;
        }
        else if (func_def->return_type == VAR_STRING && return_val_from_func.type == VAL_STRING) ret_type_match = true;
        else if (func_def->return_type == VAR_BOOLEAN && return_val_from_func.type == VAL_BOOLEAN) ret_type_match = true;
        
        if (!ret_type_match) {
            char err[250]; sprintf(err, "'%s' fonksiyonunun dönüş tipi uyuşmazlığı: beklenen %s, dönen %s",
                                   func_def->name, var_type_to_string_user(func_def->return_type), value_type_to_string(return_val_from_func.type));
            error(err);
        }
        return return_val_from_func;
    }
}

Value parse_primary_expression(bool execute) {
    Token t = peek_token();
    if (t.type == TOKEN_INT_LITERAL) { consume_token(TOKEN_INT_LITERAL); return execute ? create_value_int(t.int_value) : create_value_null(); }
    if (t.type == TOKEN_FLOAT_LITERAL) { consume_token(TOKEN_FLOAT_LITERAL); return execute ? create_value_float(t.float_value) : create_value_null(); }
    if (t.type == TOKEN_STRING_LITERAL) { consume_token(TOKEN_STRING_LITERAL); return execute ? create_value_string(t.string_value) : create_value_null(); }
    if (t.type == TOKEN_TRUE) { consume_token(TOKEN_TRUE); return execute ? create_value_bool(true) : create_value_null(); }
    if (t.type == TOKEN_FALSE) { consume_token(TOKEN_FALSE); return execute ? create_value_bool(false) : create_value_null(); }
    
    if (t.type == TOKEN_IDENTIFIER) {
        Token id_token = consume_token(TOKEN_IDENTIFIER);
        
        if (peek_token().type == TOKEN_LPAREN) { // Possible function call
            consume_token(TOKEN_LPAREN); 
            Value args[MAX_PARAMETERS];
            int num_args_passed = 0;
            
            if (peek_token().type != TOKEN_RPAREN) { 
                do {
                    if (num_args_passed >= MAX_PARAMETERS) error("Fonksiyon çağrısında maksimum argüman sayısı aşıldı.");
                    args[num_args_passed++] = evaluate_expression(execute); 
                    if (peek_token().type == TOKEN_COMMA) consume_token(TOKEN_COMMA); else break;
                } while (true);
            }
            consume_token(TOKEN_RPAREN); 
            
            if (!execute) return create_value_null(); // If not executing expression containing this call, just return null.
            
            // Dahili Fonksiyonlar
            if (is_keyword(id_token.lexeme, "length")) {
                if (num_args_passed != 1) error("'length' 1 argüman bekler.");
                if (args[0].type == VAL_STRING) return create_value_int(strlen(args[0].as.string_val));
                if (args[0].type == VAL_ARRAY_REF) return create_value_int(args[0].as.array_var->value.array.size);
                error("'length' string veya dizi argüman bekler.");
            } else if (is_keyword(id_token.lexeme, "int_to_string")) {
                if(num_args_passed!=1) error("'int_to_string' 1 argüman bekler."); if(args[0].type!=VAL_INT)error("'int_to_string' tamsayı argüman bekler.");
                char buf[MAX_STRING_LEN];sprintf(buf,"%d",args[0].as.int_val);return create_value_string(buf);
            } else if (is_keyword(id_token.lexeme, "concat")) {
                if(num_args_passed!=2) error("'concat' 2 argüman bekler."); if(args[0].type!=VAL_STRING||args[1].type!=VAL_STRING)error("'concat' iki string argüman bekler.");
                char buf[MAX_STRING_LEN];snprintf(buf,MAX_STRING_LEN,"%s%s",args[0].as.string_val,args[1].as.string_val); return create_value_string(buf);
            } else if (is_keyword(id_token.lexeme, "sqrt")) { 
                if (num_args_passed != 1) error("'sqrt' 1 argüman bekler.");
                if (args[0].type == VAL_INT) {
                    if (args[0].as.int_val < 0) error("'sqrt' negatif tamsayı alamaz.");
                    return create_value_float(sqrt((double)args[0].as.int_val));
                } else if (args[0].type == VAL_FLOAT) {
                    if (args[0].as.float_val < 0.0) error("'sqrt' negatif ondalıklı sayı alamaz.");
                    return create_value_float(sqrt(args[0].as.float_val));
                } else {
                    error("'sqrt' sayısal bir argüman (int veya float) bekler.");
                }
            } else if (is_keyword(id_token.lexeme, "to_upper")) { 
                if(num_args_passed!=1)error("'to_upper' 1 argüman bekler."); if(args[0].type!=VAL_STRING)error("'to_upper' string argüman bekler.");
                char res[MAX_STRING_LEN]; strncpy(res,args[0].as.string_val,MAX_STRING_LEN-1); res[MAX_STRING_LEN-1]='\0';
                for(int i_upper=0;res[i_upper];i_upper++) res[i_upper]=toupper((unsigned char)res[i_upper]); return create_value_string(res);
            } else if (is_keyword(id_token.lexeme, "to_lower")) { 
                if(num_args_passed!=1)error("'to_lower' 1 argüman bekler."); if(args[0].type!=VAL_STRING)error("'to_lower' string argüman bekler.");
                char res[MAX_STRING_LEN]; strncpy(res,args[0].as.string_val,MAX_STRING_LEN-1); res[MAX_STRING_LEN-1]='\0';
                for(int i_lower=0;res[i_lower];i_lower++) res[i_lower]=tolower((unsigned char)res[i_lower]); return create_value_string(res);
            } else if (is_keyword(id_token.lexeme, "read_file_text")) { 
                if (num_args_passed != 1) error("'read_file_text' 1 argüman (dosyayolu string) bekler.");
                if (args[0].type != VAL_STRING) error("'read_file_text' dosyayolu string olmalıdır.");
                FILE* file_ptr = fopen(args[0].as.string_val, "rb"); 
                if (!file_ptr) {
                    char err_msg[MAX_STRING_LEN + 100];
                    sprintf(err_msg, "Dosya okunamadı veya bulunamadı: %s", args[0].as.string_val);
                    error(err_msg);
                }
                fseek(file_ptr, 0, SEEK_END); long file_size = ftell(file_ptr); fseek(file_ptr, 0, SEEK_SET);
                if (file_size >= MAX_SOURCE_SIZE) { fclose(file_ptr); char e[MAX_STRING_LEN+100];sprintf(e,"Dosya '%s' okunacak buffer'dan (%ld bayt) büyük (max %d).",args[0].as.string_val, file_size, MAX_SOURCE_SIZE-1);error(e); } 
                
                char* file_content_buffer = (char*) malloc(file_size + 1);
                if (!file_content_buffer) { fclose(file_ptr); error("read_file_text için bellek ayrılamadı.");}
                size_t read_size = fread(file_content_buffer, 1, file_size, file_ptr);
                file_content_buffer[read_size] = '\0'; fclose(file_ptr); Value result_val = create_value_string(file_content_buffer);
                free(file_content_buffer); return result_val;
            } else if (is_keyword(id_token.lexeme, "write_file_text")) { 
                if (num_args_passed != 2) error("'write_file_text' 2 argüman (dosyayolu string, içerik string) bekler.");
                if (args[0].type != VAL_STRING || args[1].type != VAL_STRING) error("'write_file_text' argümanları string olmalıdır.");
                FILE* file_ptr = fopen(args[0].as.string_val, "w");
                if (!file_ptr) { // Could not open file for writing
                    char err_msg[MAX_STRING_LEN + 100];
                    sprintf(err_msg, "Dosya '%s' yazılamadı.", args[0].as.string_val);
                    error(err_msg); // More informative to error out than return false
                    // return create_value_bool(false); 
                }
                fprintf(file_ptr, "%s", args[1].as.string_val); fclose(file_ptr); return create_value_bool(true); 
            }
            // --- YENİ DAHİLİ FONKSİYONLAR ---
            else if (is_keyword(id_token.lexeme, "substring")) {
                if (num_args_passed != 3) error("'substring' 3 argüman bekler (string, baslangic_indisi, uzunluk).");
                if (args[0].type != VAL_STRING) error("'substring' ilk argümanı string olmalıdır.");
                if (args[1].type != VAL_INT) error("'substring' ikinci argümanı (baslangic_indisi) tamsayı olmalıdır.");
                if (args[2].type != VAL_INT) error("'substring' üçüncü argümanı (uzunluk) tamsayı olmalıdır.");
                
                const char* str = args[0].as.string_val;
                int start = args[1].as.int_val;
                int len_req = args[2].as.int_val;
                int str_len_actual = strlen(str);
                
                if (start < 0 || start > str_len_actual || len_req < 0) {
                    char err_msg[200];
                    sprintf(err_msg, "'substring' geçersiz başlangıç (%d) veya uzunluk (%d) (string uzunluğu: %d).", start, len_req, str_len_actual);
                    error(err_msg);
                }
                
                int actual_len_to_copy = len_req;
                if (start + len_req > str_len_actual) {
                    actual_len_to_copy = str_len_actual - start;
                }
                if (actual_len_to_copy < 0) actual_len_to_copy = 0; // if start is at str_len_actual
                
                char sub[MAX_STRING_LEN];
                if (actual_len_to_copy > 0 && actual_len_to_copy < MAX_STRING_LEN) {
                    strncpy(sub, str + start, actual_len_to_copy);
                } else if (actual_len_to_copy >= MAX_STRING_LEN) {
                    error("'substring' sonucu MAX_STRING_LEN'den büyük olamaz.");
                }
                sub[actual_len_to_copy] = '\0';
                return create_value_string(sub);
            } else if (is_keyword(id_token.lexeme, "string_to_int")) {
                if (num_args_passed != 1) error("'string_to_int' 1 argüman bekler (string).");
                if (args[0].type != VAL_STRING) error("'string_to_int' argümanı string olmalıdır.");
                char* endptr;
                const char* str_to_convert = args[0].as.string_val;
                errno = 0; // For overflow/underflow detection with strtol
                long val = strtol(str_to_convert, &endptr, 10);
                
                // Check for various conversion errors
                if (endptr == str_to_convert) { // No digits were found
                    char err_msg[MAX_STRING_LEN + 100];
                    sprintf(err_msg, "'string_to_int': '%s' string'i tamsayıya dönüştürülemedi (sayı bulunamadı).", str_to_convert);
                    error(err_msg);
                } else if (*endptr != '\0' && !isspace((unsigned char)*endptr)) { // Extra characters after number
                    char err_msg[MAX_STRING_LEN + 100];
                    sprintf(err_msg, "'string_to_int': '%s' string'inde sayıdan sonra geçersiz karakterler var.", str_to_convert);
                    error(err_msg);
                } else if (errno == ERANGE || val > INT_MAX || val < INT_MIN) {
                    error("'string_to_int': Değer tamsayı sınırları dışında.");
                }
                return create_value_int((int)val);
            } else if (is_keyword(id_token.lexeme, "string_to_float")) {
                if (num_args_passed != 1) error("'string_to_float' 1 argüman bekler (string).");
                if (args[0].type != VAL_STRING) error("'string_to_float' argümanı string olmalıdır.");
                char* endptr;
                const char* str_to_convert = args[0].as.string_val;
                errno = 0; // For overflow/underflow detection with strtod
                double val = strtod(str_to_convert, &endptr);
                
                if (endptr == str_to_convert) {
                    char err_msg[MAX_STRING_LEN + 100];
                    sprintf(err_msg, "'string_to_float': '%s' string'i ondalıklı sayıya dönüştürülemedi (sayı bulunamadı).", str_to_convert);
                    error(err_msg);
                } else if (*endptr != '\0' && !isspace((unsigned char)*endptr)) {
                    char err_msg[MAX_STRING_LEN + 100];
                    sprintf(err_msg, "'string_to_float': '%s' string'inde sayıdan sonra geçersiz karakterler var.", str_to_convert);
                    error(err_msg);
                } else if (errno == ERANGE) {
                    error("'string_to_float': Değer ondalıklı sayı sınırları dışında.");
                }
                return create_value_float(val);
            } else if (is_keyword(id_token.lexeme, "type_of")) {
                if (num_args_passed != 1) error("'type_of' 1 argüman bekler.");
                switch(args[0].type) {
                    case VAL_INT: return create_value_string("int");
                    case VAL_FLOAT: return create_value_string("float");
                    case VAL_STRING: return create_value_string("string");
                    case VAL_BOOLEAN: return create_value_string("boolean");
                    case VAL_ARRAY_REF: return create_value_string("array");
                    case VAL_NULL: return create_value_string("null");
                    default: return create_value_string("unknown");
                }
            } else if (is_keyword(id_token.lexeme, "pow")) {
                if (num_args_passed != 2) error("'pow' 2 argüman bekler (taban, üs).");
                double base_val, exponent_val;
                if (args[0].type == VAL_INT) base_val = (double)args[0].as.int_val;
                else if (args[0].type == VAL_FLOAT) base_val = args[0].as.float_val;
                else error("'pow' tabanı sayısal olmalıdır (int veya float).");
                
                if (args[1].type == VAL_INT) exponent_val = (double)args[1].as.int_val;
                else if (args[1].type == VAL_FLOAT) exponent_val = args[1].as.float_val;
                else error("'pow' üssü sayısal olmalıdır (int veya float).");
                
                return create_value_float(pow(base_val, exponent_val));
            }
            // Kullanıcı Tanımlı Fonksiyon Çağrısı
            else {
                FunctionDefinition* func_to_call = find_function(id_token.lexeme);
                if (!func_to_call) { 
                    char err[MAX_IDENT_LEN + 100]; 
                    sprintf(err, "'%s' adlı fonksiyon veya dahili komut bulunamadı.", id_token.lexeme); 
                    error(err); 
                }
                return execute_function_call(func_to_call, args, num_args_passed);
            }
        }
        // Dizi Elemanı Erişimi veya Değişken
        else {
            Variable* var = execute ? find_variable(id_token.lexeme) : NULL; 
            if (execute && !var) { char msg[150]; sprintf(msg, "'%s' adlı değişken/dizi bulunamadı", id_token.lexeme); error(msg); }
            
            if (peek_token().type == TOKEN_LBRACKET) { 
                if (execute && var->type != VAR_ARRAY) { char msg[150]; sprintf(msg, "'%s' bir dizi değil, indisle erişilemez.", id_token.lexeme); error(msg); }
                consume_token(TOKEN_LBRACKET); Value idx_val=evaluate_expression(execute); consume_token(TOKEN_RBRACKET);
                if (execute) { 
                    if (idx_val.type != VAL_INT) error("Dizi indisi tamsayı olmalı."); int idx = idx_val.as.int_val;
                    if (idx<0 || idx>=var->value.array.size){ char msg[200]; sprintf(msg,"Dizi sınırları dışında erişim: %s[%d] (boyut: %d)",id_token.lexeme,idx, var->value.array.size);error(msg);}
                    
                    size_t element_s = get_sizeof_element_type(var->value.array.element_type);
                    if(element_s == 0) error("Dizi eleman boyutu sıfır (okuma).");
                    void* el_ptr =(char*)var->value.array.data + idx * element_s;
                    
                    switch(var->value.array.element_type){
                        case VAR_INT: return create_value_int(*(int*)el_ptr); 
                        case VAR_FLOAT: return create_value_float(*(double*)el_ptr);
                        case VAR_BOOLEAN: return create_value_bool(*(bool*)el_ptr); 
                        case VAR_STRING: return create_value_string((char*)el_ptr);
                        default: error("Dizi elemanı için desteklenmeyen tip (okuma).");
                    }
                }
                return create_value_null(); 
            } else { // Normal değişken
                if (execute) { 
                    if(!var->is_defined && var->type != VAR_ARRAY) { char msg[150]; sprintf(msg, "'%s' değişkeni atanmadan kullanıldı", id_token.lexeme); error(msg); }
                    switch(var->type){
                        case VAR_INT: return create_value_int(var->value.int_value); case VAR_FLOAT: return create_value_float(var->value.float_value);
                        case VAR_STRING: return create_value_string(var->value.string_value); case VAR_BOOLEAN: return create_value_bool(var->value.bool_value);
                        case VAR_ARRAY: return create_value_array_ref(var); // Return reference to the array itself
                        default: error("İfadede bilinmeyen değişken tipi.");
                    }
                }
                return create_value_null(); 
            }
        }
    }
    else if (t.type == TOKEN_LPAREN) { consume_token(TOKEN_LPAREN); Value v=evaluate_expression(execute); consume_token(TOKEN_RPAREN); return v;}
    else if (t.type == TOKEN_USER) {  
        consume_token(TOKEN_USER); consume_token(TOKEN_DOT); Token im=consume_token(TOKEN_IDENTIFIER);
        if(execute){ char ib[MAX_STRING_LEN];
            if(is_keyword(im.lexeme,"in")){int v_in;printf("> ");fflush(stdout);if(scanf("%d",&v_in)!=1){while(getchar()!='\n');error("Geçersiz int girişi.");}int c; while((c=getchar())!='\n'&&c!=EOF);return create_value_int(v_in);}
            else if(is_keyword(im.lexeme,"in_float")){double v_f;printf("> ");fflush(stdout);if(scanf("%lf",&v_f)!=1){while(getchar()!='\n');error("Geçersiz float girişi.");}int c; while((c=getchar())!='\n'&&c!=EOF);return create_value_float(v_f);}
            else if(is_keyword(im.lexeme,"in_string")){printf("> ");fflush(stdout);if(!fgets(ib,MAX_STRING_LEN,stdin))error("String okuma hatası.");ib[strcspn(ib,"\n")]=0;return create_value_string(ib);}
            else if(is_keyword(im.lexeme,"in_boolean")){printf("(true/false)> ");fflush(stdout);if(!fgets(ib,sizeof(ib),stdin))error("Bool okuma hatası.");ib[strcspn(ib,"\n")]=0;
                if(is_keyword(ib,"true"))return create_value_bool(true);if(is_keyword(ib,"false"))return create_value_bool(false);error("Geçersiz bool girişi. 'true' veya 'false' bekleniyor.");}
                else{char err[100+MAX_IDENT_LEN];sprintf(err,"Bilinmeyen kullanıcı giriş komutu: user.%s",im.lexeme);error(err);}}
                return create_value_null();
    }
    else { char err[100];sprintf(err,"İfadede beklenmedik token (primary): %s ('%s')",token_type_names[t.type],t.lexeme);error(err);return create_value_null();}
}

Value parse_unary_expression(bool execute) { 
    if(peek_token().type==TOKEN_NOT){consume_token(TOKEN_NOT);Value o=parse_unary_expression(execute);if(execute){if(o.type!=VAL_BOOLEAN)error("'!' (NOT) operatörü mantıksal (boolean) bir değer bekler.");return create_value_bool(!o.as.bool_val);}return create_value_null();}
    else if(peek_token().type==TOKEN_MINUS){consume_token(TOKEN_MINUS);Value o=parse_unary_expression(execute);if(execute){if(o.type==VAL_INT)return create_value_int(-o.as.int_val);if(o.type==VAL_FLOAT)return create_value_float(-o.as.float_val);error("'-' (unary minus) operatörü sayısal bir değer (int veya float) bekler.");}return create_value_null();}
    return parse_primary_expression(execute);
}
Value parse_multiplicative_expression(bool execute) { 
    Value l=parse_unary_expression(execute); while(peek_token().type==TOKEN_MULTIPLY||peek_token().type==TOKEN_DIVIDE||peek_token().type==TOKEN_MODULO){
        Token op=consume_token(peek_token().type);Value r=parse_unary_expression(execute);if(execute){
            if(!((l.type==VAL_INT||l.type==VAL_FLOAT)&&(r.type==VAL_INT||r.type==VAL_FLOAT))){char e[200];sprintf(e,"'%s' operatörü sayısal olmayan operandlarla (%s, %s) kullanılamaz.",op.lexeme, value_type_to_string(l.type), value_type_to_string(r.type));error(e);}
            double lv=(l.type==VAL_INT)?(double)l.as.int_val:l.as.float_val; double rv=(r.type==VAL_INT)?(double)r.as.int_val:r.as.float_val; // Promote to double for calculation
            if(op.type==TOKEN_MULTIPLY){bool result_is_float=(l.type==VAL_FLOAT||r.type==VAL_FLOAT);l=result_is_float?create_value_float(lv*rv):create_value_int((int)(lv*rv));}
            else if(op.type==TOKEN_DIVIDE){if(rv==0.0)error("Sıfıra bölme hatası.");if(l.type==VAL_INT&&r.type==VAL_INT && (int)lv % (int)rv == 0)l=create_value_int((int)(lv/rv));else l=create_value_float(lv/rv);} // Prefer int if exact int division
            else if(op.type==TOKEN_MODULO){if(l.type!=VAL_INT||r.type!=VAL_INT)error("'%' (modulo) operatörü tamsayı operandlar gerektirir.");if(r.as.int_val==0)error("Sıfıra mod alma hatası.");l=create_value_int(l.as.int_val%r.as.int_val);}}
            else l=create_value_null(); /* For !execute, just parse and return null */ }return l;
}
Value parse_additive_expression(bool execute) { 
    Value l=parse_multiplicative_expression(execute); while(peek_token().type==TOKEN_PLUS||peek_token().type==TOKEN_MINUS){
        Token op=consume_token(peek_token().type);Value r=parse_multiplicative_expression(execute);if(execute){
            if(op.type==TOKEN_PLUS&&(l.type==VAL_STRING||r.type==VAL_STRING)){ // String concatenation
                char sl_buf[MAX_STRING_LEN], sr_buf[MAX_STRING_LEN]; // Buffers for string representations
                
                // Convert left operand to string if not already
                if(l.type==VAL_STRING) strncpy(sl_buf,l.as.string_val,MAX_STRING_LEN-1);
                else if(l.type==VAL_INT) snprintf(sl_buf,MAX_STRING_LEN,"%d",l.as.int_val);
                else if(l.type==VAL_FLOAT) snprintf(sl_buf,MAX_STRING_LEN,"%g",l.as.float_val);
                else if(l.type==VAL_BOOLEAN) strncpy(sl_buf,l.as.bool_val?"true":"false",MAX_STRING_LEN-1);
                else if(l.type==VAL_NULL) strncpy(sl_buf,"null",MAX_STRING_LEN-1);
                else { char e[200]; sprintf(e, "String ile '+' operatörünün sol tarafı birleştirilemeyen tipte: %s", value_type_to_string(l.type)); error(e); }
                sl_buf[MAX_STRING_LEN-1] = '\0';
                
                // Convert right operand to string if not already
                if(r.type==VAL_STRING) strncpy(sr_buf,r.as.string_val,MAX_STRING_LEN-1);
                else if(r.type==VAL_INT) snprintf(sr_buf,MAX_STRING_LEN,"%d",r.as.int_val);
                else if(r.type==VAL_FLOAT) snprintf(sr_buf,MAX_STRING_LEN,"%g",r.as.float_val);
                else if(r.type==VAL_BOOLEAN) strncpy(sr_buf,r.as.bool_val?"true":"false",MAX_STRING_LEN-1);
                else if(r.type==VAL_NULL) strncpy(sr_buf,"null",MAX_STRING_LEN-1);
                else { char e[200]; sprintf(e, "String ile '+' operatörünün sağ tarafı birleştirilemeyen tipte: %s", value_type_to_string(r.type)); error(e); }
                sr_buf[MAX_STRING_LEN-1] = '\0';
                
                if (strlen(sl_buf) + strlen(sr_buf) >= MAX_STRING_LEN) {
                    error("String birleştirme sonucu MAX_STRING_LEN sınırını aşıyor.");
                }
                char result_buf[MAX_STRING_LEN];
                strcpy(result_buf, sl_buf); // Use strcpy then strcat to avoid snprintf complexities with existing content
                strcat(result_buf, sr_buf);
                l=create_value_string(result_buf);}
                else if((l.type==VAL_INT||l.type==VAL_FLOAT)&&(r.type==VAL_INT||r.type==VAL_FLOAT)){ // Numeric addition/subtraction
                    double lv=(l.type==VAL_INT)?(double)l.as.int_val:l.as.float_val;double rv=(r.type==VAL_INT)?(double)r.as.int_val:r.as.float_val;
                    bool result_is_float=(l.type==VAL_FLOAT||r.type==VAL_FLOAT);
                    if(op.type==TOKEN_PLUS)l=result_is_float?create_value_float(lv+rv):create_value_int((int)(lv+rv));
                    else if(op.type==TOKEN_MINUS)l=result_is_float?create_value_float(lv-rv):create_value_int((int)(lv-rv));}
                    else{char e[200];sprintf(e,"'%s' operatörü uyumsuz tiplerle (%s, %s) kullanılamaz (sayısal veya string birleştirme bekleniyor).",op.lexeme,value_type_to_string(l.type),value_type_to_string(r.type));error(e);}}
                    else l=create_value_null(); /* For !execute */ }return l;
}
Value parse_relational_expression(bool execute) { 
    Value l=parse_additive_expression(execute); while(peek_token().type==TOKEN_GT||peek_token().type==TOKEN_LT||peek_token().type==TOKEN_GTE||peek_token().type==TOKEN_LTE){
        Token op=consume_token(peek_token().type);Value r=parse_additive_expression(execute);if(execute){bool res=false;
            if((l.type==VAL_INT||l.type==VAL_FLOAT)&&(r.type==VAL_INT||r.type==VAL_FLOAT)){ // Numeric comparison
                double lv=(l.type==VAL_INT)?(double)l.as.int_val:l.as.float_val;double rv=(r.type==VAL_INT)?(double)r.as.int_val:r.as.float_val;
                if(op.type==TOKEN_GT)res=lv>rv;else if(op.type==TOKEN_LT)res=lv<rv;else if(op.type==TOKEN_GTE)res=lv>=rv;else if(op.type==TOKEN_LTE)res=lv<=rv;}
                else if(l.type==VAL_STRING&&r.type==VAL_STRING){ // String comparison
                    int cr=strcmp(l.as.string_val,r.as.string_val);
                    if(op.type==TOKEN_GT)res=cr>0;else if(op.type==TOKEN_LT)res=cr<0;else if(op.type==TOKEN_GTE)res=cr>=0;else if(op.type==TOKEN_LTE)res=cr<=0;}
                    else{char e[250];sprintf(e,"Karşılaştırma operatörleri ('%s') sayısal veya metin tipleri arasında uygulanabilir. Alınan: %s ve %s.",op.lexeme,value_type_to_string(l.type),value_type_to_string(r.type));error(e);}
                    l=create_value_bool(res);}else l=create_value_null(); /* For !execute */ }return l;
}
Value parse_equality_expression(bool execute) { 
    Value l=parse_relational_expression(execute);while(peek_token().type==TOKEN_EQ||peek_token().type==TOKEN_NEQ){
        Token op=consume_token(peek_token().type);Value r=parse_relational_expression(execute);if(execute){bool res=false;
            if(l.type==r.type){ // Same type comparison
                switch(l.type){
                    case VAL_INT:res=(l.as.int_val==r.as.int_val);break;
                    case VAL_FLOAT:res=(fabs(l.as.float_val-r.as.float_val)<1e-9);break; // Epsilon comparison for floats
                    case VAL_STRING:res=(strcmp(l.as.string_val,r.as.string_val)==0);break;
                    case VAL_BOOLEAN:res=(l.as.bool_val==r.as.bool_val);break;
                    case VAL_NULL:res=true;break; // null == null is true
                    case VAL_ARRAY_REF: res=(l.as.array_var == r.as.array_var); break; // Array comparison by reference
                    default:res=false; // Should not happen for known types
                }
            } else { // Different types, generally false, except for int/float
                if((l.type==VAL_INT&&r.type==VAL_FLOAT))res=(fabs((double)l.as.int_val-r.as.float_val)<1e-9);
                else if((l.type==VAL_FLOAT&&r.type==VAL_INT))res=(fabs(l.as.float_val-(double)r.as.int_val)<1e-9);
                // null can be compared with other types, it's always false unless other is also null (handled above)
                else if (l.type == VAL_NULL || r.type == VAL_NULL) res = false;
                else res=false; // All other different type comparisons are false
            }
            if(op.type==TOKEN_NEQ)res=!res; l=create_value_bool(res);}else l=create_value_null(); /* For !execute */ }return l;
}
Value parse_logical_and_expression(bool execute) { 
    Value l=parse_equality_expression(execute);while(peek_token().type==TOKEN_AND){
        consume_token(TOKEN_AND);
        bool execute_rhs = execute; // Default: execute RHS if overall expression is executed
        if(execute && l.type==VAL_BOOLEAN && !l.as.bool_val) { // Short-circuit: if LHS is false, no need to eval RHS
            execute_rhs = false;
        }
        Value r=parse_equality_expression(execute_rhs);
        if(execute){
            if(l.type!=VAL_BOOLEAN||r.type!=VAL_BOOLEAN)error("'&&' (AND) operatörü mantıksal (boolean) operandlar bekler.");
            if(!execute_rhs) { // if RHS was short-circuited
                l=create_value_bool(false); // Result is false
            } else {
                l=create_value_bool(l.as.bool_val&&r.as.bool_val);
            }
        } else {
            l=create_value_null(); /* For !execute */
        }
    } return l;
}
Value parse_logical_or_expression(bool execute) { 
    Value l=parse_logical_and_expression(execute);while(peek_token().type==TOKEN_OR){
        consume_token(TOKEN_OR);
        bool execute_rhs = execute;
        if(execute && l.type==VAL_BOOLEAN && l.as.bool_val) { // Short-circuit: if LHS is true, no need to eval RHS
            execute_rhs = false;
        }
        Value r=parse_logical_and_expression(execute_rhs);
        if(execute){
            if(l.type!=VAL_BOOLEAN||r.type!=VAL_BOOLEAN)error("'||' (OR) operatörü mantıksal (boolean) operandlar bekler.");
            if(!execute_rhs) { // if RHS was short-circuited
                l=create_value_bool(true); // Result is true
            } else {
                l=create_value_bool(l.as.bool_val||r.as.bool_val);
            }
        } else {
            l=create_value_null(); /* For !execute */
        }
    }return l;
}
Value evaluate_expression(bool execute) { return parse_logical_or_expression(execute); }

// --- Deyim Çözümleyicileri ---
Value parse_assignment_rhs(VarType expected_lhs_type, bool execute) { 
    Value rhs_val=evaluate_expression(execute);
    if(execute){
        bool types_compatible=false;
        if(expected_lhs_type==VAR_INT && rhs_val.type==VAL_INT) types_compatible=true;
        else if(expected_lhs_type==VAR_FLOAT && rhs_val.type==VAL_FLOAT) types_compatible=true;
        else if(expected_lhs_type==VAR_FLOAT && rhs_val.type==VAL_INT){rhs_val=create_value_float((double)rhs_val.as.int_val);types_compatible=true;} // Auto-promote int to float
        else if(expected_lhs_type==VAR_STRING && rhs_val.type==VAL_STRING) types_compatible=true;
        else if(expected_lhs_type==VAR_BOOLEAN && rhs_val.type==VAL_BOOLEAN) types_compatible=true;
        else if(expected_lhs_type==VAR_NULL_TYPE) types_compatible=true; // Internal use, e.g. when LHS type isn't known yet during parsing phase
        
        if(!types_compatible){
            char err_msg[250];
            sprintf(err_msg,"Tip uyuşmazlığı: '%s' tipindeki bir değişkene '%s' tipinde bir değer atanamaz.",var_type_to_string_user(expected_lhs_type),value_type_to_string(rhs_val.type));
            error(err_msg);
        }
    }
    return rhs_val;
}

VarType parse_type_specifier() {
    Token type_token = peek_token();
    if (type_token.type == TOKEN_INT_TYPE) { consume_token(TOKEN_INT_TYPE); return VAR_INT; }
    if (type_token.type == TOKEN_STRING_TYPE) { consume_token(TOKEN_STRING_TYPE); return VAR_STRING; }
    if (type_token.type == TOKEN_FLOAT_TYPE) { consume_token(TOKEN_FLOAT_TYPE); return VAR_FLOAT; }
    if (type_token.type == TOKEN_BOOLEAN_TYPE) { consume_token(TOKEN_BOOLEAN_TYPE); return VAR_BOOLEAN; }
    if (type_token.type == TOKEN_VOID_TYPE) { consume_token(TOKEN_VOID_TYPE); return VAR_VOID; } 
    error("Geçersiz veya beklenmeyen tip belirteci.");
    return VAR_NULL_TYPE; // Should not be reached due to error
}

void parse_var_declaration(bool execute, bool is_in_for_initializer) {
    consume_token(TOKEN_VAR); Token name_token=consume_token(TOKEN_IDENTIFIER); consume_token(TOKEN_COLON);
    VarType declared_base_type = parse_type_specifier(); 
    if(declared_base_type == VAR_VOID && !is_in_for_initializer) { // Allow void for function return type, not var decl. For initializer could be part of function-like construct (not standard C*).
        error("Değişken 'void' tipinde olamaz.");
    }
    
    int array_size=0; VarType final_type = declared_base_type;
    if(peek_token().type==TOKEN_LBRACKET){
        if (declared_base_type == VAR_VOID) error("Void tipinde dizi tanımlanamaz.");
        consume_token(TOKEN_LBRACKET);
        Value size_val = evaluate_expression(execute); // Array size expression can be evaluated
        consume_token(TOKEN_RBRACKET);
        
        final_type = VAR_ARRAY;
        if(execute){
            if(size_val.type!=VAL_INT)error("Dizi boyutu tamsayı olmalı.");
            array_size=size_val.as.int_val;
            if(array_size<=0)error("Dizi boyutu pozitif olmalı.");
        } else {
            array_size=1; // Dummy size for parsing when not executing
        }
    }
    
    Variable* var_ptr=NULL; 
    if(execute){
        var_ptr=declare_variable(name_token.lexeme,final_type,is_in_for_initializer, (final_type == VAR_ARRAY ? declared_base_type : VAR_NULL_TYPE), array_size);
    } else if(is_in_for_initializer){ // If not executing but it's a for loop initializer, still track the var name for later removal
        if(for_loop_var_stack_ptr+1<MAX_LOOP_NESTING){
            for_loop_var_stack_ptr++;
            strncpy(for_loop_vars_stack[for_loop_var_stack_ptr],name_token.lexeme,MAX_IDENT_LEN-1);
            for_loop_vars_stack[for_loop_var_stack_ptr][MAX_IDENT_LEN-1]='\0';
        } else {
            error("For döngüsü değişken yığını taştı (MAX_LOOP_NESTING).");
        }
    }
    
    if(peek_token().type==TOKEN_ASSIGN){
        consume_token(TOKEN_ASSIGN);
        if(final_type==VAR_ARRAY)error("Dizi tanımında doğrudan atama desteklenmiyor (var arr: int[] = ...). Elemanlara tek tek atama yapın.");
        
        Value rhs_val=parse_assignment_rhs(declared_base_type,execute); // Use declared_base_type for RHS check
        if(execute && var_ptr){ // var_ptr should be non-NULL if execute is true
            var_ptr->is_defined=true;
            switch(declared_base_type){ // Assignment to non-array variable
                case VAR_INT:var_ptr->value.int_value=rhs_val.as.int_val;break;
                case VAR_FLOAT:var_ptr->value.float_value=rhs_val.as.float_val;break;
                case VAR_STRING:strncpy(var_ptr->value.string_value,rhs_val.as.string_val,MAX_STRING_LEN-1);var_ptr->value.string_value[MAX_STRING_LEN-1]='\0';break;
                case VAR_BOOLEAN:var_ptr->value.bool_value=rhs_val.as.bool_val;break;
                default: /* Should be caught by type check or VAR_VOID check */ break;
            }
        }
    } else { // No assignment
        if(execute && var_ptr && final_type!=VAR_ARRAY) {
            // Non-array variables are marked as undefined if not initialized.
            // Arrays are considered "defined" (memory allocated, elements default-initialized) upon declaration.
            var_ptr->is_defined=false; 
        }
    }
    if(!is_in_for_initializer)consume_token(TOKEN_SEMICOLON);
}

void parse_assignment(bool execute) { 
    Token ident_token = consume_token(TOKEN_IDENTIFIER); 
    Variable* target_var = execute ? find_variable(ident_token.lexeme) : NULL;
    
    if(execute && !target_var){
        char msg[100+MAX_IDENT_LEN];
        sprintf(msg,"Atama yapılacak '%s' değişkeni bulunamadı.",ident_token.lexeme);
        error(msg);
    }
    
    VarType effective_lhs_type = VAR_NULL_TYPE; // Type of the actual L-value (var or var[idx])
    void* array_element_target_ptr = NULL;    // If L-value is an array element, points to it
    
    if(peek_token().type==TOKEN_LBRACKET){ // Array element assignment: ident[expr] = ...
        if(execute && target_var->type != VAR_ARRAY) {
            char msg[150]; sprintf(msg, "'%s' bir dizi değil, indisle atama yapılamaz.", ident_token.lexeme); error(msg);
        }
        consume_token(TOKEN_LBRACKET);
        Value index_val = evaluate_expression(execute);
        consume_token(TOKEN_RBRACKET);
        
        if (target_var) effective_lhs_type = target_var->value.array.element_type;
        // else if !execute, effective_lhs_type remains VAR_NULL_TYPE which is fine for parse_assignment_rhs(..., false)
        
        if(execute && target_var){ // target_var must be non-null if execute
            if(index_val.type != VAL_INT) error("Dizi atamasında indis tamsayı olmalı.");
            int idx = index_val.as.int_val;
            if(idx < 0 || idx >= target_var->value.array.size){
                char err_msg[150+MAX_IDENT_LEN];
                sprintf(err_msg,"Dizi sınırları dışında atama: '%s[%d]' (boyut: %d)",target_var->name,idx, target_var->value.array.size);
                error(err_msg);
            }
            size_t element_s = get_sizeof_element_type(target_var->value.array.element_type);
            if(element_s == 0) error("Dizi eleman boyutu sıfır (atama).");
            array_element_target_ptr = (char*)target_var->value.array.data + idx * element_s;
        }
    } else { // Simple variable assignment: ident = ...
        if(target_var) effective_lhs_type = target_var->type;
        // else if !execute, effective_lhs_type remains VAR_NULL_TYPE
    }
    
    consume_token(TOKEN_ASSIGN);
    Value rhs_val = parse_assignment_rhs(effective_lhs_type, execute);
    
    if(execute && target_var){ // target_var must be non-null
        if(array_element_target_ptr){ // Assigning to array element
            switch(effective_lhs_type){ // which is target_var->value.array.element_type
                case VAR_INT:    *((int*)array_element_target_ptr) = rhs_val.as.int_val; break;
                case VAR_FLOAT:  *((double*)array_element_target_ptr) = rhs_val.as.float_val; break;
                case VAR_BOOLEAN:*((bool*)array_element_target_ptr) = rhs_val.as.bool_val; break;
                case VAR_STRING: strncpy((char*)array_element_target_ptr, rhs_val.as.string_val, MAX_STRING_LEN-1);
                ((char*)array_element_target_ptr)[MAX_STRING_LEN-1] = '\0'; break;
                default: error("Dizi elemanına bilinmeyen veya desteklenmeyen tipte atama yapıldı.");
            }
        } else { // Assigning to a simple variable
            target_var->is_defined = true;
            switch(target_var->type){ // which is effective_lhs_type
                case VAR_INT:    target_var->value.int_value = rhs_val.as.int_val; break;
                case VAR_FLOAT:  target_var->value.float_value = rhs_val.as.float_val; break;
                case VAR_STRING: strncpy(target_var->value.string_value, rhs_val.as.string_val, MAX_STRING_LEN-1);
                target_var->value.string_value[MAX_STRING_LEN-1] = '\0'; break;
                case VAR_BOOLEAN:target_var->value.bool_value = rhs_val.as.bool_val; break;
                case VAR_ARRAY:  error("Bir dizi değişkenine doğrudan atama yapılamaz (örn: arr1 = arr2)."); break;
                default: error("Değişkene bilinmeyen veya desteklenmeyen tipte atama yapıldı.");
            }
        }
    }
    consume_token(TOKEN_SEMICOLON);
}
void print_value_recursive(Value val) { 
    switch(val.type){case VAL_INT:printf("%d",val.as.int_val);break;case VAL_FLOAT:printf("%g",val.as.float_val);break;
        case VAL_STRING:printf("%s",val.as.string_val);break; // Removed extra quotes for display consistency with user input strings
        case VAL_BOOLEAN:printf("%s",val.as.bool_val?"true":"false");break;
        case VAL_ARRAY_REF:{Variable*av=val.as.array_var;printf("[");for(int k=0;k<av->value.array.size;++k){
            
            size_t element_s = get_sizeof_element_type(av->value.array.element_type);
            if(element_s == 0) { printf("<?>"); continue; } // Should not happen
            void*ep=(char*)av->value.array.data+k*element_s;
            Value et=create_value_null();
            
            switch(av->value.array.element_type){case VAR_INT:et=create_value_int(*(int*)ep);break;case VAR_FLOAT:et=create_value_float(*(double*)ep);break;
                case VAR_BOOLEAN:et=create_value_bool(*(bool*)ep);break; case VAR_STRING:et=create_value_string((char*)ep);break;default:printf("<?>");break;}
                print_value_recursive(et);if(k<av->value.array.size-1)printf(", ");}printf("]");break;}
                case VAL_NULL:printf("null");break;default:printf("<bilinmeyen_tip_yazdirma>");}
}
void parse_out_display(bool execute) { 
    consume_token(TOKEN_OUT);consume_token(TOKEN_DOT);consume_token(TOKEN_DISPLAY);consume_token(TOKEN_LPAREN);
    Value vtd=evaluate_expression(execute);consume_token(TOKEN_RPAREN);consume_token(TOKEN_SEMICOLON);
    if(execute){print_value_recursive(vtd);printf("\n");fflush(stdout);}
}

void parse_if_statement(bool execute, bool* break_flag, bool* continue_flag, bool in_function_body) {
    consume_token(TOKEN_IF);consume_token(TOKEN_LPAREN);Value cond_val=evaluate_expression(execute);consume_token(TOKEN_RPAREN);
    
    bool actual_condition_result=false;
    if(execute){
        if(cond_val.type!=VAL_BOOLEAN)error("If koşulu mantıksal (boolean) bir değer olmalıdır.");
        actual_condition_result=cond_val.as.bool_val;
    }
    
    bool execute_then_block = execute && actual_condition_result;
    parse_block(execute_then_block, break_flag, continue_flag, in_function_body); 
    
    if(g_return_flag || (*break_flag && execute_then_block) || (*continue_flag && execute_then_block) ) return; // If block caused jump, don't process else
    
    if(peek_token().type==TOKEN_ELSE){
        consume_token(TOKEN_ELSE);
        bool execute_else_block = execute && !actual_condition_result;
        parse_block(execute_else_block,break_flag,continue_flag,in_function_body);
    }
}
void parse_while_statement(bool execute, bool* break_flag_outer, bool* continue_flag_outer, bool in_function_body) {
    consume_token(TOKEN_WHILE); 
    int condition_start_token_idx = current_token_idx; // Start of (condition)
    loop_depth++;
    
    bool local_break_flag=false, local_continue_flag=false; 
    
    if(execute){
        while(true){
            current_token_idx = condition_start_token_idx; // Rewind to parse condition
            consume_token(TOKEN_LPAREN);
            Value cond_val=evaluate_expression(true); // Condition is always evaluated if loop executes
            consume_token(TOKEN_RPAREN);
            int body_start_token_idx = current_token_idx; // Save start of block
            
            if(cond_val.type!=VAL_BOOLEAN)error("While koşulu mantıksal (boolean) bir değer olmalıdır.");
            if(!cond_val.as.bool_val)break; // Condition is false, exit loop
            
            local_continue_flag=false; local_break_flag=false;
            parse_block(true, &local_break_flag, &local_continue_flag, in_function_body);
            
            if(g_return_flag){loop_depth--;return;} // Function returned from loop
            if(local_break_flag)break; // Break statement executed
            if(local_continue_flag)continue; // Continue statement executed (loop will re-eval condition)
        }
    }
    
    // If not executing, or after loop finishes, skip parsing the condition and body once
    // to advance current_token_idx correctly.
    current_token_idx = condition_start_token_idx;
    consume_token(TOKEN_LPAREN);
    evaluate_expression(false); // Parse condition (don't execute)
    consume_token(TOKEN_RPAREN);
    bool dummy_b=false, dummy_c=false;
    parse_block(false, &dummy_b, &dummy_c, in_function_body); // Parse block (don't execute)
    
    loop_depth--;
    if (execute && local_break_flag && break_flag_outer) *break_flag_outer = true; // Propagate break if needed
}
void parse_for_statement(bool execute, bool* break_flag_outer, bool* continue_flag_outer, bool in_function_body) { 
    consume_token(TOKEN_FOR);consume_token(TOKEN_LPAREN);
    
    char declared_loop_var_name[MAX_IDENT_LEN]="";
    bool loop_var_declared_here=false;
    int original_for_loop_var_stack_ptr = for_loop_var_stack_ptr;
    
    // 1. Initializer
    if(execute) enter_scope(); // Scope for loop variable if declared here
    if(peek_token().type==TOKEN_VAR){
        parse_var_declaration(execute,true); // is_in_for_initializer = true
        loop_var_declared_here=true;
        if(execute && for_loop_var_stack_ptr > original_for_loop_var_stack_ptr){ // If a var was pushed
            strncpy(declared_loop_var_name, for_loop_vars_stack[for_loop_var_stack_ptr], MAX_IDENT_LEN-1);
            declared_loop_var_name[MAX_IDENT_LEN-1]='\0';
        }
    } else if(peek_token().type!=TOKEN_SEMICOLON){ // Assignment or expression
        evaluate_expression(execute); // e.g. i = 0
    }
    // Semicolon after initializer is consumed by parse_var_declaration or here
    if (peek_token().type == TOKEN_SEMICOLON) consume_token(TOKEN_SEMICOLON);
    else if (!loop_var_declared_here) error("For döngüsü başlatıcısında ';' bekleniyor.");
    
    
    int condition_expr_start_idx = current_token_idx;
    bool condition_is_empty = (peek_token().type==TOKEN_SEMICOLON);
    if(!condition_is_empty) evaluate_expression(false); // Parse condition to find its end
    consume_token(TOKEN_SEMICOLON);
    
    int increment_expr_start_idx = current_token_idx;
    int temp_parser_idx = current_token_idx;
    int paren_balance = 0; // To find the closing parenthesis of for header
    while(temp_parser_idx < num_tokens) {
        if(tokens[temp_parser_idx].type == TOKEN_LPAREN) paren_balance++;
        else if(tokens[temp_parser_idx].type == TOKEN_RPAREN) {
            if (paren_balance == 0) break; // This is the RPAREN for the FOR's LPAREN
            paren_balance--;
        }
        temp_parser_idx++;
    }
    if(temp_parser_idx >= num_tokens || tokens[temp_parser_idx].type != TOKEN_RPAREN) {
        error("For döngüsü başlığında kapatma parantezi ')' bulunamadı.");
    }
    int increment_expr_end_idx = temp_parser_idx; // Points to RPAREN
    bool increment_is_empty = (increment_expr_start_idx == increment_expr_end_idx);
    
    if(!increment_is_empty) {
        // Temporarily parse increment to correctly position current_token_idx for body parsing
        current_token_idx = increment_expr_start_idx;
        // Check if it's an assignment like i = i + 1, or general expression
        // This is complex, simply use evaluate_expression(false) to parse it.
        // It's not perfect but parse_assignment_rhs is also not directly usable here.
        // This part is only for advancing tokens correctly if !execute.
        if(peek_next_token().type==TOKEN_ASSIGN && peek_token().type==TOKEN_IDENTIFIER){
            consume_token(TOKEN_IDENTIFIER); consume_token(TOKEN_ASSIGN);
            parse_assignment_rhs(VAR_NULL_TYPE,false); // Type doesn't matter for false
        } else {
            evaluate_expression(false);
        }
    }
    
    current_token_idx = increment_expr_end_idx; // current_token_idx is now at RPAREN
    consume_token(TOKEN_RPAREN); // Consumes the ')'
    int body_start_token_idx = current_token_idx; // Start of the for loop's block
    
    loop_depth++;
    bool local_break_flag=false, local_continue_flag=false;
    
    if(execute){
        while(true){
            // 2. Condition
            current_token_idx = condition_expr_start_idx;
            bool condition_result = true; // Empty condition is true
            if(!condition_is_empty){
                Value cond_val = evaluate_expression(true);
                if(cond_val.type != VAL_BOOLEAN) error("For döngüsü koşulu mantıksal (boolean) olmalıdır.");
                condition_result = cond_val.as.bool_val;
            }
            if(!condition_result) break; // Exit loop if condition is false
            
            // 3. Body
            current_token_idx = body_start_token_idx;
            local_continue_flag=false; local_break_flag=false;
            parse_block(true, &local_break_flag, &local_continue_flag, in_function_body);
            
            if(g_return_flag){ // Function returned from loop body
                if(loop_var_declared_here) exit_scope(); // Clean up loop-declared var's scope
                loop_depth--;
                return;
            }
            if(local_break_flag) break; // Break from loop
            
            // 4. Increment (executes even if continue was hit)
            current_token_idx = increment_expr_start_idx;
            if(!increment_is_empty){
                // Re-evaluate the increment logic here
                if(peek_next_token().type==TOKEN_ASSIGN && peek_token().type==TOKEN_IDENTIFIER){
                    Token nit_token = consume_token(TOKEN_IDENTIFIER);
                    consume_token(TOKEN_ASSIGN);
                    Variable* v_for_inc = find_variable(nit_token.lexeme);
                    if(!v_for_inc){char e[100+MAX_IDENT_LEN];sprintf(e,"For döngüsü artırımında '%s' değişkeni tanımsız.",nit_token.lexeme);error(e);}
                    Value rhs_inc_val = parse_assignment_rhs(v_for_inc->type, true);
                    v_for_inc->is_defined = true;
                    switch(v_for_inc->type){
                        case VAR_INT: v_for_inc->value.int_value = rhs_inc_val.as.int_val; break;
                        case VAR_FLOAT: v_for_inc->value.float_value = rhs_inc_val.as.float_val; break;
                        case VAR_STRING: strncpy(v_for_inc->value.string_value, rhs_inc_val.as.string_val, MAX_STRING_LEN-1); v_for_inc->value.string_value[MAX_STRING_LEN-1]='\0'; break;
                        case VAR_BOOLEAN: v_for_inc->value.bool_value = rhs_inc_val.as.bool_val; break;
                        default: /* Should not happen */ break;
                    }
                } else {
                    evaluate_expression(true); // For other increment forms like function_call()
                }
            }
            // After increment, if continue was hit, loop continues to condition.
            if(local_continue_flag) continue; 
        }
    }
    
    // If not executing, or after loop execution, set current_token_idx past the body
    current_token_idx = body_start_token_idx;
    bool dummy_b=false, dummy_c=false;
    parse_block(false, &dummy_b, &dummy_c, in_function_body); // Parse body (don't execute)
    
    loop_depth--;
    if(execute && loop_var_declared_here) exit_scope(); // Clean up scope if var was declared in this for
    
    if(loop_var_declared_here && strlen(declared_loop_var_name)>0){
        // This remove is for the for_loop_vars_stack, not symbol_table if scope handled it.
        // Symbol table cleanup is handled by exit_scope if 'execute' was true.
        // If 'execute' was false, declare_variable wasn't called, but name might be on stack.
        if(for_loop_var_stack_ptr >= 0 && strcmp(for_loop_vars_stack[for_loop_var_stack_ptr], declared_loop_var_name)==0){
            for_loop_var_stack_ptr--;
        }
    }
    if (execute && local_break_flag && break_flag_outer) *break_flag_outer = true;
}
void parse_block(bool execute, bool* break_flag, bool* continue_flag, bool in_function_body) {
    // A block introduces a new scope *unless* it's the direct body of a function,
    // in which case the function call setup (execute_function_call) already created the scope.
    bool local_scope_created_here = false; 
    if(!in_function_body) { // If it's a generic block (if, else, while, for, or standalone { })
        if (execute) { // Only enter/exit scope if the block is actually part of execution path
            enter_scope();
        }
        local_scope_created_here = true; // Mark that this parse_block instance is responsible for scope
    }
    
    consume_token(TOKEN_LBRACE);
    parse_statement_list(execute, break_flag, continue_flag, in_function_body);
    consume_token(TOKEN_RBRACE);
    
    if(local_scope_created_here && execute) {
        exit_scope();
    }
}

void parse_fun_declaration() {
    consume_token(TOKEN_FUN);
    Token func_name_token = consume_token(TOKEN_IDENTIFIER);
    
    if (find_function(func_name_token.lexeme) != NULL) {
        char err[150]; sprintf(err, "'%s' adlı fonksiyon zaten tanımlı.", func_name_token.lexeme); error(err);
    }
    // Check for built-in name conflict
    const char* builtins[] = {"length", "int_to_string", "concat", "sqrt", "to_upper", "to_lower", 
        "read_file_text", "write_file_text", "substring", "string_to_int", 
        "string_to_float", "type_of", "pow", NULL};
        for(int i=0; builtins[i] != NULL; ++i) {
            if(is_keyword(func_name_token.lexeme, builtins[i])) {
                char err[MAX_IDENT_LEN + 100];
                sprintf(err, "'%s' bir dahili komut adıdır, fonksiyon adı olarak kullanılamaz.", func_name_token.lexeme);
                error(err);
            }
        }
        
        
        if (num_functions >= MAX_FUNCTIONS) error("Maksimum fonksiyon sayısına ulaşıldı.");
        
        FunctionDefinition* new_func = &function_table[num_functions];
    strncpy(new_func->name, func_name_token.lexeme, MAX_IDENT_LEN - 1);
    new_func->name[MAX_IDENT_LEN-1] = '\0';
    new_func->num_params = 0;
    
    consume_token(TOKEN_LPAREN);
    if (peek_token().type != TOKEN_RPAREN) { 
        do {
            if (new_func->num_params >= MAX_PARAMETERS) error("Fonksiyon tanımında maksimum parametre sayısı aşıldı.");
            Token param_name_token = consume_token(TOKEN_IDENTIFIER);
            consume_token(TOKEN_COLON);
            VarType param_type = parse_type_specifier();
            if(param_type == VAR_ARRAY || param_type == VAR_VOID) { // Arrays not passed by value, void invalid param type
                error("Fonksiyon parametresi void veya doğrudan array tipinde olamaz (array referansları ileride desteklenebilir).");
            }
            // Check for duplicate parameter names
            for(int k=0; k < new_func->num_params; ++k) {
                if(strcmp(new_func->params[k].name, param_name_token.lexeme) == 0) {
                    char err_param[MAX_IDENT_LEN + 100];
                    sprintf(err_param, "'%s' parametresi fonksiyon tanımında zaten mevcut.", param_name_token.lexeme);
                    error(err_param);
                }
            }
            strncpy(new_func->params[new_func->num_params].name, param_name_token.lexeme, MAX_IDENT_LEN -1);
            new_func->params[new_func->num_params].name[MAX_IDENT_LEN-1]='\0';
            new_func->params[new_func->num_params].type = param_type;
            new_func->num_params++;
            if (peek_token().type == TOKEN_COMMA) consume_token(TOKEN_COMMA); else break;
        } while (true);
    }
    consume_token(TOKEN_RPAREN);
    
    if (peek_token().type == TOKEN_COLON) { 
        consume_token(TOKEN_COLON);
        new_func->return_type = parse_type_specifier();
    } else { 
        new_func->return_type = VAR_VOID; // Default return type is void
    }
    
    if (peek_token().type != TOKEN_LBRACE) error("Fonksiyon tanımında gövde ('{...}') bekleniyor.");
    new_func->body_start_token_idx = current_token_idx; // This will be the index of LBRACE
    
    bool dummy_b=false, dummy_c=false;
    // When parsing function declaration, we are NOT in a function body yet.
    // The 'true' for in_function_body for parse_block here means that parse_block
    // itself won't create another scope layer, as the function's scope will be
    // handled during actual execution by execute_function_call.
    parse_block(false, &dummy_b, &dummy_c, true); // Skip body by parsing with execute=false.
    // 'true' for in_function_body is for the block's internal logic.
    num_functions++;
}

void parse_statement(bool execute, bool* break_flag, bool* continue_flag, bool in_function_body) {
    Token t = peek_token();
    switch (t.type) {
        case TOKEN_VAR: parse_var_declaration(execute, false); break;
        case TOKEN_IDENTIFIER: { 
            // Lookahead to distinguish assignment from expression statement (e.g. function call)
            int initial_idx = current_token_idx;
            int lookahead_idx = initial_idx + 1; // After IDENTIFIER
            
            if (lookahead_idx < num_tokens && tokens[lookahead_idx].type == TOKEN_LBRACKET) { // ident [ ...
                int bracket_nesting = 1;
                lookahead_idx++; // Move past LBRACKET
                while(lookahead_idx < num_tokens && bracket_nesting > 0) {
                    if (tokens[lookahead_idx].type == TOKEN_LBRACKET || tokens[lookahead_idx].type == TOKEN_LPAREN) bracket_nesting++;
                    else if (tokens[lookahead_idx].type == TOKEN_RBRACKET || tokens[lookahead_idx].type == TOKEN_RPAREN) bracket_nesting--;
                    
                    if (bracket_nesting == 0 && tokens[lookahead_idx].type == TOKEN_RBRACKET) break; // Found matching RBRACKET for array index
                    lookahead_idx++;
                }
                if (bracket_nesting == 0 && lookahead_idx < num_tokens) { // Successfully found matching RBRACKET
                    lookahead_idx++; // Move to token *after* RBRACKET
                } else {
                    // Malformed array access or complex expression, assume not assignment for this simple check.
                    // The actual parser (parse_assignment or evaluate_expression) will catch errors.
                    // For safety, reset lookahead_idx to simple case if brackets are complex/mismatched.
                    lookahead_idx = initial_idx + 1; 
                }
            }
            
            if (lookahead_idx < num_tokens && tokens[lookahead_idx].type == TOKEN_ASSIGN) {
                parse_assignment(execute);
            } else {
                // Not an assignment, so it's an expression statement (e.g., function call, var access)
                evaluate_expression(execute); // The value of the expression is discarded
                consume_token(TOKEN_SEMICOLON);
            }
            break;
        }
        case TOKEN_OUT: parse_out_display(execute); break;
        case TOKEN_IF: parse_if_statement(execute, break_flag, continue_flag, in_function_body); break;
        case TOKEN_WHILE: parse_while_statement(execute, break_flag, continue_flag, in_function_body); break;
        case TOKEN_FOR: parse_for_statement(execute, break_flag, continue_flag, in_function_body); break;
        case TOKEN_LBRACE: parse_block(execute, break_flag, continue_flag, in_function_body); break; // Standalone block
        case TOKEN_IMPORT: parse_import_statement(execute); break; 
        case TOKEN_BREAK: 
            consume_token(TOKEN_BREAK);
            consume_token(TOKEN_SEMICOLON);
            if(execute){
                if(loop_depth<=0)error("'break' ifadesi sadece bir döngü içinde kullanılabilir.");
                *break_flag=true;
            }
            break;
        case TOKEN_CONTINUE:
            consume_token(TOKEN_CONTINUE);
            consume_token(TOKEN_SEMICOLON);
            if(execute){
                if(loop_depth<=0)error("'continue' ifadesi sadece bir döngü içinde kullanılabilir.");
                *continue_flag=true;
            }
            break;
        case TOKEN_FUN: error("Fonksiyon tanımı ('fun') sadece en üst düzeyde (global kapsamda) yapılabilir, bir ifade bloğu içinde yapılamaz."); break; 
        case TOKEN_RETURN:
            consume_token(TOKEN_RETURN);
            if (!in_function_body && call_stack_ptr < 0) { // Not in a function body context (call_stack_ptr check for safety)
                error("'return' ifadesi sadece bir fonksiyon gövdesi içinde kullanılabilir.");
            }
            
            Value ret_val = create_value_null(); 
            if (peek_token().type != TOKEN_SEMICOLON) { // return expr;
                ret_val = evaluate_expression(execute);
            }
            consume_token(TOKEN_SEMICOLON);
            
            if (execute) { // Only set return flag if the return statement itself is in an executing path
                g_return_value_holder = ret_val;
                g_return_flag = true;
            }
            break;
        case TOKEN_SEMICOLON: consume_token(TOKEN_SEMICOLON); break; // Empty statement
        default: {char err[150]; sprintf(err,"Deyim başında beklenmedik token: %s ('%s')",token_type_names[t.type],t.lexeme);error(err);}}
}
void parse_statement_list(bool execute, bool* break_flag, bool* continue_flag, bool in_function_body_context) {
    while (peek_token().type != TOKEN_RBRACE && peek_token().type != TOKEN_EOF) {
        parse_statement(execute, break_flag, continue_flag, in_function_body_context);
        
        // If a return, break, or continue was triggered AND we are executing this list
        if (execute && (g_return_flag || *break_flag || *continue_flag)) {
            // Skip remaining statements in this block by parsing them with execute=false
            while (peek_token().type != TOKEN_RBRACE && peek_token().type != TOKEN_EOF) {
                bool dummy_b=false, dummy_c=false; 
                // Pass in_function_body_context for correct parsing of nested returns, etc.
                parse_statement(false, &dummy_b, &dummy_c, in_function_body_context); 
            }
            return; // Exit this statement list processing
        }
    }
}
void parse_import_statement(bool execute) { 
    consume_token(TOKEN_IMPORT); Token file_token =consume_token(TOKEN_STRING_LITERAL); consume_token(TOKEN_SEMICOLON);
    if(execute){
        // Check if already imported
        for(int i=0;i<num_imported_files;++i)if(strcmp(imported_files[i],file_token.string_value)==0)return; // Already imported, do nothing
        
        if(num_imported_files>=MAX_IMPORTS)error("Maksimum import sayısına ('MAX_IMPORTS') ulaşıldı.");
        strncpy(imported_files[num_imported_files++],file_token.string_value,MAX_FILENAME_LEN-1);
        imported_files[num_imported_files-1][MAX_FILENAME_LEN-1]='\0';
        
        // Save current interpreter state
        char* saved_src_code_backup = strdup(source_code); 
        if(!saved_src_code_backup) error("strdup bellek hatası (import için kaynak kod yedekleme).");
        
        Token* saved_tokens_backup = (Token*)malloc(sizeof(Token)*MAX_TOKENS); // MAX_TOKENS should be current num_tokens capacity
        if(!saved_tokens_backup) {free(saved_src_code_backup); error("Import için token belleği ayrılamadı (yedekleme).");}
        memcpy(saved_tokens_backup, tokens, sizeof(Token)*num_tokens); // Save current tokens
        
        int saved_num_tokens = num_tokens;
        int saved_current_token_idx = current_token_idx;
        int saved_current_line = current_line;
        char prev_file_path_for_errors_backup[MAX_FILENAME_LEN]; 
        strncpy(prev_file_path_for_errors_backup, current_file_path_for_errors, MAX_FILENAME_LEN);
        prev_file_path_for_errors_backup[MAX_FILENAME_LEN-1] = '\0';
        
        // Set global pointers for potential cleanup in error() during import
        g_importer_saved_source_code = saved_src_code_backup;
        g_importer_saved_tokens = saved_tokens_backup;
        
        // Load and tokenize the new file
        FILE* import_file_ptr =fopen(file_token.string_value,"r");
        if(!import_file_ptr){
            char err_msg[MAX_STRING_LEN+100];
            sprintf(err_msg,"İçe aktarılacak dosya ('%s') bulunamadı veya okunamadı.",file_token.string_value);
            // Cleanup allocated memory before erroring
            free(g_importer_saved_source_code); g_importer_saved_source_code = NULL;
            free(g_importer_saved_tokens); g_importer_saved_tokens = NULL;
            error(err_msg);
        }
        size_t len_read_import =fread(source_code,1,MAX_SOURCE_SIZE-1,import_file_ptr);
        source_code[len_read_import]='\0';
        fclose(import_file_ptr);
        
        // Note: Global variables like num_functions, num_variables are NOT saved/restored here.
        // Imports modify the global state (add functions, potentially global vars if allowed by language spec).
        // If imports should be isolated, more complex state management is needed.
        // Current model: imports merge their definitions into the global namespace.
        
        tokenize(); // Tokenize the new source_code
        current_token_idx=0; 
        
        interpret_current_file_tokens(file_token.string_value); // Interpret the new tokens
        
        // Restore interpreter state for the original file
        strncpy(source_code,saved_src_code_backup,MAX_SOURCE_SIZE-1);source_code[MAX_SOURCE_SIZE-1]='\0';
        free(saved_src_code_backup); g_importer_saved_source_code = NULL;
        
        memcpy(tokens,saved_tokens_backup,sizeof(Token)*saved_num_tokens); // Restore original tokens
        free(saved_tokens_backup); g_importer_saved_tokens = NULL;
        
        num_tokens = saved_num_tokens;
        current_token_idx = saved_current_token_idx;
        current_line = saved_current_line;
        strncpy(current_file_path_for_errors, prev_file_path_for_errors_backup, MAX_FILENAME_LEN);
    }
}

void interpret_current_file_tokens(const char* filepath_display_name) {
    char previous_filepath_for_errors[MAX_FILENAME_LEN]; 
    strncpy(previous_filepath_for_errors,current_file_path_for_errors,MAX_FILENAME_LEN-1);
    previous_filepath_for_errors[MAX_FILENAME_LEN-1]='\0';
    strncpy(current_file_path_for_errors,filepath_display_name,MAX_FILENAME_LEN-1);
    current_file_path_for_errors[MAX_FILENAME_LEN-1]='\0';
    
    bool top_level_break_flag=false, top_level_continue_flag=false; 
    int previous_loop_depth = loop_depth; loop_depth=0; // Reset loop depth for this file context
    int previous_scope_stack_ptr = scope_stack_ptr; // Save current scope stack state
    // Note: call_stack_ptr is NOT reset here, as imports can happen within function calls.
    
    int initial_token_idx_for_file = current_token_idx; 
    
    // 1. Pass: Parse all top-level function declarations in the current token stream
    while(tokens[current_token_idx].type != TOKEN_EOF) { // Use tokens[idx] to avoid issues if num_tokens changes due to error
        if (tokens[current_token_idx].type == TOKEN_FUN) {
            parse_fun_declaration(); // This advances current_token_idx past the function
        } else {
            // To skip other top-level statements robustly, parse them with execute=false
            // This ensures current_token_idx advances correctly over them.
            int temp_idx_before_skip = current_token_idx;
            bool dummy_b=false, dummy_c=false;
            // false for in_function_body because these are top-level statements
            parse_statement(false, &dummy_b, &dummy_c, false); 
            if (current_token_idx == temp_idx_before_skip && tokens[current_token_idx].type != TOKEN_EOF) {
                // If parse_statement didn't advance (e.g., on an error token it couldn't handle, or empty statement ";"),
                // force advancement to prevent infinite loop.
                current_token_idx++; 
            }
        }
    }
    
    current_token_idx = initial_token_idx_for_file; // Reset for execution pass
    
    bool global_scope_opened_for_this_file = false;
    // A file's top-level execution should have its own global-like scope
    // IF it's the main file or an import NOT happening inside a function call's context.
    // If call_stack_ptr > -1, we are inside a function call, global scope is already established by main or outer file.
    // Top level of a file IS a scope.
    if(call_stack_ptr == -1) { // If not currently executing inside any function
        enter_scope(); 
        global_scope_opened_for_this_file = true;
    }
    
    g_return_flag = false; // Reset global return flag before executing statements of this file
    
    // 2. Pass: Execute all statements (function definitions are skipped by their parsing logic)
    while(tokens[current_token_idx].type != TOKEN_EOF) { 
        if (tokens[current_token_idx].type == TOKEN_FUN) {
            // Skip already parsed function definition. parse_fun_declaration (called in pass 1)
            // already advanced current_token_idx past the function.
            // So, we need a robust way to skip it again here.
            // Calling parse_fun_declaration again would try to re-declare.
            // Instead, manually skip based on structure.
            consume_token(TOKEN_FUN); 
            consume_token(TOKEN_IDENTIFIER); 
            consume_token(TOKEN_LPAREN);
            int paren_level = 1; // For parameters
            while(paren_level > 0 && tokens[current_token_idx].type != TOKEN_EOF) {
                if(tokens[current_token_idx].type == TOKEN_LPAREN) paren_level++;
                else if(tokens[current_token_idx].type == TOKEN_RPAREN) paren_level--;
                
                if (tokens[current_token_idx].type != TOKEN_EOF) consume_token(tokens[current_token_idx].type); // Consume whatever token it is
                else break; // EOF reached unexpectedly
            }
            // if (paren_level != 0) error("Fonksiyon tanımı atlarken eşleşmeyen parantez."); // Should be caught by parser
            
            if(tokens[current_token_idx].type == TOKEN_COLON){ // Optional return type
                consume_token(TOKEN_COLON);
                parse_type_specifier(); // Consumes the type token
            } 
            if(tokens[current_token_idx].type==TOKEN_LBRACE){ // Function body
                bool dummy_b=false,dummy_c=false; 
                parse_block(false,&dummy_b,&dummy_c,true); // Parse block with execute=false, true for in_function_body context
            } else if (tokens[current_token_idx].type != TOKEN_EOF) { // Should be LBRACE or EOF
                error("Fonksiyon gövdesi ('{') bekleniyordu (ikinci geçişte fonksiyon tanımı atlarken).");
            }
            
        } else { // Not a function definition, so it's a statement to execute
            parse_statement(true, &top_level_break_flag, &top_level_continue_flag, false); // false for in_function_body
            
            if(g_return_flag) { // A return statement was executed at top level
                error("'return' ifadesi sadece bir fonksiyon gövdesi içinde kullanılabilir (en üst düzeyde 'return' bulundu).");
            }
            if(top_level_break_flag || top_level_continue_flag){
                char err_msg[200];
                sprintf(err_msg,"'break' veya 'continue' ifadeleri '%s' dosyasının en üst düzeyinde (bir döngü dışında) kullanılamaz.",current_file_path_for_errors);
                error(err_msg);
            }
        }
    }
    
    if (global_scope_opened_for_this_file) {
        exit_scope(); // Close the top-level scope for this file
    }
    // Restore scope_stack_ptr to what it was before this file's interpretation,
    // unless this was the absolute outermost call (prev_ssp == -1 and current ssp is also -1 now).
    // This is tricky. If an import happens, prev_ssp might be 0 (global of importer).
    // The scope stack should naturally unwind.
    // The `scope_stack_ptr = previous_scope_stack_ptr;` line might be problematic if scopes were nested correctly.
    // `exit_scope()` should handle decrementing `scope_stack_ptr`.
    // If `global_scope_opened_for_this_file` was true, `scope_stack_ptr` should now be `previous_scope_stack_ptr`.
    // If it was false, `scope_stack_ptr` should not have been changed by this function's direct enter/exit.
    
    loop_depth = previous_loop_depth; // Restore loop depth for the calling context
    strncpy(current_file_path_for_errors,previous_filepath_for_errors,MAX_FILENAME_LEN-1); 
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Kullanım: %s <dosya_adi.cstar>\n", argv[0]);
        printf("Dosya adı belirtilmedi. Dahili fonksiyon test örneği çalıştırılıyor.\n---\n");
        strcpy(source_code,
               "// --- C* Fonksiyon ve Dahili Komut Testi ---\n"
               "out.display(\"Dahili Fonksiyon Testleri:\");\n"
               "out.display(\"sqrt(16.0) = \" + sqrt(16.0));\n"
               "out.display(\"pow(2.0, 3.0) = \" + pow(2.0, 3.0));\n"
               "out.display(to_upper(\"merhaba dünya\"));\n"
               "out.display(\"substring(\\\"hello world\\\", 6, 5) = \" + substring(\"hello world\", 6, 5));\n"
               "out.display(\"type_of(123) = \" + type_of(123));\n"
               "out.display(\"type_of(null) = \" + type_of(null));\n"
               "var my_str_num: string = \"456.78\";\n"
               "out.display(\"string_to_float(\\\"\"+my_str_num+\"\\\") = \" + string_to_float(my_str_num));\n"
               
               
               "var dosyaya_yaz: boolean = write_file_text(\"test_cikti.txt\", \"Bu C* tarafından yazıldı!\\nMerhaba Tekrar!\\nSayı: \" + (10+5));\n"
               "if(dosyaya_yaz) { out.display(\"test_cikti.txt dosyasına yazıldı.\");}\n"
               "out.display(\"test_cikti.txt içeriği:\");\n"
               "var dosya_icerigi: string = read_file_text(\"test_cikti.txt\");\n" 
               "out.display(dosya_icerigi);\n\n"
               
               "fun selamla(isim: string) {\n" 
               "    out.display(\"Merhaba, \" + isim + \"!\");\n"
               "}\n\n"
               "fun topla(a: int, b: int) : int {\n"
               "    out.display(\"  -> topla fonksiyonu içinde: a=\" + int_to_string(a) + \", b=\" + int_to_string(b));\n"
               "    var sonuc: int = a + b;\n"
               "    out.display(\"  -> topla sonucu: \" + int_to_string(sonuc));\n"
               "    return sonuc;\n"
               "}\n\n"
               "fun fakt(n: int) : int {\n"
               "    if (n <= 1) {\n"
               "        return 1;\n"
               "    }\n"
               "    var rec_call_val: int = fakt(n - 1);\n" // Test intermediate var
               "    return n * rec_call_val;\n" 
               "}\n\n"
               "fun test_scope(dis_param: int) {\n"
               "    var ic_degisken: int = dis_param * 2;\n"
               "    out.display(\"test_scope: dis_param=\" + int_to_string(dis_param) + \", ic_degisken=\" + int_to_string(ic_degisken));\n"
               "    if (dis_param > 5) {\n"
               "        var blok_degiskeni: string = \"Blok içi!\";\n" 
               "        out.display(blok_degiskeni);\n"
               "    }\n"
               // "out.display(blok_degiskeni);" // This would be a scope error
               "}\n\n"
               "selamla(\"ifade olarak çağrı\");\n\n"
               
               
               "out.display(\"Kullanıcı Fonksiyon Testleri:\");\n"
               "selamla(\"C*\");\n"
               "var toplam_sonuc: int = topla(10, 25);\n"
               "out.display(\"Ana programda topla sonucu: \" + int_to_string(toplam_sonuc));\n"
               "var bes_fakt: int = fakt(5);\n"
               "out.display(\"5 Faktöriyel = \" + int_to_string(bes_fakt));\n"
               "test_scope(3);\n"
               "test_scope(7);\n"
               
               "import \"math_lib_func.cstar\";\n"
               "out.display(\"PI kütüphaneden: \" + math_pi_func());\n"
               "out.display(\"5 karesi kütüphaneden: \" + math_kare_func(5.0));\n"
               
               "fun bos_donus_test() : void { \n"
               "   out.display(\"Bos donus test fonksiyonu ici.\");\n"
               "   return; \n" // Explicit return from void
               "   out.display(\"Bu satır çalışmamalı.\");\n"
               "}\n"
               "bos_donus_test();\n"
               "fun implicit_void_return_test() { \n" // Implicit void return type
               "   out.display(\"Implicit void return test (dönüş tipi belirtilmedi).\");\n"
               "}\n" // Implicit return at end of void function
               "implicit_void_return_test();\n"
               
               "fun test_ret_no_val_fail() : int { \n"
               "    out.display(\"Bu fonksiyon int dönmeli ama return; ile dönecek (hata bekleniyor).\");\n"
               "    // return; // Hata: 'test_ret_no_val_fail' fonksiyonu değer döndürmeliydi (int) ama return ifadesi bulunamadı\n"
               "    return 1; // Hata vermemesi için düzeltildi\n"
               "}\n"
               "// test_ret_no_val_fail(); \n" // Error if uncommented without fix.
               
               "out.display(\"Testler tamamlandı.\");\n"
        );
        strncpy(current_file_path_for_errors, "dahili_ornek.cstar", MAX_FILENAME_LEN-1);
        current_file_path_for_errors[MAX_FILENAME_LEN-1] = '\0';
        
        FILE* lib_file_func = fopen("math_lib_func.cstar", "w");
        if(lib_file_func){
            fprintf(lib_file_func, 
                    "out.display(\"  -> math_lib_func.cstar yükleniyor...\");\n"
                    "var lib_var_test: string = \"Bu bir kütüphane değişkeni\";\n" // Test global var from lib
                    "fun math_pi_func() : float { return 3.14159; }\n"
                    "fun math_kare_func(x: float) : float { return x * x; }\n"
                    "out.display(\"  -> math_lib_func.cstar fonksiyonları tanımlandı. lib_var_test = \" + lib_var_test);\n"
            );
            fclose(lib_file_func);
        } else {
            fprintf(stderr, "Uyarı: Dahili test için math_lib_func.cstar dosyası oluşturulamadı.\n");
        }
        
    } else {
        FILE *file = fopen(argv[1], "r");
        if (!file) {perror("Dosya açma hatası"); return 1;}
        size_t len_read = fread(source_code, 1, MAX_SOURCE_SIZE - 1, file); source_code[len_read] = '\0'; fclose(file);
        printf("--- '%s' dosyası çalıştırılıyor ---\n", argv[1]);
        strncpy(current_file_path_for_errors, argv[1], MAX_FILENAME_LEN-1); current_file_path_for_errors[MAX_FILENAME_LEN-1] = '\0';
    }
    
    // Initialize global states before first interpretation
    num_variables = 0; num_functions = 0;
    for_loop_var_stack_ptr = -1; call_stack_ptr = -1; scope_stack_ptr = -1; 
    num_imported_files = 0; 
    g_importer_saved_source_code = NULL; g_importer_saved_tokens = NULL;
    g_return_flag = false; 
    
    tokenize(); 
    current_token_idx = 0; 
    
    printf("--- Program Çıktısı ---\n");
    interpret_current_file_tokens(current_file_path_for_errors); 
    printf("--- Program Çıktısı Sonu ---\n");
    
    // Final cleanup (mostly for arrays in the very last global scope if any remain)
    // exit_scope called by interpret_current_file_tokens should handle most.
    while(scope_stack_ptr >=0) { // Ensure all scopes are exited
        exit_scope();
    }
    
    return 0;
}
