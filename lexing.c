#include "utils.h"
#include "lexing.h"

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
            token.kind = TOKEN_NAME;
            token.name = str_intern_range(token.start, stream);
            stream++;
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