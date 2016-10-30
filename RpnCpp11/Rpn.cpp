#include "Rpn.h"
#include <cctype>



// Устанавливает s на первый значащий символ строки *s.
void getNextWord( const char** ps )
{
    while(**ps && !(std::isalnum(**ps) || std::ispunct(**ps)))
        ++*ps;
}



// Возвращает код соответствующий строке s.
unsigned hashFunction( const char* s )
{
    unsigned res = 1;
    while (*s) {
        res *= *s++ + 31; // код символа + простое число
    }
    return res;
}