#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>

# define HT_SIZE 20

typedef struct Node {
    char *key;
    int key_len;
    char *val;
    int val_len;
    struct Node *next;
} Node;

typedef struct Hashtable {
    Node **arr;
    pthread_mutex_t lock;
} Hashtable;

/**
 * Initialize hashtable.
 **/
Hashtable* initialize() {
    Hashtable *ht;

    ht = malloc(sizeof(Hashtable));
    if (ht == NULL) {
        perror("Malloc failed\n");
        exit(1);
    }
    ht->arr = (Node **)malloc(sizeof(Node) * HT_SIZE);
    if (ht->arr == NULL) {
        perror("Malloc failed\n");
        exit(1);
    }
    memset(ht->arr, 0 , HT_SIZE * sizeof(Node));
    pthread_mutex_init(&ht->lock, NULL);

    return ht;
}

/**
 * Hash function used for mapping.
 **/
int hashFunction(char *key, int key_len) {
    int hashNum;
    hashNum = 0;

    int i;
    for (i = 0; i < key_len; ++i) {
        hashNum = (hashNum + key[i]) % HT_SIZE;
    }
    return hashNum;
}

/*int compare(char* a, char* b, int a_len, int b_len) {
    if (a_len != b_len) { return 1; }
    int i = 0;
    for (i = 0; i < a_len; ++i) {
        if (a[i] != b[i]) {
            return 1; 
        }
    }
    return 0;
}*/

/**
 * Add new line function
 **/
void addNewLine(int output_fd) {
    char* newLine = malloc(sizeof(char));
	if (newLine == NULL) {
		perror("Malloc failed\n");
		exit(1);
	}
    newLine[0] = '\n';
    write(output_fd, &newLine[0], sizeof(char)); 
    free(newLine);
}

/**
 * Print KNF error code.
 **/
void printKNF(int output_fd) {
    char* err = malloc(sizeof(char) * 4);
    if (err == NULL) {
        perror("Malloc failed\n");
        exit(1);
    }
    err[0] = 'K';
    err[1] = 'N';
    err[2] = 'F';
    err[3] = '\n';
    write(output_fd, &err[0], sizeof(char) * 4); 
    free(err);
}
/**
 * Count number of digits (used to malloc) in the value length.
 **/
int countDigits(int val_len) {
    int count = 0;
    while (val_len != 0) {
        val_len = val_len / 10;
        ++count;
    }
    return count;
}

/**
 * Write value details (incl. length and value) to standard output.
 **/
void printValDetails(Node* n, int output_fd) {
    int output_length = n->val_len + 1;
    int digit_alloc = countDigits(output_length);
    // Allocates the number of digits, 2 newlines, and length of value to print value
    int detail_len = digit_alloc + 2 + n->val_len;
    char* value_details = malloc(sizeof(char) * detail_len);
    if (value_details == NULL) {
        perror("Malloc failed\n");
        exit(1);
    }
    int i, j;
    // Loop i for each digit until digit alloc
    for (i = digit_alloc-1; i >= 0; --i) {
        value_details[i] = (output_length % 10) + '0';
        output_length = output_length / 10;
    }
    //value_details[0] = output_length + '0';
    value_details[digit_alloc] = '\n';

    j = 0;
    for (i = digit_alloc + 1; j < n->val_len; ++i) {
        value_details[i] = n->val[j];
        ++j;
    }
    value_details[i] = '\n';
    write(output_fd, &value_details[0], sizeof(char) * detail_len); 
    free(value_details);
}

/**
 * Handle collisions by appending to hashtable linked list.
 **/
void collisionDetector(Hashtable *ht, Node *new_node) {
    int hashNum = hashFunction(new_node->key, new_node->key_len);
    Node* ptr = ht->arr[hashNum];

    if (ht->arr[hashNum] != NULL) {
        ptr = ht->arr[hashNum];
        while (ptr != NULL) {
            if (strcmp(ptr->key, new_node->key) == 0) {
                break;
            }
            ptr = ptr->next;
        }
        if (ptr == NULL) {
            new_node->next = ht->arr[hashNum];
            ht->arr[hashNum] = new_node;
        } else {
            free(ptr->val);
            ptr->val = new_node->val;   // copy char
            free(new_node->val);
            free(new_node->key);
            free(new_node);
        }
    } else {
        new_node->next = NULL;
        ht->arr[hashNum] = new_node;
    }
}

/**
 * Get value from key.
 **/
Node* get(Hashtable *ht, char* key, int key_len, int output_fd) {
    int hashNum;    
    Node* ptr;
    
    if (ht == NULL) {
        return NULL;
    }

    // Retrieve a hash # based on the properties of the key
    hashNum = hashFunction(key, key_len);
    ptr = ht->arr[hashNum];
    
    while (ptr != NULL) {
        if (strcmp(ptr->key, key) == 0) {   // PTR->KEY
            break;
        }
        ptr = ptr->next;
    }
    if (ptr == NULL) {  // If key was not found
        //printKNF(output_fd);
        return NULL;
    }
	//write(output_fd, "OKG\n", 4);
    //printValDetails(ptr, output_fd);
    return ptr;
}

/**
 * Set or update a new hashtable element.
 **/
int set(Hashtable *ht, char *key, char *val, int key_len, int val_len, int output_fd) {
    Node* new_node;

    if (ht == NULL) {
        return -1;
    }

    // Check if key exists with get. If so, update. Else? New node.
    new_node = get(ht, key, key_len, output_fd);
    if (new_node != NULL) {
        free(new_node->val);        // free existing value
        new_node->val = val;
        new_node->val_len = val_len;
        return 0;
    }

    new_node = malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Malloc failed\n");
        return -1;
    }
    new_node->key = key;        // strdup
    new_node->key_len = key_len;
    new_node->val = val;    //strdup
    new_node->val_len = val_len;

    collisionDetector(ht, new_node);

    return 0;
}

/**
 * Delete key-value pair and return the last set value.
 **/
void del(Hashtable* ht, char* key, int key_len, int output_fd) {
    int hashNum;
    Node** front;
    Node* ptr;
    
    if (ht == NULL) {
        printKNF(output_fd);
        return;
    }

    // Retrieve a hash # based on the properties of the key
    hashNum = hashFunction(key, key_len);
    Node* prev;
    prev = NULL;
    front = &(ht->arr[hashNum]);
    ptr = *front;

    if (ptr != NULL && strcmp(ptr->key, key) == 0) {
        write(output_fd, "OKD\n", 4);
        printValDetails(ptr, output_fd);
        *front = ptr->next;
        free(ptr->key);
        free(ptr->val);
        free(ptr);
        return;
    } 

    while (ptr != NULL) {
        if (strcmp(ptr->key, key) == 0) {   // PTR->KEY
            break;
        }
        prev = ptr;
        ptr = ptr->next;
    }

    if (ptr == NULL) {  // If key was not found
        printKNF(output_fd);
        return;
    }
    // Else return key_len and key
	write(output_fd, "OKD\n", 4);
    printValDetails(ptr, output_fd);
    prev->next = ptr->next;

    // Free node from LL
    free(ptr->key);
    free(ptr->val);
    free(ptr);
}

/**
 * Free the elements of the hashtable.
 **/
void freeHT(Hashtable *ht) {
    int i = 0;
    for (i = 0; i < HT_SIZE; ++i) {
        if (ht->arr[i] != NULL) {
            Node* front = ht->arr[i];
            Node* ptr = front;
            while (front != NULL) {
                ptr = front;
                front = front->next;
                free(ptr->key);
                free(ptr->val);
                free(ptr);
            }
        }
    }
    pthread_mutex_destroy(&ht->lock);
    free(ht->arr);
    free(ht);
}

void printCharArr(char* a, int a_len) {
    int i;
    for (i = 0; i < a_len; ++i) {
        printf("%c", a[i]);
    }
    printf("\n");
}
