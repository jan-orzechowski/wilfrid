#include "utils.h"
#include "lexing.h"

void print_token_kind(token_kind kind)
{
    switch (kind)
    {
        case TOKEN_EOF: { printf("EOF"); }; break;
        case TOKEN_INT: { printf("INT"); }; break;
        case TOKEN_NAME: { printf("NAME"); }; break;
        case TOKEN_GT: { printf(">"); }; break;
        case TOKEN_LT: { printf("<"); }; break;
        case TOKEN_GEQ: { printf(">="); }; break;
        case TOKEN_LEQ: { printf("<="); }; break; 
        case TOKEN_EQ: { printf("=="); }; break;
        case TOKEN_NEQ: { printf("!="); }; break;
        case TOKEN_AND: { printf("AND"); }; break;
        case TOKEN_OR: { printf("OR"); }; break;
        default:{ printf("%.1s", &(char)kind); }; break;
    }
}

const char* struct_keyword;
const char* enum_keyword;
const char* union_keyword;
const char* let_keyword;
const char* fn_keyword;
const char* sizeof_keyword;
const char* break_keyword;
const char* continue_keyword;
const char* return_keyword;
const char* if_keyword;
const char* else_keyword;
const char* while_keyword;
const char* do_keyword;
const char* for_keyword;
const char* switch_keyword;
const char* case_keyword;
const char* default_keyword;

const char* first_keyword;
const char* last_keyword;
char** keywords_list;

#define KEYWORD(name) name##_keyword = str_intern(#name); buf_push(keywords_list, name##_keyword)

void init_keywords()
{
    static bool initialized = false;
    if (false == initialized)
    {        
        KEYWORD(struct);
        KEYWORD(enum);
        KEYWORD(union);
        KEYWORD(let);
        KEYWORD(fn);
        KEYWORD(sizeof);
        KEYWORD(break);
        KEYWORD(continue);
        KEYWORD(return);
        KEYWORD(if);
        KEYWORD(else);
        KEYWORD(while);
        KEYWORD(do);
        KEYWORD(for);
        KEYWORD(switch);
        KEYWORD(case);
        KEYWORD(default);
    }
    first_keyword = struct_keyword;
    last_keyword = default_keyword;
    initialized = true;
}

bool is_name_keyword(const char* name)
{
    bool result = (name >= first_keyword && name <= last_keyword);
    return result;
}

char* stream;
tok token;
tok** all_tokens;
int lexed_token_index;

void next_token()
{
    bool discard_token = false;
    token.start = stream;
    switch (*stream)
    {
        case '0': case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
        {
            // idziemy po następnych
            int val = 0;
            while (isdigit(*stream))
            {
                val *= 10; // dotychczasową wartość traktujemy jako 10 razy większą - bo znaleźliśmy kolejne miejsce dziesiętne
                val += *stream++ - '0'; // przerabiamy char na integer
            }
            //stream--; // w ostatnim przejściu pętli posunęliśmy się o 1 za daleko
            token.kind = TOKEN_INT;
            token.val = val;
        }
        break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
        case 's': case 't': case 'u': case 'v': case 'w': case 'x':
        case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
        case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
        case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
        case 'Y': case 'Z':
        case '_':
        {
            // zaczęliśmy od litery - dalej idziemy po cyfrach, literach i _
            while (isalnum(*stream) || *stream == '_')
            {
                stream++;
            }
            token.name = str_intern_range(token.start, stream);
            if (is_name_keyword(token.name))
            {
                token.kind = TOKEN_KEYWORD;
            }
            else
            {
                token.kind = TOKEN_NAME;
            }
        }
        break;
        case '+':
        {
            token.kind = '+';
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '-':
        {
            token.kind = '-';
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '*':
        {
            token.kind = '*';
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '/':
        {
            token.kind = '/';
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '(':
        {
            token.kind = '(';
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case ')':
        {
            token.kind = ')';
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '<':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_LEQ;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = TOKEN_LT;
                token.name = str_intern_range(token.start, stream);
            }            
        }
        break;
        case '>':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_GEQ;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = TOKEN_GT;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '|':
        {
            if (*(stream + 1) == '|')
            {
                stream += 2;
                token.kind = TOKEN_OR;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = '|';
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '&':
        {
            if (*(stream + 1) == '&')
            {
                stream += 2;
                token.kind = TOKEN_AND;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = '&';
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '!':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_NEQ;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = '!';
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '=':
        {
            if (*(stream + 1) == '==')
            {
                stream += 2;
                token.kind = TOKEN_EQ;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = '=';
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case ' ':
        {
            discard_token = true;
            stream++;
        }
        break;
        default:
        {
            token.kind = *stream++;
        }
        break;
    }

    token.end = stream;    

    if (false == discard_token)
    {
        tok* new_tok = xmalloc(sizeof(token));
        memcpy(new_tok, &token, sizeof(token));
        buf_push(all_tokens, new_tok);
    }
}

void init_stream(char* str)
{
    init_keywords();

    stream = str;
    buf_free(all_tokens);
    next_token();
}

void get_first_lexed_token()
{
    token = *all_tokens[0];
    lexed_token_index = 1;
}

void next_lexed_token()
{
    if (lexed_token_index + 1 < buf_len(all_tokens))
    {
        tok* next_token = all_tokens[lexed_token_index];
        if (next_token)
        {
            token = *next_token;
            lexed_token_index++;
        }
    }
    else
    {
        token.kind = TOKEN_EOF;
    }
}