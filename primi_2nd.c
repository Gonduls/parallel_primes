#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> // just for sleep function, but never used, considering removing it

// impostazioni di funzionamento
#define nthread 4 // che controllano se un numero è primo o meno
#define MAX 10000000 // numeri primi desiderati
#define starting 1000 // inizializzazione della lista con starting numeri primi, 
                        // non in parallelo. Modificabile da terminale dalla chiamata

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
    short number;       // numero del thread, usato solo per printf
    short updating;     // 1 solo se thread deve aumentare "limite"
    int start;          // da dove si inizia a controllare
    int end;            // fino a dove si deve controllare
    int found;          // numeri primi effettivamente trovati
    int to_find;        // numeri che voglio trovati, usato solo per concludere
    int expect;         // numeri che mi aspetto di trovare
    punt first;         // indirizzo del primo 'numero' da riempire
    punt attachment;    // indirizzo dell'ultimo nodo a cui mi devo attaccare
    pthread_t th_handle;
} check_values;


//************ variabili globali **************************

int limite;             // ultimo numero primo calcolato (+/-), stabilisce limite
punt HEAD;              // testa della lista dinamica contenente i numeri cercati
sem_t stop;             // >1 se uno o più thread si aspettano nuovi valori
sem_t go[nthread];      // dà il via al thread appena inizializzato
short ready[nthread];   // comunica quali thread si aspettano i nuovi valori
short exit_val;         // 1 se i thread devono uscire

//************ funzioni ***********************************
// restituisce 1 se valore è primo, 0 altrimenti
short primo(int valore);
void stampa(punt testa);
float min(float a, float b);
// calcola i primi start numeri primi
punt init(int start);

// controllano se i numeri sono primi e costruiscono la lista dinamica
void* checker(void* arg);

int main(int argc, char* argv[]){
    int i, y;           // per i cicli

    punt ultimo;        // raccoglie l'ultimo "numero" dalla funzione init
    int calcolati;      // usato per sapere quanti numeri sono già stati calcolati
    int expected;       // usato per tenere traccia dei numeri che mi aspetto di ottenere
    int da;             // tiene conto dello starting point di ogni thread
    int chunk_size;     // usato per sapere quanti numeri sono stati assegnati al th e condizione di uscita dal while
    float density;      // usato per sapere avere un limite massimo di numeri primi che mi devo aspettare
                        // in un certo intervallo (immaginando che la densità decresca sempre)
    check_values thread[nthread]; //array che conterrà i valori necessari ai thread

    // starting può essere modificato da terminale (prossima inizializzazione e if)
    int start = starting;// quantità di numeri minimi che conviene trovare non in parallelo
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
    
    // funzione init e ulteriori inizializzazioni
    if(nthread == 0 || nthread == 1){
        init(MAX);
        stampa(HEAD);
        return 0;
    }
    ultimo = init(start);
    limite = ultimo->value;
    ultimo->next = malloc(sizeof(numero));
    ultimo = ultimo->next;

    da = limite + 2;
    calcolati = start;
    expected = start;
    density = (float)start / (float)limite;
    exit_val = 0;

    //************ calcolo la maggior parte dei numeri ***********
    // creazione dei thread
    for(i=0; i<nthread;i++){
        thread[i].number = i;
        thread[i].found = 0;
        pthread_create(&thread[i].th_handle, NULL, checker, &thread[i]);
    }

    // loop principale
    y = 0;
    while(1){
        sem_wait(&stop);
        // individuo il thread che ha fatto post su stop
        i = 0;
        while(ready[i] == 0)
            i++;
        ready[i] = 0;
        // se il thread non aveva ancora lavorato salto la raccolta dei dati che ha elaborato
        if(thread[i].found != 0){
            expected = expected - thread[i].expect + thread[i].found;
            calcolati += thread[i].found;
            float new_den = (float)thread[i].found / (float) (thread[i].end - thread[i].start);
            density = min(density, new_den);
        }

        //*********** inizio settaggio valori utili al thread *************
        thread[i].updating = 0;
        if((y+1) % nthread == 0)        // chi controlla i valori più grandi aggiorna limite
            thread[i].updating = 1;     // può creare problemi se nthread > numero di core
        
        if(y % nthread == 0){
            printf("Ciclo #%d\n", y);
            printf("limite per densità: %.3f\n", ((float)(MAX - expected))/(density * nthread));
            printf("limite per numero più grande calcolato: %.3f, numero più grande calcolato: %d\n", ((float)(limite*(limite-1))/nthread), limite);
            chunk_size = (int) min(((float)(MAX - expected))/(density * nthread), ((float)(limite*(limite-1))/nthread));
            if (chunk_size<0)
                chunk_size = (int) ((float)(MAX - expected))/(density * nthread);
            chunk_size -= chunk_size % 2;
            printf("chunk_size: %d\n\n", chunk_size);
        }
        thread[i].start = da;
        thread[i].end = da + chunk_size -2;
        thread[i].found = 0;
        thread[i].to_find = 0;
        thread[i].expect = (int) ((float)chunk_size * density);
        thread[i].first = ultimo;
        thread[i].attachment = malloc(sizeof(numero));
        sem_post(go + i);

        ultimo = thread[i].attachment;
        expected += thread[i].expect;
        da += chunk_size;
        y++;
        if((chunk_size < 1000 && (y)%nthread == 0) || expected > MAX) //al posto di 1000 va start
            break;
    }
    
    // raccolta thread non necessari
    for (y=0; y<nthread - 1; y++){

        sem_wait(&stop);
        // individuo il thread che ha fatto post su stop
        i = 0;
        while(ready[i] == 0)
            i++;
        ready[i] = 0;
        calcolati += thread[i].found;
        thread[i].found = -1;
        sem_post(go + i);
    }
    //printf("hi\n");
    //************ calcolo degli ultimi valori necessari ***********
    exit_val = 0;
    // individuo l'ultimo thread rimasto
    i = 0;
    sem_wait(&stop);
    while(ready[i] == 0){
        i++;
    }
    calcolati += thread[i].found;
    thread[i].start = da;
    thread[i].found = 0;
    thread[i].to_find = MAX - calcolati;
    thread[i].first = ultimo;
    thread[i].attachment = NULL;

    sem_post(go + i);
    sem_wait(&stop);
    exit_val = 1;
    pthread_join (thread[i].th_handle, NULL);

    //stampa(HEAD);
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

punt init(int start){
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
    return(pointer); // ultimo valore calcolato
}

short primo(int valore){
    punt curr = HEAD->next;
    while(curr != NULL && valore % curr->value != 0){
        if(valore < curr->value * curr->value)
            return 1;
        curr = curr->next;
    }
    if(curr == NULL){
        printf("Errore, valore troppo grande\n");
        return(-1);
    }
    return 0;
}

float min(float a, float b){
    if (a>b)
        return b;
    return a;
}

void* checker(void* arg){
    check_values* valori = ((check_values*) arg);
    int curr;
    short ans;
    punt corrente = NULL;
    
    while(1){
        ready[valori->number] = 1;
        sem_post(&stop);
        sem_wait(&(go[valori->number]));


        if(valori->found == -1){
            printf("exiting\n");
            return NULL;
        }
        corrente = valori->first;

        curr = valori->start;
        
        while(1){
            if(valori->to_find == 0){
                if(curr > valori->end)
                    break;
            }
            else if(valori->found == valori->to_find){
                sem_post(&stop);
                printf("Last value found: %d\n", curr - 2);
                printf("exiting_2\n");
                return NULL;
            }

            ans = primo(curr);
            
            // have to come up with better way to handle this
            while(ans == -1){
                printf("waiting\n");
                sleep(1);
                ans = primo(curr);
            }

            if(ans == 1){
                // aggiorno limite di volta in volta nel non finisca per primo questo th
                if(valori->updating)
                    limite = curr;
                
                // Aspetto un ciclo per far scorrere current,
                // per poter attaccare attachment, senza problemi, dopo
                if(valori->found){
                    corrente->next = malloc(sizeof(numero));
                    corrente->next->value = curr;
                    corrente->next->next = NULL;
                    corrente = corrente->next;
                }
                else{ // se ho trovato 0 numeri primi
                    corrente->value = curr;
                    corrente->next = NULL;
                }

                valori->found += 1;
            }

            curr += 2;
        }
        corrente->next = valori->attachment;
    }
    printf("exiting_3\n");
    return NULL;
}