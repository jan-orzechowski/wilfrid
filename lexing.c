#include "utils.h"
#include "lexing.h"

const char* get_token_kind_name(token_kind kind)
{
    if (kind < sizeof(token_kind_names) / sizeof(*token_kind_names))
    {
        return token_kind_names[kind];
    }
    else
    {
        return 0;
    }
}

void print_token_kind(token_kind kind)
{
    const char* name = get_token_kind_name(kind);
    if (name)
    {
         printf("%s", name);
    }
    else
    {
        printf("TOKEN UNKNOWN: %.1s", &(char)kind);
    }
}

const char* struct_keyword;
const char* enum_keyword;
const char* union_keyword;
const char* let_keyword;
const char* fn_keyword;
const char* typedef_keyword;
const char* sizeof_keyword;
const char* const_keyword;
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
const char** keywords_list;

#define KEYWORD(name) name##_keyword = str_intern(#name); buf_push(keywords_list, name##_keyword)

void init_keywords(void)
{
    static bool initialized = false;
    if (false == initialized)
    {        
        KEYWORD(struct);
        KEYWORD(enum);
        KEYWORD(union);
        KEYWORD(let);
        KEYWORD(fn);
        KEYWORD(typedef);
        KEYWORD(sizeof);
        KEYWORD(const);
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

bool next_token(void)
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
            if (*(stream + 1) == '+')
            {
                stream += 2;
                token.kind = TOKEN_INC;
                token.name = str_intern_range(token.start, stream);
            }
            else if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_ADD_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = TOKEN_ADD;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '-':
        {          
            if (*(stream + 1) == '-')
            {
                stream += 2;
                token.kind = TOKEN_DEC;
                token.name = str_intern_range(token.start, stream);
            }
            else if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_SUB_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = TOKEN_SUB;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '*':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_MUL_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                token.kind = TOKEN_MUL;
                stream++;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '/':
        {
            if (*(stream + 1) == '/')
            {
                discard_token = true;
                stream += 1;
                for (;;)
                {
                    stream++;
                    if (*(stream) == 0)
                    {
                        break;
                    }

                    if (*(stream) == '\n')
                    {
                        stream += 1;
                        break;
                    }
                }
            }
            else if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_DIV_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
            else if (*(stream + 1) == '*')
            {
                discard_token = true;
                stream += 2;
                for (;;)
                {
                    stream++;
                    if (*(stream) == 0)
                    {
                        break;
                    }

                    if (*(stream) == '*' && *(stream + 1) == '/')
                    {
                        stream += 2;
                        break;
                    }
                }
            }
            else
            {
                token.kind = TOKEN_DIV;
                stream++;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '"':
        {
            token.kind = TOKEN_STRING;
            stream++;
            for (;;)
            {
                stream++;
                if (*(stream) == 0)
                {
                    break;
                }

                if (*(stream) == '"')
                {
                    token.string_val = str_intern_range(token.start + 1, stream);
                    stream++;
                    break;
                }
            }
        }
        break;
        case '(':
        {
            token.kind = TOKEN_LEFT_PAREN;
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case ')':
        {
            token.kind = TOKEN_RIGHT_PAREN;
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '{':
        {
            token.kind = TOKEN_LEFT_BRACE;
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '}':
        {
            token.kind = TOKEN_RIGHT_BRACE;
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case ',':
        {
            token.kind = TOKEN_COMMA;
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '.':
        {
            token.kind = TOKEN_DOT;
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '%':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_MOD_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                token.kind = TOKEN_MOD;
                stream++;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case ';':
        {
            token.kind = TOKEN_SEMICOLON;
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '[':
        {
            token.kind = TOKEN_LEFT_BRACKET;
            stream++;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case ']':
        {
            token.kind = TOKEN_RIGHT_BRACKET;
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
            else if (*(stream + 1) == '<')
            {
                if (*(stream + 1) == '=')
                {
                    stream += 3;
                    token.kind = TOKEN_LEFT_SHIFT_ASSIGN;
                    token.name = str_intern_range(token.start, stream);
                }
                else
                {
                    stream += 2;
                    token.kind = TOKEN_LEFT_SHIFT;
                    token.name = str_intern_range(token.start, stream);
                }
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
            else if (*(stream + 1) == '>')
            {
                if (*(stream + 1) == '=')
                {
                    stream += 3;
                    token.kind = TOKEN_RIGHT_SHIFT_ASSIGN;
                    token.name = str_intern_range(token.start, stream);
                }
                else
                {
                    stream += 2;
                    token.kind = TOKEN_RIGHT_SHIFT;
                    token.name = str_intern_range(token.start, stream);
                }              
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
                if (*(stream + 2) == '=')
                {
                    stream += 3;
                    token.kind = TOKEN_OR_ASSIGN;
                    token.name = str_intern_range(token.start, stream);
                }
                else
                {
                    stream += 2;
                    token.kind = TOKEN_OR;
                    token.name = str_intern_range(token.start, stream);
                }
            }
            else if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_BITWISE_OR_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = TOKEN_BITWISE_OR;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '&':
        {
            if (*(stream + 1) == '&')
            {
                if (*(stream + 2) == '=')
                {
                    stream += 3;
                    token.kind = TOKEN_AND_ASSIGN;
                    token.name = str_intern_range(token.start, stream);
                }
                else
                {
                    stream += 2;
                    token.kind = TOKEN_AND;
                    token.name = str_intern_range(token.start, stream);
                }               
            }
            else if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_BITWISE_AND_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = TOKEN_BITWISE_AND;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '^':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_XOR_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = TOKEN_XOR;
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
                token.kind = TOKEN_NOT;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '~':
        {
            stream++;
            token.kind = TOKEN_BITWISE_NOT;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '=':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_EQ;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = TOKEN_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case ':':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                token.kind = TOKEN_COLON_ASSIGN;
                token.name = str_intern_range(token.start, stream);
            }
            else
            {
                stream++;
                token.kind = TOKEN_COLON;
                token.name = str_intern_range(token.start, stream);
            }
        }
        break;
        case '?':
        {
            stream++;
            token.kind = TOKEN_QUESTION;
            token.name = str_intern_range(token.start, stream);
        }
        break;
        case '\n':
        case '\r':
        case ' ':
        case '\t': 
        case '\v':
        {
            discard_token = true;            
            if (*stream == '\n')
            {
                token.pos.line++;
            }
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

    bool is_at_end = (token.kind == TOKEN_EOF && false == discard_token);
    return (false == is_at_end);
}

void init_stream(char* filename, char* str)
{
    init_keywords(); 
    stream = str;
    token.pos.filename = filename ? filename : "<string>";
    token.pos.line = 1;
    buf_free(all_tokens);
    next_token();
}

void get_first_lexed_token(void)
{
    token = *all_tokens[0];
    lexed_token_index = 1;
}

void next_lexed_token(void)
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

void lex(char* filename, char* test)
{
    init_stream(filename, test);
    while (next_token());
    get_first_lexed_token();
}