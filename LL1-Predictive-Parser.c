#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define maxStates 100
#define maxTerminals 50

typedef struct TreeNode
{
    char symbol[50];
    struct TreeNode *children[10];
    int childCount;
} TreeNode;

TreeNode *createNode(char *symbol)
{
    TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
    strcpy(node->symbol, symbol);
    node->childCount = 0;
    for (int i = 0; i < 10; i++)
        node->children[i] = NULL;
    return node;
}

void addChild(TreeNode *parent, TreeNode *child)
{
    if (parent->childCount < 10)
    {
        parent->children[parent->childCount++] = child;
    }
}

void printTree(TreeNode *root, int level)
{
    if (root == NULL)
        return;
    for (int i = 0; i < level; i++)
    {
        if (i == level - 1)
            printf("|-- ");
        else
            printf("|   ");
    }
    printf("%s\n", root->symbol);
    for (int i = 0; i < root->childCount; i++)
    {
        printTree(root->children[i], level + 1);
    }
}

typedef struct Transition
{
    char body[100];
    struct Transition *next;
} Transition;

typedef struct SetNode
{
    char symbol[20];
    struct SetNode *next;
} SetNode;

Transition *transitions[maxStates];
char terminals[maxTerminals][20];
int terminalCount = 0;
int maxUsedStates = 0;
SetNode *firstSets[maxStates];
SetNode *followSets[maxStates];
char analyseTable[maxStates][maxTerminals][200];

typedef struct StackItem
{
    char symbol[20];
    TreeNode *node;
} StackItem;

StackItem stack[100];
int top = -1;

void push(char *s, TreeNode *node)
{
    top++;
    strcpy(stack[top].symbol, s);
    stack[top].node = node;
}

void pop()
{
    if (top >= 0)
        top--;
}

StackItem peekItem()
{
    if (top == -1)
    {
        StackItem empty = {"", NULL};
        return empty;
    }
    return stack[top];
}

void printStack(char *buffer)
{
    buffer[0] = '\0';
    for (int i = 0; i <= top; i++)
    {
        strcat(buffer, stack[i].symbol);
    }
}

bool addToSet(SetNode **headRef, char *symbol)
{
    SetNode *current = *headRef;
    while (current != NULL)
    {
        if (strcmp(current->symbol, symbol) == 0)
            return false;
        current = current->next;
    }
    SetNode *newNode = (SetNode *)malloc(sizeof(SetNode));
    if (!newNode)
        exit(1);
    strcpy(newNode->symbol, symbol);
    newNode->next = *headRef;
    *headRef = newNode;
    return true;
}

bool mergeSets(SetNode **destRef, SetNode *src, bool includeEpsilon)
{
    bool changed = false;
    SetNode *current = src;
    while (current != NULL)
    {
        if (strcmp(current->symbol, "ep") == 0 && !includeEpsilon)
        {
            // Skip epsilon (ep)
        }
        else
        {
            if (addToSet(destRef, current->symbol))
                changed = true;
        }
        current = current->next;
    }
    return changed;
}

bool hasSymbol(SetNode *head, char *symbol)
{
    while (head != NULL)
    {
        if (strcmp(head->symbol, symbol) == 0)
            return true;
        head = head->next;
    }
    return false;
}

int getNextToken(char *str, int *pos, char *buffer)
{
    int i = *pos;
    while (str[i] != '\0' && isspace(str[i]))
        i++;
    if (str[i] == '\0')
    {
        *pos = i;
        return 0;
    }

    if (strncmp(&str[i], "ep", 2) == 0)
    {
        strcpy(buffer, "ep");
        *pos = i + 2;
        return 2;
    }

    int bestK = -1;
    int maxLen = 0;
    for (int k = 0; k < terminalCount; k++)
    {
        int len = strlen(terminals[k]);
        if (strncmp(&str[i], terminals[k], len) == 0)
        {
            if (len > maxLen)
            {
                maxLen = len;
                bestK = k;
            }
        }
    }
    if (bestK != -1)
    {
        strcpy(buffer, terminals[bestK]);
        *pos = i + maxLen;
        return 2;
    }

    if (isdigit(str[i]))
    {
        buffer[0] = str[i];
        buffer[1] = '\0';
        *pos = i + 1;
        return 1;
    }

    buffer[0] = str[i];
    buffer[1] = '\0';
    *pos = i + 1;
    return 2;
}

void computeFirst()
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (int i = 0; i <= maxUsedStates; i++)
        {
            Transition *t = transitions[i];
            while (t != NULL)
            {
                int pos = 0;
                char token[50];
                bool allPreviousNullable = true;
                while (true)
                {
                    int type = getNextToken(t->body, &pos, token);
                    if (type == 0)
                        break;
                    if (type == 2)
                    {
                        if (addToSet(&firstSets[i], token))
                            changed = true;
                        allPreviousNullable = false;
                        break;
                    }
                    if (type == 1)
                    {
                        int Y = atoi(token);
                        if (mergeSets(&firstSets[i], firstSets[Y], false))
                            changed = true;

                        if (!hasSymbol(firstSets[Y], "ep"))
                        {
                            allPreviousNullable = false;
                            break;
                        }
                    }
                }
                if (allPreviousNullable)
                {
                    if (addToSet(&firstSets[i], "ep"))
                        changed = true;
                }
                t = t->next;
            }
        }
    }
}

void computeFollow()
{
    addToSet(&followSets[0], "$");
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (int A = 0; A <= maxUsedStates; A++)
        {
            Transition *t = transitions[A];
            while (t != NULL)
            {
                struct
                {
                    int type;
                    char val[50];
                } tokens[50];
                int count = 0;
                int pos = 0;
                while (1)
                {
                    int type = getNextToken(t->body, &pos, tokens[count].val);
                    if (type == 0)
                        break;
                    tokens[count].type = type;
                    count++;
                }
                for (int k = 0; k < count; k++)
                {
                    if (tokens[k].type == 1)
                    {
                        int B = atoi(tokens[k].val);
                        bool betaNullable = true;
                        for (int next = k + 1; next < count; next++)
                        {
                            if (tokens[next].type == 2)
                            {
                                if (strcmp(tokens[next].val, "ep") != 0)
                                {
                                    if (addToSet(&followSets[B], tokens[next].val))
                                        changed = true;
                                }
                                betaNullable = false;
                                break;
                            }
                            if (tokens[next].type == 1)
                            {
                                int Y = atoi(tokens[next].val);
                                if (mergeSets(&followSets[B], firstSets[Y], false))
                                    changed = true;

                                if (!hasSymbol(firstSets[Y], "ep"))
                                {
                                    betaNullable = false;
                                    break;
                                }
                            }
                        }
                        if (betaNullable)
                        {
                            if (mergeSets(&followSets[B], followSets[A], true))
                                changed = true;
                        }
                    }
                }
                t = t->next;
            }
        }
    }
}

int getTerminalIndex(char *t)
{
    for (int i = 0; i < terminalCount; i++)
    {
        if (strcmp(terminals[i], t) == 0)
            return i;
    }
    return -1;
}

bool ruleAlreadyInCell(char *cell, char *rule)
{
    char cellCopy[200];
    strcpy(cellCopy, cell);
    char *entry = strtok(cellCopy, "|");
    while (entry != NULL)
    {
        while (*entry == ' ')
            entry++;
        int len = strlen(entry);
        while (len > 0 && entry[len - 1] == ' ')
            entry[--len] = '\0';
        if (strcmp(entry, rule) == 0)
            return true;
        entry = strtok(NULL, "|");
    }
    return false;
}

void addToTable(int state, int col, char *rule)
{
    if (col == -1)
        return;

    if (analyseTable[state][col][0] == '\0')
    {
        strcpy(analyseTable[state][col], rule);
    }
    else
    {
        if (!ruleAlreadyInCell(analyseTable[state][col], rule))
        {
            strcat(analyseTable[state][col], " | ");
            strcat(analyseTable[state][col], rule);
        }
    }
}

void analyze()
{
    for (int i = 0; i < maxStates; i++)
    {
        for (int j = 0; j < maxTerminals; j++)
        {
            analyseTable[i][j][0] = '\0';
        }
    }

    if (getTerminalIndex("$") == -1)
    {
        strcpy(terminals[terminalCount], "$");
        terminalCount++;
    }

    for (int i = 0; i <= maxUsedStates; i++)
    {
        Transition *t = transitions[i];
        while (t != NULL)
        {
            SetNode *firstAlpha = NULL;
            int pos = 0;
            char token[50];
            bool allNullable = true;

            while (1)
            {
                int type = getNextToken(t->body, &pos, token);
                if (type == 0)
                    break;

                if (type == 2)
                {
                    if (strcmp(token, "ep") == 0)
                    {
                    }
                    else
                    {
                        addToSet(&firstAlpha, token);
                        allNullable = false;
                    }
                    break;
                }
                if (type == 1)
                {
                    int Y = atoi(token);
                    mergeSets(&firstAlpha, firstSets[Y], false);
                    if (!hasSymbol(firstSets[Y], "ep"))
                    {
                        allNullable = false;
                        break;
                    }
                }
            }
            if (allNullable)
                addToSet(&firstAlpha, "ep");

            char ruleString[120];
            snprintf(ruleString, sizeof(ruleString), "%d->%s", i, t->body);

            SetNode *current = firstAlpha;
            while (current != NULL)
            {
                if (strcmp(current->symbol, "ep") != 0)
                {
                    addToTable(i, getTerminalIndex(current->symbol), ruleString);
                }
                else
                {
                    SetNode *fNode = followSets[i];
                    while (fNode != NULL)
                    {
                        addToTable(i, getTerminalIndex(fNode->symbol), ruleString);
                        fNode = fNode->next;
                    }
                }
                current = current->next;
            }
            t = t->next;
        }
    }
}

void Analye()
{
    char inputBuffer[200];
    printf("\nEnter the string to be analyzed: ");
    fflush(stdout);
    if (fgets(inputBuffer, sizeof(inputBuffer), stdin) == NULL)
        return;
    inputBuffer[strcspn(inputBuffer, "\n")] = 0;

    char startSymbol[10];
    sprintf(startSymbol, "%d", 0);
    TreeNode *rootNode = createNode(startSymbol);

    top = -1;
    push("$", NULL);
    push(startSymbol, rootNode);

    printf("\n----------------------------------------------------------------------\n");
    printf("| %-20s | %-20s | %-20s |\n", "Stack", "String", "Action");
    printf("----------------------------------------------------------------------\n");

    int inputPos = 0;
    char action[200];
    char stackStr[100];
    char currentToken[50];
    bool accepted = false;

    while (1)
    {
        printStack(stackStr);
        char *remainingInput = inputBuffer + inputPos;
        StackItem item = peekItem();
        char *X = item.symbol;
        TreeNode *currentNode = item.node;

        int tempPos = inputPos;
        int tokenType = getNextToken(inputBuffer, &tempPos, currentToken);
        if (tokenType == 0)
            strcpy(currentToken, "$");

        if (strcmp(X, "$") == 0 && strcmp(currentToken, "$") == 0)
        {
            sprintf(action, "Accepted");
            printf("| %-20s | %-20s | %-20s |\n", stackStr, remainingInput, action);
            accepted = true;
            break;
        }
        else if (strcmp(X, currentToken) == 0)
        {
            sprintf(action, "POP, Forward");
            printf("| %-20s | %-20s | %-20s |\n", stackStr, remainingInput, action);
            pop();
            getNextToken(inputBuffer, &inputPos, currentToken);
        }
        else
        {
            int stateIndex = atoi(X);
            int termIndex = getTerminalIndex(currentToken);

            if (termIndex == -1 && strcmp(currentToken, "$") != 0)
            {
                printf("| %-20s | %-20s | %-20s |\n", stackStr, remainingInput, "Error: Unknown Token");
                return;
            }

            if (strcmp(currentToken, "$") == 0)
                termIndex = getTerminalIndex("$");

            if (termIndex == -1 || analyseTable[stateIndex][termIndex][0] == '\0')
            {
                printf("| %-20s | %-20s | %-20s |\n", stackStr, remainingInput, "Error: No Rule");
                return;
            }

            char *rule = analyseTable[stateIndex][termIndex];
            char ruleCopy[100];
            strcpy(ruleCopy, rule);

            char *bodyStart = strstr(ruleCopy, "->");
            if (bodyStart)
                bodyStart += 2;
            else
                bodyStart = ruleCopy;

            char bodyTokens[20][20];
            int count = 0;
            int bPos = 0;
            char bTok[50];

            while (1)
            {
                int type = getNextToken(bodyStart, &bPos, bTok);
                if (type == 0)
                    break;
                strcpy(bodyTokens[count++], bTok);
            }

            strcpy(action, "POP");
            if (count == 1 && strcmp(bodyTokens[0], "ep") == 0)
            {
                strcat(action, ", PUSH ep");
                TreeNode *epNode = createNode("ep");
                addChild(currentNode, epNode);
            }
            else
            {
                if (count > 0)
                    strcat(action, ", PUSH");
                for (int i = 0; i < count; i++)
                {
                    strcat(action, " ");
                    strcat(action, bodyTokens[i]);
                }

                TreeNode *childrenNodes[20];
                for (int i = 0; i < count; i++)
                {
                    childrenNodes[i] = createNode(bodyTokens[i]);
                    addChild(currentNode, childrenNodes[i]);
                }

                printf("| %-20s | %-20s | %-20s |\n", stackStr, remainingInput, action);
                pop();

                for (int i = count - 1; i >= 0; i--)
                {
                    push(bodyTokens[i], childrenNodes[i]);
                }
                continue;
            }

            printf("| %-20s | %-20s | %-20s |\n", stackStr, remainingInput, action);
            pop();
        }
    }
    printf("----------------------------------------------------------------------\n");

    if (accepted)
    {
        printf("\nSyntax Tree:\n");
        printTree(rootNode, 0);
        printf("\n");
    }
}

bool Check()
{
    bool isLL1 = true;
    for (int i = 0; i <= maxUsedStates; i++)
    {
        for (int j = 0; j < terminalCount; j++)
        {
            if (strstr(analyseTable[i][j], " | ") != NULL)
            {
                printf("Conflict detected at State %d, Input '%s': { %s }\n",
                       i, terminals[j], analyseTable[i][j]);
                isLL1 = false;
            }
        }
    }
    return isLL1;
}

void printSet(SetNode *head)
{
    if (!head)
    {
        printf(" ");
        return;
    }
    while (head)
    {
        printf("%s", head->symbol);
        if (head->next)
            printf(", ");
        head = head->next;
    }
    printf(" ");
}

void printTable()
{
    printf("Analysis Table:\n");

    printf("%-6s", "State");
    for (int j = 0; j < terminalCount; j++)
    {
        printf("| %-10s", terminals[j]);
    }
    printf("\n");

    for (int k = 0; k < 6 + (terminalCount * 13); k++)
        printf("-");
    printf("\n");

    for (int i = 0; i <= maxUsedStates; i++)
    {
        bool empty = true;
        for (int j = 0; j < terminalCount; j++)
            if (analyseTable[i][j][0])
                empty = false;

        if (!empty)
        {
            printf(" %-5d", i);
            for (int j = 0; j < terminalCount; j++)
            {
                if (analyseTable[i][j][0] != '\0')
                    printf("| %-10s", analyseTable[i][j]);
                else
                    printf("| %-10s", " ");
            }
            printf("\n");
        }
    }
}

void addTransition(int from, char body[])
{
    Transition *t = (Transition *)malloc(sizeof(Transition));
    strcpy(t->body, body);
    t->next = transitions[from];
    transitions[from] = t;
}

int main()
{
    FILE *f = fopen("grammar.txt", "r");
    if (!f)
    {
        printf("Error: grammar.txt not found.\n");
        return 1;
    }

    for (int i = 0; i < maxStates; i++)
    {
        transitions[i] = NULL;
        firstSets[i] = NULL;
        followSets[i] = NULL;
    }

    char line[200];

    fgets(line, sizeof(line), f);
    if (fgets(line, sizeof(line), f))
    {
        line[strcspn(line, "\n")] = 0;
        char *token = strtok(line, " ");
        while (token != NULL && terminalCount < maxTerminals)
        {
            strcpy(terminals[terminalCount], token);
            terminalCount++;
            token = strtok(NULL, " ");
        }
    }

    while (fgets(line, sizeof(line), f))
    {
        char *arrow = strstr(line, "->");
        if (!arrow)
            arrow = strstr(line, ">");

        if (arrow)
        {
            int head = 0;
            int i = 0;
            while (line[i] != '\0' && line + i < arrow)
            {
                if (isdigit(line[i]))
                    head = head * 10 + (line[i] - '0');
                i++;
            }
            if (head > maxUsedStates)
                maxUsedStates = head;

            char *bodyStart = arrow + 1;
            if (arrow[0] == '-' && arrow[1] == '>')
                bodyStart = arrow + 2;
            bodyStart[strcspn(bodyStart, "\r\n")] = 0;

            addTransition(head, bodyStart);
        }
    }
    fclose(f);

    computeFirst();
    computeFollow();

    printf("\n%-10s | %-30s | %-30s\n", "State", "First", "Follow");
    printf("--------------------------------------------------------------------------------\n");
    for (int i = 0; i <= maxUsedStates; i++)
    {
        if (transitions[i] || firstSets[i] || followSets[i])
        {
            printf(" %-4d       | ", i);
            printSet(firstSets[i]);
            printf("\t\t\t | ");
            printSet(followSets[i]);
            printf("\n");
        }
    }

    analyze();
    printTable();

    if (Check())
    {
        printf("This grammar is LL(1).\n");
        Analye();
    }
    else
    {
        printf("This grammar is NOT LL(1) due to conflicts.\n");
    }

    return 0;
}