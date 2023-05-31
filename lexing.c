#include "utils.h"
#include "tokens.h"
#include "keywords.h"

char *stream;
token tok;
token *all_tokens;
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
        case '0': 
        {            
            // hexadecimal number
            if (*(stream + 1) == 'x' || *(stream + 1) == 'X')
            {
                stream += 2;
                uint64_t val = 0;
                while (true)
                {
                    if (*stream == '_')
                    {
                        stream++;
                    }
                    else
                    {
                        uint8_t digit = char_to_digit[(unsigned char)*stream];
                        if (digit == 0 && *stream != '0')
                        {
                            break;
                        }
                        val *= 16;
                        val += digit;
                        stream++;
                    }
                }
                tok.kind = TOKEN_INT;
                tok.uint_val = val;
                break;
            }

            // binary number
            if (*(stream + 1) == 'b' || *(stream + 1) == 'B')
            {
                stream += 2;
                uint64_t val = 0;
                while (*stream == '0' || *stream == '1' || *stream == '_')
                {
                    if (*stream == '_')
                    {
                        stream++;
                    }
                    else
                    {
                        uint8_t digit = char_to_digit[(unsigned char)*stream];
                        if (digit == 0 && *stream != '0')
                        {
                            break;
                        }
                        val *= 2;
                        val += digit;
                        stream++;
                    }
                }
                tok.kind = TOKEN_INT;
                tok.uint_val = val;
                break;
            }
        } 
        // intentional fallthrough
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
        {
            uint64_t val = 0;

            while (isdigit(*stream) || *stream == '_')
            {
                if (*stream == '_')
                {
                    stream++;
                }
                else
                {
                    uint8_t digit = char_to_digit[(uint8_t)*stream];
                    val *= 10;
                    val += digit;
                    stream++;
                }
            }

            if (*stream == '.')
            {
                stream++;
                double float_val = (double)val;
                size_t decimal_place = 0;
                while (isdigit(*stream) || *stream == '_')
                {
                    if (*stream == '_')
                    {
                        stream++;
                    }
                    else
                    {
                        decimal_place++;
                        double digit = (double)char_to_digit[(uint8_t)*stream];
                        digit /= pow(10, decimal_place);
                        float_val += digit;
                        stream++;
                    }
                }
                tok.kind = TOKEN_FLOAT;
                tok.float_val = float_val;
            }
            else
            {
                tok.kind = TOKEN_INT;
                tok.uint_val = val;
            }          
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
                stream++;
                tok.kind = TOKEN_MUL;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case '/':
        {
            if (*(stream + 1) == '/')
            {
                stream += 1;
                discard_token = true;
                for (;;)
                {
                    stream++;
                    if (*(stream) == 0)
                    {
                        break;
                    }

                    if (*(stream) == '\n')
                    {
                        stream++;
                        current_line_beginning = stream;
                        tok.pos.line++;
                        break;
                    }
                }
            }         
            else if (*(stream + 1) == '*')
            {
                stream += 2;
                discard_token = true;
                nested_comments_level++;

                for (;;)
                {
                    stream++;
                    if (*(stream) == 0)
                    {
                        break;
                    }

                    if (*(stream) == '\n')
                    {
                        stream++;
                        current_line_beginning = stream;                        
                        tok.pos.line++;
                    }

                    if (*(stream) == '/' && *(stream + 1) == '*')
                    {
                        stream += 2;
                        nested_comments_level++;
                    }

                    if (*(stream) == '*' && *(stream + 1) == '/')
                    {
                        stream += 2;
                        nested_comments_level--;
                    }

                    if (nested_comments_level == 0)
                    {
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
            else
            {
                stream++;
                tok.kind = TOKEN_DIV;
                tok.name = str_intern_range(tok.start, stream);
            }
        }
        break;
        case '"':
        {
            stream++;
            tok.kind = TOKEN_STRING;
            for (;;)
            {
                stream++;

                if (*stream == '\n')
                {
                    current_line_beginning = stream;
                    tok.pos.line++;
                    continue;
                }

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
        case '\'':
        {
            stream++;

            tok.kind = TOKEN_CHAR;
            
            char *begin = stream;            
            while (*stream && *stream != '\'' && *stream != '\n')
            {
                stream++;
            }
            
            char *end = stream;

            if (*stream && *stream == '\'')
            {
                stream++;
            }
            else
            {
                error("Character literal without matching ' sign", tok.pos, 1);
            }
            
            if (end - begin > 2 && *begin != '\\')
            {
                error("Character literals can only be one character long", tok.pos, 1);
            }

            tok.string_val = str_intern_range(begin, end);
        }
        break;
        case '(':
        {
            stream++;
            tok.kind = TOKEN_LEFT_PAREN;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case ')':
        {
            stream++;
            tok.kind = TOKEN_RIGHT_PAREN;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '{':
        {
            stream++;
            tok.kind = TOKEN_LEFT_BRACE;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '}':
        {
            stream++;
            tok.kind = TOKEN_RIGHT_BRACE;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case ',':
        {
            stream++;
            tok.kind = TOKEN_COMMA;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case '.':
        {
            stream++;
            tok.kind = TOKEN_DOT;
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
                stream++;
                tok.kind = TOKEN_MOD;
                tok.name = str_intern_range(tok.start, stream);
            }
        }      
        break;
        case '[':
        {
            stream++;
            tok.kind = TOKEN_LEFT_BRACKET;
            tok.name = str_intern_range(tok.start, stream);
        }
        break;
        case ']':
        {
            stream++;
            tok.kind = TOKEN_RIGHT_BRACKET;
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
                if (*(stream + 2) == '=')
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
                if (*(stream + 2) == '=')
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
            stream++;
            current_line_beginning = stream;
            tok.pos.line++;
            tok.kind = TOKEN_NEWLINE;
        }
        break;
        case '\r':
        case ' ':
        case '\t': 
        case '\v':
        {
            stream++;
            discard_token = true;
        }
        break;
        case '\0':
        {
            tok.kind = TOKEN_EOF;
        }
        break;
        case ';':
        default:
        {
            stream++;
            discard_token = true;
            unexpected_character = true;
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
        buf_push(all_tokens, tok);
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
    nested_comments_level = 0;
    lexed_token_index = 0;

    tok = (token){0};
    tok.pos.filename = filename ? filename : "<string>";
    tok.pos.line = 1;
    tok.pos.character = 1;

    buf_free(all_tokens);
    
    if (source)
    {
        lex_next_token();
    }
}

void next_token(void)
{
    if (lexed_token_index + 1 < buf_len(all_tokens))
    {
        token *next = &all_tokens[lexed_token_index];
        if (next)
        {
            tok = *next;
            lexed_token_index++;
        }
        if (next->kind == TOKEN_NEWLINE)
        {
            next_token();
        }
    }
    else
    {
        tok = all_tokens[lexed_token_index];
    }
}

bool was_previous_token_newline(void)
{
    if (lexed_token_index > 2)
    {
        bool result = (all_tokens[lexed_token_index - 2].kind == TOKEN_NEWLINE);
        return result;
    }
    else
    {
        return false;
    }
}

void ignore_tokens_until_newline(void)
{
    while (lexed_token_index + 1 < buf_len(all_tokens))
    {
        if (all_tokens[lexed_token_index].kind == TOKEN_NEWLINE)
        {
            lexed_token_index++;
            // przypadek wielu newlines pod rząd
            while (lexed_token_index + 1 < buf_len(all_tokens))
            {
                if (all_tokens[lexed_token_index].kind == TOKEN_NEWLINE)
                {
                    lexed_token_index++;
                }
                else
                {
                    break;
                }
            }

            tok = all_tokens[lexed_token_index];
            lexed_token_index++;
            break;
        }
        else if (all_tokens[lexed_token_index].kind == TOKEN_EOF)
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
        if (all_tokens[lexed_token_index].kind == TOKEN_RIGHT_BRACE)
        {
            next_token();
            break;
        }
        else
        {
            next_token();
        }
    }
}

void lex(char *source, char *filename)
{
    init_stream(source, filename);

    if (source == null)
    {
        return;
    }

    while (lex_next_token());

    tok = all_tokens[0];
    lexed_token_index = 1;
}