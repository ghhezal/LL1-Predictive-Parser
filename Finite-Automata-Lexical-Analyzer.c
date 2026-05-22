#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define maxStates 100

typedef struct Transition
{
    char symbol;
    int nextState;
    struct Transition *next;
} Transition;

Transition *transitions[maxStates];
int isFinal[maxStates];
int maxUsedState = 0;
int sharedFinalState = -1;

Transition *createTransition(char symbol, int nextState)
{
    Transition *t = (Transition *)malloc(sizeof(Transition));
    t->symbol = symbol;
    t->nextState = nextState;
    t->next = NULL;
    return t;
}

void addTransition(int from, char symbol, int nextState)
{
    Transition *t = createTransition(symbol, nextState);
    t->next = transitions[from];
    transitions[from] = t;
}

int getNextStates(int state, char symbol)
{
    Transition *t = transitions[state];
    while (t)
    {
        if (t->symbol == symbol)
            return t->nextState;
        t = t->next;
    }
    return -1;
}

int containsFinal(int states[], int n)
{
    for (int i = 0; i < n; i++)
    {
        if (states[i] != -1 && isFinal[states[i]] == 1)
            return 1;
    }
    return 0;
}

int main()
{
    FILE *f = fopen("grammar.txt", "r");
    if (!f)
    {
        printf("Cannot open grammar.txt\n");
        return 1;
    }

    for (int i = 0; i < maxStates; i++)
    {
        transitions[i] = NULL;
        isFinal[i] = 0;
    }

    // get final states

    char line[100];
    int stateNBR;
    int i = 0;
    fgets(line, sizeof(line), f); // read the "enter the final states:" line
    fgets(line, sizeof(line), f); // read the final states

    i = 0;
    while (line[i] != '\0')
    {
        while (isspace(line[i])) // skip spaces
            i++;
        if (line[i] == '\0')
            break;
        stateNBR = 0;
        while (isdigit(line[i]))
        {
            stateNBR = stateNBR * 10 + (line[i] - '0');
            i++;
        }

        isFinal[stateNBR] = 1;// mark it as final
    }

    // create automata
    fgets(line, sizeof(line), f); // read the "enter the transitions:" line
    while (fgets(line, sizeof(line), f))
    {
        int leftState = 0, nextState = 0;
        char symbol = 0;
        i = 0;

        while (isdigit(line[i]))
        {
            leftState = leftState * 10 + (line[i] - '0');
            i++;
        }

        if (leftState > maxUsedState) // eg: 1->c => a final state should be added.
            maxUsedState = leftState;

        while (line[i] && line[i] != '>') // skip "->".
            i++;
        i++;

        while (line[i] && isspace(line[i]))
            i++;

        symbol = line[i];
        i++;

        if (isdigit(line[i]))
        {
            while (isdigit(line[i]))
            {
                nextState = nextState * 10 + (line[i] - '0');
                i++;
            }
            addTransition(leftState, symbol, nextState);
            if (nextState > maxUsedState)
                maxUsedState = nextState;
        }
        else
        {
            if (sharedFinalState == -1)
            {
                maxUsedState++;
                sharedFinalState = maxUsedState;
                isFinal[sharedFinalState] = 1;
            }
            addTransition(leftState, symbol, sharedFinalState);
        }
    }
    fclose(f);

    f = fopen("text.txt", "r");
    if (!f)
    {
        printf("Cannot open grammar.txt\n");
        return 1;
    }

    char text[100], word[30];
    fgets(text, sizeof(text), f);     // read the "enter the text:" line
    fgets(text, sizeof(text), f);     // read the text
    text[strcspn(text, "\n")] = '\0'; // remove "\n"

    fgets(text, sizeof(text), f);

    int textIndex = 0, wordIndex, state;
    char symbol;
    bool continueLoop;
    while (text[textIndex] != "\0")
    {
        wordIndex = 0;
        state = 0;
        continueLoop = false;
        while (isspace(text[textIndex]))
            textIndex++;
        while (text[textIndex] != '\0' && wordIndex < 29)
        {
            symbol = word[wordIndex++] = text[textIndex++];
            state = getNextStates(state, symbol);
            if (state == -1 && isFinal[state] != 1)
            {
                word[wordIndex] = "\0";
                printf("%s Rejected at symbol '%c' (number %d)\n", word, symbol, wordIndex);
                continueLoop = true;
                continue;
            }
        }

        if (!continueLoop)
            continue;
        printf("The word %s is ACCEPTED\n", word);
    }
}