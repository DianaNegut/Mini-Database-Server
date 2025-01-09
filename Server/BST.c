#include "BST.h"
#include <ctype.h>
#include <string.h>

BSTNode* createNode(char *word, int colIndex, int* rowIndex) {
    BSTNode *newNode = (BSTNode *)malloc(sizeof(BSTNode));
    if (!newNode) {
        printf("Eroare la alocarea memoriei.\n");
        return NULL;
    }
    newNode->word = strdup(word);
    newNode->column = colIndex;
    newNode->row = *rowIndex;
    newNode->left = newNode->right = NULL;
    (*rowIndex)++;
    return newNode;
}

BSTNode* insert(BSTNode *root, char *word, int colIndex, int* rowIndex) {
    if (root == NULL) {
        return createNode(word, colIndex, rowIndex);
    }
    
    if (strcmp(word, root->word) < 0) {
        root->left = insert(root->left, word, colIndex, rowIndex);
    } else if (strcmp(word, root->word) >= 0) {
        root->right = insert(root->right, word, colIndex, rowIndex);
    }
    
    return root;
}

BSTNode* buildBST(char **elemente, int numarElemente, int colIndex, int* rowIndex) {
    BSTNode *root = NULL;
    for (int i = 0; i < numarElemente; i++) {
        root = insert(root, elemente[i], colIndex, rowIndex);
    }
    return root;
}



void addResult(BSTNode ***results, int *resultCount, BSTNode *node) {
    (*results) = realloc(*results, (*resultCount + 1) * sizeof(BSTNode *));
    if (*results == NULL) {
        printf("Eroare la alocarea memoriei pentru rezultate.\n");
        return;
    }
    (*results)[*resultCount] = node;
    (*resultCount)++;
}

void searchBST(BSTNode *root, char *target, BSTNode ***results, int *resultCount) {
    if (root == NULL) return;

    int cmp = strcmp(target, root->word);

    if (cmp < 0) {
        searchBST(root->left, target, results, resultCount);
    }

    else if (cmp > 0) {
        searchBST(root->right, target, results, resultCount);
    }

    else {
        addResult(results, resultCount, root);
        searchBST(root->left, target, results, resultCount);
        searchBST(root->right, target, results, resultCount);
    }
}


BSTNode **findNodesWithValue(BSTNode *root, char *target, int *foundCount) {
    *foundCount = 0;
    BSTNode **results = NULL;

    searchBST(root, target, &results, foundCount);

    return results;
}

void collectNodesExcluding(BSTNode *root, BSTNode ***nodes, int *count, int *size, char *excludeValue) {
    if (!root) return;

    collectNodesExcluding(root->left, nodes, count, size, excludeValue);

    if (strcmp(root->word, excludeValue) != 0) {
        if (*count >= *size) {
            *size *= 2;
            *nodes = realloc(*nodes, (*size) * sizeof(BSTNode *));
        }
        (*nodes)[(*count)++] = root;
    }

    collectNodesExcluding(root->right, nodes, count, size, excludeValue);
}

BSTNode **getNodesExcluding(BSTNode *root, char *excludeValue, int *count) {
    if (!root) return NULL;

    int size = 10;
    *count = 0;
    BSTNode **result = (BSTNode **)malloc(size * sizeof(BSTNode *));
    collectNodesExcluding(root, &result, count, &size, excludeValue);
    return result;
}


void collectNodesByCondition(BSTNode *root, char *value, char *operator, BSTNode ***result, int *count, int *size) {
    if (!root) return;

    int comparison = strcmp(root->word, value);
    int matches = 0;

    if (strcmp(operator, "<") == 0) {
        matches = (comparison < 0);
    } else if (strcmp(operator, ">") == 0) {
        matches = (comparison > 0);
    } else if (strcmp(operator, "<=") == 0) {
        matches = (comparison <= 0);
    } else if (strcmp(operator, ">=") == 0) {
        matches = (comparison >= 0);
    }

    if (matches) {
        if (*count >= *size) {
            *size *= 2;
            *result = realloc(*result, (*size) * sizeof(BSTNode *));
        }
        (*result)[(*count)++] = root;
    }

    collectNodesByCondition(root->left, value, operator, result, count, size);
    collectNodesByCondition(root->right, value, operator, result, count, size);
}

BSTNode **getNodesByCondition(BSTNode *root, char *value, char *operator, int *count) {
    int size = 10;
    *count = 0;
    BSTNode **result = (BSTNode **)malloc(size * sizeof(BSTNode *));
    if (!result) {
        printf("Eroare la alocarea memoriei.\n");
        return NULL;
    }

     collectNodesByCondition(root, value, operator, &result, count, &size);
    return result;
}

#include <stdlib.h>

void freeBST(BSTNode *node) {
    if (node == NULL) {
        return; 
    }
    freeBST(node->left);
    freeBST(node->right);

    if (node->word != NULL) {
        free(node->word);
    }

    free(node);
}

