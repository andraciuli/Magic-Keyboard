#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALPHABET_SIZE 26
#define ALPHABET "abcdefghijklmnopqrstuvwxyz"

/* useful macro for handling error codes */
#define DIE(assertion, call_description)                                       \
	do {                                                                       \
		if (assertion) {                                                       \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                 \
			perror(call_description);                                          \
			exit(errno);                                                       \
		}                                                                      \
	} while (0)

typedef struct trie_node_t trie_node_t;
struct trie_node_t {
	/* nr de aparitii ale cuvantului*/
	int count;

	/* 1 if current node marks the end of a word, 0 otherwise */
	int end_of_word;

	trie_node_t **children;
	int n_children;
};

typedef struct trie_t trie_t;
struct trie_t {
	trie_node_t *root;

	/* Number of keys */
	int size;

	/* Generic Data Structure */
	int data_size;

	/* Trie-Specific, alphabet properties */
	int alphabet_size;
	char *alphabet;


	/* Optional - number of nodes, useful to test correctness */
	int nNodes;
};

trie_node_t *trie_create_node(trie_t *trie)
{
	trie_node_t *node = malloc(sizeof(trie_node_t));
	DIE(node == NULL, "malloc node");
	node->count = 0;
	node->end_of_word = 0;
	node->children = calloc(trie->alphabet_size, sizeof(trie_node_t *));
	DIE(node->children == NULL, "calloc children");
	node->n_children = 0;

	return node;
}

trie_t *trie_create(int data_size, int alphabet_size, char *alphabet,
					void (*free_value_cb)(void *))
{
	trie_t *trie = malloc(sizeof(trie_t));
	DIE(trie == NULL, "malloc trie");

	trie->size = 0;
	trie->data_size = data_size;
	trie->alphabet_size = alphabet_size;
	trie->alphabet = alphabet;
	trie->nNodes = 1;

	trie->root = trie_create_node(trie);


	return trie;
}

void trie_insert(trie_t *trie, char *key)
{
	if (key[0] == '\0') {
		trie->root->count++;
		trie->root->end_of_word = 1;
		return;
	}

	trie_node_t *current = trie->root;
	int i = 0;

	while (key[i] != '\0') {
		int letter = key[i] - 'a';

		if (current->children[letter] == NULL) {
			current->children[letter] = trie_create_node(trie);
			++trie->nNodes;
			++current->n_children;
		}

		current = current->children[letter];
		++i;
	}
	current->count++;
	current->end_of_word = 1;
	trie->size++;
}


void load(trie_t* trie, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        return;
    }

    char word[256];
    while (fscanf(file, "%s", word) != EOF) {
        trie_insert(trie, word);
    }

    fclose(file);

    printf("File %s successfully loaded.\n", filename);
}

void autocorrect_recursive(trie_t *trie, trie_node_t *node, char *word, int current_idx, int k, int changes, char *new_word)
{
	if (node == NULL) {
        return;
    }

    if (k < 0) {
        return;
    }

	if(changes > k) {
		return;
	}

    if (strlen(word) == 0) {
        if (node->end_of_word && changes <= k) {
            printf("%s\n", new_word);
        }
        return;
    }
    
    int letter = word[0] - 'a';
        
    if (changes <= k) {
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            trie_node_t *child = node->children[i];
            //char *current_word = malloc(strlen(word) + 2);
    		//strcpy(current_word, word);
            if (child != NULL) {
            if (i != letter) {
                if (changes < k) {
                    new_word[current_idx] = trie->alphabet[i];
                    autocorrect_recursive(trie, child, word + 1, current_idx + 1, k, changes + 1, new_word);
                    new_word[current_idx] = '\0';
                }
            } else {
				new_word[current_idx] = trie->alphabet[i];
                autocorrect_recursive(trie, child, word + 1, current_idx + 1, k, changes, new_word);
				new_word[current_idx] = '\0';
            }
    
    
        }
    }
}
}

void trie_autocorrect(trie_t *trie, char *word, int k)
{
    int current_idx = 0;
    int changes = 0;
	char *new_word = calloc(50, sizeof(char));

    
    autocorrect_recursive(trie, trie->root, word, current_idx, k, changes, new_word);
	free(new_word);
}

void trie_free_subtrie(trie_t *trie, trie_node_t *node)
{
	if (!node)
		return;

	for (int i = 0; i < trie->alphabet_size && node->n_children; ++i) {
		if (node->children[i] == NULL)
			continue;

		trie_free_subtrie(trie, node->children[i]);
		--node->n_children;
		node->children[i] = NULL;
	}

	free(node->children);
	free(node);

	--trie->nNodes;
}

void trie_remove(trie_t *trie, char *key)
{
	if (key[0] == '\0') {
		trie->root->end_of_word = 0;
		--trie->size;
		return;
	}

	trie_node_t *current = trie->root;
	trie_node_t *parent = trie->root;
	int parent_letter = (key[0] - 'a');
	int i = 0;

	while (key[i] != '\0') {
		int letter = key[i] - 'a';

		if (current->children[letter] == NULL) {
			return;
		}

		if (current->n_children > 1 || current->end_of_word) {
			parent = current;
			parent_letter = letter;
		}

		current = current->children[letter];

		++i;
	}
	current->count--;
	current->end_of_word = 0;
	if (!current->n_children) {
		trie_free_subtrie(trie, parent->children[parent_letter]);
		parent->children[parent_letter] = NULL;
		--parent->n_children;
	}

	--trie->size;
}

void trie_free(trie_t **pTrie)
{
	trie_free_subtrie(*pTrie, (*pTrie)->root);
	free(*pTrie);
}

int main()
{
	int n, k;
	char alphabet[] = ALPHABET;
	char key[256], file[256];


	trie_t *trie = trie_create(sizeof(int), ALPHABET_SIZE, alphabet, free);

	while(1) {
        char command[16];
        scanf("%s ", command);
        if(strncmp(command, "INSERT", 7) == 0) {
            scanf("%s", key);
			trie_insert(trie, key);
        }
        if(strncmp(command, "LOAD", 5) == 0) {
            scanf("%s\n", file);
			load(trie, file);
        }
        if(strncmp(command, "REMOVE", 7) == 0) {
            scanf("%s\n", key);
			trie_remove(trie, key);
        }
        if(strncmp(command, "AUTOCORRECT", 12) == 0) {
            scanf("%s %d", key, &k);
			trie_autocorrect(trie, key, k);
        }
        if(strncmp(command, "AUTOCOMPLETE", 13) == 0) {
            
        }
        if(strncmp(command, "EXIT", 5) == 0) {
            trie_free(&trie);
            break;
        }
    }


	return 0;
}