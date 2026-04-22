

#define PATH_MAX_LEN  512
#define FILE_MAX_LEN  256

typedef struct {
    char dirEntrada[PATH_MAX_LEN]; /* -e  (BED); default "." */
    char geoArq[FILE_MAX_LEN];     /* -f  nome do arquivo .geo (sem path) */
    char geoPath[PATH_MAX_LEN];    /* BED + "/" + geoArq (caminho completo) */
    char dirSaida[PATH_MAX_LEN];   /* -o  (BSD) */
    char qryArq[FILE_MAX_LEN];     /* -q  arquivo .qry; vazio se ausente */
    char qryPath[PATH_MAX_LEN];    /* BED + "/" + qryArq */
    char pmArq[FILE_MAX_LEN];      /* -pm arquivo .pm; vazio se ausente */
    char pmPath[PATH_MAX_LEN];     /* BED + "/" + pmArq */
} Args;