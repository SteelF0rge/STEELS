#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>  // Include this header for INT_MAX

#define MAX_FRAMES 4
#define REFERENCE_STRING_LENGTH 14

int page_stream[REFERENCE_STRING_LENGTH] = {7,0,1,2,0,3,0,4,2,3,0,3,2,3};
int page_frames[MAX_FRAMES];
int faults = 0;
int used[MAX_FRAMES] = {0};
int count=0; //used in LRU
pthread_mutex_t lock;
int count2=0; //used in OPTIMAL
int next_index = 0;  //used in FIFO
void *LRU(void *arg) {
    int page_number = *(int *)arg;
    free(arg);

    pthread_mutex_lock(&lock);

    int pages = REFERENCE_STRING_LENGTH;

    int pageFound = 0;
    for (int j = 0; j < MAX_FRAMES; j++) {
        if (page_frames[j] == page_number) {
            pageFound = 1;
            used[j] = count+1;
            count++;
            break;
        }
    }

    if (!pageFound) {
        int minUsed = used[0];
        int minIndex = 0;

        for (int j = 1; j < MAX_FRAMES; j++) {
            if (used[j] < minUsed) {
                minUsed = used[j];
                minIndex = j;
            }
        }

        page_frames[minIndex] = page_number;
        used[minIndex] = count+1;
        count++;
        faults++;
    }
    printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\n", page_number, page_frames[0], page_frames[1], page_frames[2],page_frames[3]);

    pthread_mutex_unlock(&lock);

    return NULL;
}

void *FIFO(void *arg) {
    int page_number = *(int *)arg;
    free(arg);

    pthread_mutex_lock(&lock);

    int pages = REFERENCE_STRING_LENGTH;
    
    int page_found = 0;

    for (int j = 0; j < MAX_FRAMES; j++) {
        if (page_frames[j] == page_number) {
            page_found = 1;
            break;
        }
    }

    if (!page_found) {
        page_frames[next_index] = page_number;
        next_index = (next_index + 1) % MAX_FRAMES;
        faults++;
    }

    printf("%d\t\t", page_number);
    for (int j = 0; j < MAX_FRAMES; j++) {
        if (page_frames[j] != -1)
            printf(" %d\t\t", page_frames[j]);
        else
            printf(" -\t\t");
    }
    printf("\n");

    pthread_mutex_unlock(&lock);

    return NULL;
}

void *Optimal(void *arg) {
    int page = *(int *)arg;
    free(arg);

    pthread_mutex_lock(&lock);

    int pages = REFERENCE_STRING_LENGTH;
    int j, k;

    int found = 0;

    for (j = 0; j < MAX_FRAMES; j++) {
        if (page_frames[j] == page) {
            found = 1;
            break;
        }
    }

    if (!found) {
        int emptyFrame = -1;
        for (j = 0; j < MAX_FRAMES; j++) {
            if (page_frames[j] == -1) {
                emptyFrame = j;
                break;
            }
        }

        if (emptyFrame != -1) {
            page_frames[emptyFrame] = page;
        } else {
            int farthest = -1;
            int replaceIndex = -1;

            for (j = 0; j < MAX_FRAMES; j++) {
                int futureIndex = INT_MAX;

                for (k = count2 + 1; k < pages; k++) {
                    if (page_stream[k] == page_frames[j]) {
                        futureIndex = k;
                        break;
                    }
                }

                if (futureIndex == INT_MAX) {
                    replaceIndex = j;
                    break;
                }

                if (futureIndex > farthest) {
                    farthest = futureIndex;
                    replaceIndex = j;
                }
            }

            page_frames[replaceIndex] = page;
        }
        faults++;
    }

    printf("%d\t\t", page);
    for (j = 0; j < MAX_FRAMES; j++) {
        if (page_frames[j] == -1)
            printf("-\t\t");
        else
            printf("%d\t\t", page_frames[j]);
    }
    printf("\n");
    count2++;
    pthread_mutex_unlock(&lock);

    return NULL;
}

int main() {
    pthread_t threads[REFERENCE_STRING_LENGTH];

    for (int i = 0; i < MAX_FRAMES; i++) {
        page_frames[i] = -1;
    }

    pthread_mutex_init(&lock, NULL);
    
printf("Incoming");  
for(int i=0;i<MAX_FRAMES;i++)
{
	printf("\t\tBLOCK %d",i+1);
}
printf("\n");
    for (int i = 0; i < REFERENCE_STRING_LENGTH; i++) {
        int *page_number = (int *)malloc(sizeof(int)); // Type cast malloc to (int *)
        *page_number = page_stream[i];
        pthread_create(&threads[i], NULL,LRU, page_number);  // your choice to call whichever function
    }

    for (int i = 0; i < REFERENCE_STRING_LENGTH; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    printf("Total page faults : %d\n", faults);

    return 0;
}