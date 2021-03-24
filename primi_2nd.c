#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

// impostazioni di funzionamento
#define nthread 8 // che controllano se un numero è primo o meno
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
    short number;
    int start;          // da dove si inizia a controllare
    int end;            // fino a dove si deve controllare
    int found;          // numeri primi effettivamente trovati
    int to_find;        // numeri che voglio trovati, usato solo per concludere
    punt first;         // indirizzo del primo 'numero' da riempire
    punt attachment;    // indirizzo dell'ultimo nodo a cui mi devo attaccare
    pthread_t th_handle;
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
float min(float a, float b);
// calcola i primi start numeri primi
int init(int start);

// funzioni eseguite da thread
/*// organizza i thread che controllano i numeri
void* handler(void* arg);*/
// controllano se i numeri sono primi e costruiscono la lista dinamica
void* checker(void* arg);

int main(int argc, char* argv[]){
    int i;              // per i cicli

    int ultimo;         // raccoglie l'ultimo valore dalla funzione init
    int calcolati;      // usato per sapere quanti numeri sono già stati calcolati
    float density;      // usato per sapere avere un limite massimo di numeri primi che mi devo aspettare
                        // in un certo intervallo (immaginando che la densità decresca sempre)
    check_values thread[nthread]; //array che conterrà i valori necessari ai thread

    // starting può essere modificato da terminale (prossima inizializzazione e if)
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
    
    // inizializzazione lista
    HEAD = malloc(sizeof(numero));
    HEAD->value = 2;
    HEAD->next = malloc(sizeof(numero));
    HEAD->next->value = 3;
    HEAD->next->next = NULL;
    ultimo = init(start);
    stampa(HEAD);
    calcolati = start;
    density = (float)start / (float)ultimo;
    exit_val = 0;
    //************ calcolo la maggior parte dei numeri ***********
    // creazione dei thread
    for(i=0; i<nthread;i++){
        thread[i].number = i;
        thread[i].to_find = 0;
        thread[i].found = 1;
        thread[i].end = 1;
        pthread_create(&thread[i].th_handle, NULL, checker, &thread[i]);
    }

    // loop principale
    do{
        sem_wait(&stop);
        i = 0;
        while(ready[i] == 0)
            i++;
        calcolati += thread[i].found;
        float new_den = (float)thread[i].found / (float) (thread[i].end - thread[i].start);
        density = min(density, new_den);
    }
    while(0);
    
    // raccolta
    for (i=0; i<nthread; i++)
        pthread_join(thread[i].th_handle, NULL);
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

float min(float a, float b){
    if (a<b)
        return b;
    return a;
}

void* checker(void* arg){
    check_values valori = *((check_values*) arg);
    int curr;
    punt corrente = valori.first;

    printf("thread %d pronto\n", valori.number);
    
    while(exit_val == 0){
        ready[valori.number] = 1;
        sem_post(&stop);
        sem_wait(go+valori.number);
        
        // mi serve protezione rispetto
        if(valori.to_find == 0)
            valori.to_find = valori.end;    // ho bisogno di un valore assurdamente grande
        else
            valori.end = valori.start + MAX; // ho bisogno di un valore assurdamente grande
        
        curr = valori.start;
        
        while(1){
            if(valori.to_find == 0){
                if(curr > valori.end)
                    break;
            }
            else if(valori.found == valori.to_find){
                free(corrente->next);
                corrente->next = NULL;
                return NULL;
            }
            if(primo(curr) == 1){
                valori.found += 1;
                corrente->value = curr;
                corrente->next = malloc(sizeof(numero));
            }

            curr += 2;
        }
        
        free(corrente->next);
        corrente->next = valori.attachment;
    }
    return NULL;
}