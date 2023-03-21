#include "utils.h"
#include "lexing.h"

const char *get_token_kind_name(token_kind kind)
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
    const char *name = get_token_kind_name(kind);
    if (name)
    {
         printf("%s", name);
    }
    else
    {
        printf("TOKEN UNKNOWN: %.1s", &(char)kind);
    }
}

const char *struct_keyword;
const char *enum_keyword;
const char *union_keyword;
const char *let_keyword;
const char *fn_keyword;
const char *const_keyword;
const char *new_keyword;
const char *auto_keyword;
const char *delete_keyword;
const char *sizeof_keyword;
const char *null_keyword;
const char *true_keyword;
const char *false_keyword;
const char *break_keyword;
const char *continue_keyword;
const char *return_keyword;
const char *if_keyword;
const char *else_keyword;
const char *while_keyword;
const char *do_keyword;
const char *for_keyword;
const char *switch_keyword;
const char *case_keyword;
const char *default_keyword;
const char *variadic_keyword;
const char *extern_keyword;

const char *first_keyword;
const char *last_keyword;
const char **keywords_list;

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
        KEYWORD(sizeof);
        KEYWORD(const);
        KEYWORD(new);
        KEYWORD(auto);
        KEYWORD(delete);
        KEYWORD(null);
        KEYWORD(true);
        KEYWORD(false);
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
        KEYWORD(variadic);
        KEYWORD(extern);
    }
    first_keyword = struct_keyword;
    last_keyword = extern_keyword;
    initialized = true;
}

bool is_name_keyword(const char *name)
{
    bool result = (name >= first_keyword && name <= last_keyword);
    return result;
}

char *stream;
token tok;
token **all_tokens;
int lexed_token_index;

size_t nested_comments_level;
char *current_line_beginning;

bool lex_next_token(void)
{
    bool discard_token = false;
    bool unexpected_character = false;

    tok.start = stream;
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
            tok.kind = TOKEN_INT;
            tok.val = val;
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
            tok.name = str_intern_range(tok.start, stream);
            if (is_name_keyword(tok.name))
            {
                tok.kind = TOKEN_KEYWORD;
            }
            else
            {
                tok.kind = TOKEN_NAME;
            }
        }
        break;
        case '+':
        {
            if (*(stream + 1) == '+')
            {
                stream += 2;
                tok.kind = TOKEN_INC;
                tok.name = str_intern_range(tok.start, stream);
            }
            else if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_ADD_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                stream++;
                tok.kind = TOKEN_ADD;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case '-':
        {          
            if (*(stream + 1) == '-')
            {
                stream += 2;
                tok.kind = TOKEN_DEC;
                tok.name = str_intern_range(tok.start, stream);
            }
            else if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_SUB_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                stream++;
                tok.kind = TOKEN_SUB;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case '*':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_MUL_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                tok.kind = TOKEN_MUL;
                stream++;
                tok.name = str_intern_range(tok.start, stream);
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
                tok.kind = TOKEN_DIV_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
            else if (*(stream + 1) == '*')
            {
                discard_token = true;
                nested_comments_level++;
                stream += 2;

                for (;;)
                {
                    stream++;
                    if (*(stream) == 0)
                    {
                        break;
                    }

                    if (*(stream) == '/' && *(stream + 1) == '*')
                    {
                        nested_comments_level++;
                        stream += 2;
                    }

                    if (*(stream) == '*' && *(stream + 1) == '/')
                    {
                        nested_comments_level--;
                        stream += 2;
                    }

                    if (nested_comments_level == 0)
                    {
                        break;
                    }
                }
            }
            else
            {
                tok.kind = TOKEN_DIV;
                stream++;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case '"':
        {
            tok.kind = TOKEN_STRING;
            stream++;
            for (;;)
            {
                stream++;
                if (*(stream) == 0 || *(stream) == '"')
                {
                    tok.string_val = str_intern_range(tok.start + 1, stream);
                    if (*(stream) == '"')
                    {
                        stream++;
                    }
                    break;
                }
            }
        }
        break;
        case '(':
        {
            tok.kind = TOKEN_LEFT_PAREN;
            stream++;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case ')':
        {
            tok.kind = TOKEN_RIGHT_PAREN;
            stream++;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '{':
        {
            tok.kind = TOKEN_LEFT_BRACE;
            stream++;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '}':
        {
            tok.kind = TOKEN_RIGHT_BRACE;
            stream++;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case ',':
        {
            tok.kind = TOKEN_COMMA;
            stream++;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '.':
        {
            tok.kind = TOKEN_DOT;
            stream++;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '%':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_MOD_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                tok.kind = TOKEN_MOD;
                stream++;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case ';':
        {
            // usuwamy i zastępujemy whitespace
            *(stream) = ' ';
            discard_token = true;
            warning("Semicolons are ignored", tok.pos, 1);
        }
        break;
        case '[':
        {
            tok.kind = TOKEN_LEFT_BRACKET;
            stream++;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case ']':
        {
            tok.kind = TOKEN_RIGHT_BRACKET;
            stream++;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '<':
        {
            if (*(stream + 1) == '=')
            {              
                stream += 2;
                tok.kind = TOKEN_LEQ;
                tok.name = str_intern_range(tok.start, stream);                
            }
            else if (*(stream + 1) == '<')
            {
                if (*(stream + 1) == '=')
                {
                    stream += 3;
                    tok.kind = TOKEN_LEFT_SHIFT_ASSIGN;
                    tok.name = str_intern_range(tok.start, stream);
                }
                else
                {
                    stream += 2;
                    tok.kind = TOKEN_LEFT_SHIFT;
                    tok.name = str_intern_range(tok.start, stream);
                }
            }
            else
            {
                stream++;
                tok.kind = TOKEN_LT;
                tok.name = str_intern_range(tok.start, stream);
            }            
        }
        break;
        case '>':
        {
            if (*(stream + 1) == '=')
            {             
                stream += 2;
                tok.kind = TOKEN_GEQ;
                tok.name = str_intern_range(tok.start, stream);
            }
            else if (*(stream + 1) == '>')
            {
                if (*(stream + 1) == '=')
                {
                    stream += 3;
                    tok.kind = TOKEN_RIGHT_SHIFT_ASSIGN;
                    tok.name = str_intern_range(tok.start, stream);
                }
                else
                {
                    stream += 2;
                    tok.kind = TOKEN_RIGHT_SHIFT;
                    tok.name = str_intern_range(tok.start, stream);
                }              
            }
            else
            {
                stream++;
                tok.kind = TOKEN_GT;
                tok.name = str_intern_range(tok.start, stream);
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
                    tok.kind = TOKEN_OR_ASSIGN;
                    tok.name = str_intern_range(tok.start, stream);
                }
                else
                {
                    stream += 2;
                    tok.kind = TOKEN_OR;
                    tok.name = str_intern_range(tok.start, stream);
                }
            }
            else if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_BITWISE_OR_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                stream++;
                tok.kind = TOKEN_BITWISE_OR;
                tok.name = str_intern_range(tok.start, stream);
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
                    tok.kind = TOKEN_AND_ASSIGN;
                    tok.name = str_intern_range(tok.start, stream);
                }
                else
                {
                    stream += 2;
                    tok.kind = TOKEN_AND;
                    tok.name = str_intern_range(tok.start, stream);
                }               
            }
            else if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_BITWISE_AND_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                stream++;
                tok.kind = TOKEN_BITWISE_AND;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case '^':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_XOR_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                stream++;
                tok.kind = TOKEN_XOR;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case '!':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_NEQ;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                stream++;
                tok.kind = TOKEN_NOT;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case '~':
        {
            stream++;
            tok.kind = TOKEN_BITWISE_NOT;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '=':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_EQ;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                stream++;
                tok.kind = TOKEN_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case ':':
        {
            if (*(stream + 1) == '=')
            {
                stream += 2;
                tok.kind = TOKEN_COLON_ASSIGN;
                tok.name = str_intern_range(tok.start, stream);
            }
            else
            {
                stream++;
                tok.kind = TOKEN_COLON;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case '?':
        {
            stream++;
            tok.kind = TOKEN_QUESTION;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '\n':
        {
            tok.kind = TOKEN_NEWLINE;
            tok.pos.line++;
            current_line_beginning = stream + 1;
            stream++;
        }
        break;
        case '\r':
        case ' ':
        case '\t': 
        case '\v':
        {
            discard_token = true;
            stream++;
        }
        break;
        case '\0':
        {
            tok.kind = TOKEN_EOF;
        }
        break;
        default:
        {
            discard_token = true;
            unexpected_character = true;
            stream++;
        }
        break;
    }

    tok.pos.character = tok.start - current_line_beginning + 1LL;
    tok.end = stream;

    if (unexpected_character)
    {
        error(xprintf("Unexpected character: %c", *(stream - 1)), tok.pos, 1);
        unexpected_character = false;
    }

    if (false == discard_token)
    {
        token *new_tok = xmalloc(sizeof(token));
        memcpy(new_tok, &tok, sizeof(token));
        buf_push(all_tokens, new_tok);
    }

    assert((tok.kind != TOKEN_NAME && tok.kind != TOKEN_KEYWORD) 
        || tok.name == str_intern(tok.name));

    bool is_at_end = (tok.kind == TOKEN_EOF && false == discard_token);
    return (false == is_at_end);
}

void init_stream(char *source, char *filename)
{
    init_keywords(); 
    stream = source;
    current_line_beginning = stream;
    tok.pos.filename = filename ? filename : "<string>";
    tok.pos.line = 1;
    tok.pos.character = 1;
    buf_free(all_tokens);
    lex_next_token();
}

void next_lexed_token(void)
{
    if (lexed_token_index + 1 < buf_len(all_tokens))
    {
        token *next_token = all_tokens[lexed_token_index];
        if (next_token)
        {
            tok = *next_token;
            lexed_token_index++;
        }
        if (next_token->kind == TOKEN_NEWLINE)
        {
            next_lexed_token();
        }
    }
    else
    {
        tok = *all_tokens[lexed_token_index];
    }
}

void ignore_tokens_until_newline(void)
{
    while (lexed_token_index + 1 < buf_len(all_tokens))
    {
        if (all_tokens[lexed_token_index]->kind == TOKEN_NEWLINE)
        {
            lexed_token_index++;
            // przypadek wielu newlines pod rząd
            while (lexed_token_index + 1 < buf_len(all_tokens))
            {
                if (all_tokens[lexed_token_index]->kind == TOKEN_NEWLINE)
                {
                    lexed_token_index++;
                }
                else
                {
                    break;
                }
            }

            tok = *all_tokens[lexed_token_index];
            lexed_token_index++;
            break;
        }
        else if (all_tokens[lexed_token_index]->kind == TOKEN_EOF)
        {
            break;
        }

        lexed_token_index++;
    }
}

void ignore_tokens_until_next_block(void)
{
    while (lexed_token_index + 1 < buf_len(all_tokens))
    {
        if (all_tokens[lexed_token_index]->kind == TOKEN_RIGHT_BRACE)
        {
            next_lexed_token();
            break;
        }
        else
        {
            next_lexed_token();
        }
    }
}

void lex(char *source, char *filename)
{
    init_stream(filename, source);

    while (lex_next_token());

    tok = *all_tokens[0];
    lexed_token_index = 1;
}