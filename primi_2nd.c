#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

// impostazioni di funzionamento
#define nthread 4 // che controllano se un numero è primo o meno
#define MAX 100000 // numeri primi desiderati
#define starting 100 // inizializzazione della lista con starting numeri primi, 
                     // non in parallelo e modificabile da terminale dalla chiamata

// impostazioni di output
#define WAY "w"  //w per sovrascrivere il file, a per inserire in coda
#define colonne 5 //colonne di numeri stampate nel file (0 per averli stampati in una riga continua, 1 per averli tutti in colonna)
//#define OUT "output.txt" //file filnale + relativa directory.

typedef struct el{
    int value;
    struct el *next;
} numero;
typedef numero* punt;

// variabile globale che punterà alla lista dinamica di numeri primi
punt HEAD;

void stampa(punt testa);
int primo(int valore); // restituisce 1 se valore è primo, 0 altrimenti

void init(int start);

int main(int argc, char* argv[]){


    // si può modificare starting da terminale (prossima inizializzazione e if)
    int start = starting;
    if (argc!= 1){
        for (int i = 1; i< argc; i++)
            if(argv[i][0]>'0' && argv[i][0]<='9'){
                int j = 0;
                start = 0;
                while(argv[i][j] != '\0'){
                    start = start * 10 + argv[i][j] - '0';
                    j ++;
                }
            }
                
                
    }
    
    HEAD = malloc(sizeof(numero));
    HEAD->value = 2;
    HEAD->next = malloc(sizeof(numero));
    HEAD->next->value = 3;
    HEAD->next->next = NULL;
    init(start);
    stampa(HEAD);
    
    return 0;
}

void stampa(punt testa){
    FILE* fileoutput;
    int i = 1;
    fileoutput = stdout;
    #ifdef OUT
    fileoutput=fopen(OUT, WAY);
    #endif
    if(fileoutput == NULL){
        printf ("Errore nell'apertura del file, chiusura programma\n");
        return;
    }
    while(testa != NULL){
        fprintf (fileoutput, "%d", testa->value);
        if(colonne == 0){
            fprintf (fileoutput, "\t");
        }
        else{
            if (i % colonne == 0)
                fprintf (fileoutput, "\n");
            else
                fprintf (fileoutput, "\t");
        }
        testa = testa->next;
        i ++;
    }

    #ifdef OUT
    i=fclose(fileoutput);
    if (i == EOF)
        printf ("Errore nella chiusura del file, chiusura programma\n");
    #endif
}

void init(int start){
    int i = 2, curr = 5;
    punt pointer;
    pointer = HEAD->next;

    while(i < start){
        while(primo(curr) == 0)
            curr += 2;
        pointer->next = malloc(sizeof(numero));
        pointer = pointer->next;
        pointer->value = curr;
        pointer->next = NULL;
        curr += 2;
        i++;
    }
    
}

int primo(int valore){
    punt curr = HEAD->next;
    while(curr != NULL && valore % curr->value != 0){
        if(valore < curr->value * curr->value)
            return 1;
        curr = curr->next;
    }
    if(curr == NULL)
        printf("Errore, valore troppo grande\n");
    
    return 0;
}