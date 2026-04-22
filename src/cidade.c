
#include <stdint.h>
#include "hash.h"
#include "svg.h"

#define CEP_MAX_LEN 32

typedef struct {
    char   cep[CEP_MAX_LEN]; /* identificador textual da quadra         */
    double x, y;             /* âncora: canto sudeste                   */
    double w, h;             /* largura e altura                        */
    double sw;               /* espessura da borda                      */
    char   corFill[32];      /* cor de preenchimento                    */
    char   corStroke[32];    /* cor da borda                            */
} Quadra;