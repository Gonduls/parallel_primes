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
#define OUT "output2.txt" //file filnale + relativa directory.


//************ nuovi tipi definiti ************************
typedef struct el{
    int value;
    struct el *next;
} numero;
typedef numero* punt;

typedef struct elem{
    int start;          // da dove si inizia a controllare
    int end;            // fino a dove si deve controllare
    int found;          // numeri primi effettivamente trovati
    int to_find;        // numeri che voglio trovati, usato solo per concludere
    punt first;         // indirizzo del primo 'numero' da riempire
    punt attachment;    // indirizzo dell'ultimo nodo a cui mi devo attaccare
} check_values;


//************ variabili globali **************************

punt HEAD;              // testa della lista dinamica contenente i numeri cercati
sem_t stop;             // >1 se uno o più thread si aspettano nuovi valori
sem_t go[nthread];      // dà il via al thread appena inizializzato
short ready[nthread];   // comunica quali thread si aspettano i nuovi valori
short exit_val;             // 1 se i thread devono uscire

//************ funzioni ***********************************
// restituisce 1 se valore è primo, 0 altrimenti
short primo(int valore);
void stampa(punt testa);

// calcola i primi start numeri primi
int init(int start);

// funzioni eseguite da thread
/*// organizza i thread che controllano i numeri
void* handler(void* arg);*/
// controllano se i numeri sono primi e costruiscono la lista dinamica
void* checkers(void* arg);

int main(int argc, char* argv[]){
    
    int ultimo;         // raccoglie l'ultimo valore dalla funzione init
    int calcolati;      // usato per sapere quanti numeri sono già stati calcolati
    float density;      // usato per sapere avere un limite massimo di numeri primi che mi devo aspettare
                        // in un certo intervallo (immaginando che la densità decresca sempre)
    check_values thread[nthread]; //array che conterrà i valori necessari ai thread

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
    ultimo = init(start);
    stampa(HEAD);
    calcolati = start;
    density = (float)start / (float)ultimo;
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

int init(int start){
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
    return(curr -2); // ultimo valore calcolato
}

short primo(int valore){
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

