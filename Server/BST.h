#ifndef BST
#define BST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct BSTNode {
    char *word;
    int row;
    int column;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

BSTNode* createNode(char *word, int colIndex, int* rowIndex);
BSTNode* insert(BSTNode *root, char *word, int colIndex, int* rowIndez);
BSTNode* buildBST(char **elemente, int numarElemente,int colIndex, int* rowindex);
BSTNode **findNodesWithValue(BSTNode *root, char *target, int *foundCount);
void searchBST(BSTNode *root, char *target, BSTNode ***results, int *resultCount);
void addResult(BSTNode ***results, int *resultCount, BSTNode *node);
void collectNodesExcluding(BSTNode *root, BSTNode ***nodes, int *count, int *size, char *excludeValue);
BSTNode **getNodesExcluding(BSTNode *root, char *excludeValue, int *count);
BSTNode **getNodesByCondition(BSTNode *root, char *value, char *operator, int *count);
void collectNodesByCondition(BSTNode *root, char *value, char *operator, BSTNode ***result, int *count, int *size);
void freeBST(BSTNode *node);

#endif //BST