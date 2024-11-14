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
    } else if (strcmp(word, root->word) > 0) {
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

