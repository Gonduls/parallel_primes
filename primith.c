#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#define nthread 1 //# che controllano se un numero è primo o meno, thread totali = nthread + 2
#define MAX 100000 //# numeri primi desiderati
#define WAY "w"  //w per sovrascrivere il file, a per inserire in coda
#define colonne 5 //colonne di numeri stampate nel file (0 per averli stampati in una riga continua, 1 per averli tutti in colonna)
#define OUT "output.txt" //file filnale + relativa directory.
/*Inserire "//" prima della riga precedente per stampare i numeri a schermo
 *Esempio directory /mnt/c/desktop/output.txt
 *Si può non inserire una directory, ma semplicemente il nome del file output, 
 *verrà creato nella directory corrente. Se il file non esisteva in precedenza, viene creato */

//lista sequenziale che contiene i numeri primi
struct {
    int array[MAX];
    int ind;
} primi;

//creo un array di struct che contengono i semafori e le variabili di passaggio, ciò che lega i thread assieme
/* spiegazione valori della struct e array di struct:
 * l'array è utilizzato per passare da newval agli anelli (checkval) e infine a storeval i numeri, a storeval arrivano solo numeri primi.
 * ogni anello riceve valori da arrpass[suo indice +1] e scrive i risultati in arrpass[suo indice]
 * campo val: il numero vero e proprio da passare
 * campo index: se l'anello inferiore mi chiede un valore perchè lui non sta controllando nulla, con index gli dico fino a quale numero primo ho controllato
 * campo prime: usato quasi come bool, se ricevo un valore con prime=1 il numero è già primo e devo passarlo all'anello inferiore, senza fare calcoli
 *  prime = 0 => numero potrebbe essere primo, prime = -1 => sicuramente il numero non è primo, non lo passo all'anello inferiore
 * campo waiting: usato come bool, setto a 1 se mi serve un valore da controllare, verrà settato a 0 dall'anello superiore
 * campo need: faccio una wait su need dopo aver settato waiting a 1, aspetto che l'anello superiore abbia un valore per me e resetti i miei campi
 * campo give: faccio una wait su give se ho un valore da passare, non è detto che l'anello inferiore sia pronto a ricevere*/

typedef struct {
    int val;
    int index;
    int prime;
    int waiting;
    sem_t need;
    sem_t give;
}passval;
passval arrpass[nthread+1];

//semaforo usato nella fase di creazione dei thread, per passargli il loro indice
sem_t thnum;
//intero utilizzato come bool, 1 solo quando voglio far terminare i thread
int exitstatus; 

void * checkval(void * arg);
void * newval(void * arg);
void * storeval(void * arg);
/*funzione per far terminare l'esecuzione dei thread, evitando deadlock:
 * ogni thread ha un controllo di existatus dopo una wait, exiting fa le post*/
int exiting(int exitvalue);

//funzione per stampare in colonne ben definite alla fine tutti i numeri primi
void printval();

int main(){
    int i;
    //creo un array per contenere tutti i thread necessari
    pthread_t threads[nthread+2];

    sem_init(& thnum, 0, 0);
    exitstatus = 0;
    //inizializzo tutti i thread catena
    for(i=0; i<nthread; i++){
        arrpass[i].val=0;
        arrpass[i].index=0;
        arrpass[i].prime=0;
        arrpass[i].waiting=0;
        sem_init(&arrpass[i].need, 0, 0);
        sem_init(&arrpass[i].give, 0, 0);
        pthread_create (&threads[i], NULL, checkval, (void *) i);
        sem_wait(&thnum);
    }

    //fase di inizializzazione della lista sequenziale: preferisco impostare i primi 4 valori per evitare errori
    primi.array[0]= 2;
    primi.array[1]= 3;
    primi.array[2]= 5;
    primi.array[3]= 7;
    primi.ind = 3;
    pthread_create(&threads[nthread], NULL, newval, NULL);
    pthread_create(&threads[nthread+1], NULL, storeval, NULL);
    for(i=0; i<nthread +2; i++){
        pthread_join (threads[i], NULL);
        printf("Chiuso thread %d\n", i);
    }
    printval();
    return 0;
}

//fornisce un nuvo valore al thread #nthread-1 (il più grande, l'ultimo anello). Il nuovo valore è sempre dispari.
void * newval(void * arg){
    int current;
    printf("Sono il thread newval\n");
    current = primi.array[primi.ind];
    //exitstatus = 0 => continuo
    while (primi.ind<MAX-1 && !exitstatus){
        current = current + 2;
        sem_wait(&arrpass[nthread].need);
        arrpass[nthread].val = current;
        arrpass[nthread].prime = 0;
        //dato che il numero è dispari mi risparmio i controlli val%2==0, comincia a controllare da 3
        arrpass[nthread].index = 1;
        sem_post(&arrpass[nthread].give);
    }
    exiting(1);
    return NULL;
}


void * checkval(void * arg){
    int numero;
    numero = (int) arg;
    sem_post(&thnum);
    printf("Sono il thread #%d\n", numero);
    
    while (primi.ind<MAX-1){
        sem_post(& arrpass[numero + 1].need);   //now I can accept a new value
        arrpass[numero + 1].waiting=1;
        sem_wait(& arrpass[numero + 1].give);   //waiting for your value
        if (exitstatus == 1)
            return NULL;
        // controllo un numero solo fin quando il thread minore di me non sta aspettando un valore da me
        while (arrpass[numero].waiting == 0){
            if (arrpass[numero+1].val < (primi.array[arrpass[numero+1].index] * primi.array[arrpass[numero+1].index])){
                arrpass[numero+1].prime = 1;
                break;
            }
            if (arrpass[numero+1].val % primi.array[arrpass[numero+1].index] == 0){
                arrpass[numero+1].prime = -1;
                break;
            }
            arrpass[numero+1].index ++;
        }
        //sono sicuro che tale numero non sia primo
        if(arrpass[numero+1].prime == -1)
            continue;
        // uso nuovamente due int come bool: se ho un numero primo o se mi hanno richiesto un numero dall'anello precendente
        if(arrpass[numero+1].prime == 1 || arrpass[numero].waiting == 1){
            sem_wait(& arrpass[numero].need);
            if (exitstatus == 1)
                return NULL;
            //resetto campo waiting
            arrpass[numero].waiting = 0;
            // passaggio di valori tra due anelli della catena
            arrpass[numero].val = arrpass[numero + 1].val;
            arrpass[numero].index = arrpass[numero + 1].index;
            arrpass[numero].prime = arrpass[numero + 1].prime;
            sem_post(&arrpass[numero].give);
        }
    }
    exiting(1);
    return(NULL);
}

//qui non uso mai waiting: mi serve un valore solo se è primo
void * storeval(void * arg){
    printf("Sono il thread storeval\n");
    while(primi.ind < MAX-1){
        sem_post(&arrpass[0].need);
        sem_wait(&arrpass[0].give);
        if (exitstatus == 1)
            return NULL;
        primi.ind ++;
        primi.array[primi.ind]=arrpass[0].val;
    }
    exiting(1);
    return NULL;
}


int exiting(int exitvalue){
    int i;
    if (exitvalue){
        exitstatus = 1;
        for (i=0; i<(nthread + 1); i++){
            sem_post(&arrpass[i].need);
            sem_post(&arrpass[i].give);
        }
    }
    return(exitstatus);
}

//normale funzione indipendente dal numero di thread, chiamata quando 
void printval(){
    int i;
    FILE* fileoutput;
    fileoutput = stdout;
    #ifdef OUT
    fileoutput=fopen(OUT, WAY);
    #endif
    if(fileoutput == NULL){
        printf ("Errore nell'apertura del file, chiusura programma\n");
        return;
    }
    for (i=1; i<MAX+1; i++){
        fprintf (fileoutput, "%d", primi.array[i-1]);
        if(colonne == 0){
            fprintf (fileoutput, "\t");
        }
        else{
            if (i % colonne == 0)
                fprintf (fileoutput, "\n");
            else
                fprintf (fileoutput, "\t");
        }
    }
    #ifdef OUT
    i=fclose(fileoutput);
    if (i == EOF)
        printf ("Errore nella chiusura del file, chiusura programma\n");
    #endif
}