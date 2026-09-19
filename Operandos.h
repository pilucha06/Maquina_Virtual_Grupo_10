#define MAXVEC 4


typedef int (*getters)(TMV *, int);
typedef void (*setters)(TMV *, int, int);

int get(TMV *mv, int op);
void set(TMV *mv, int op, int valor);