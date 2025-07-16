#define TARGET_EXTENSION 1
#include "pd_api.h"
#include "helpers.h"

void drawCenteredBitmap(LCDBitmap *bitmap, int x, int y) {
	int width, height;
	pd->graphics->getBitmapData(bitmap, &width, &height, NULL, NULL, NULL);
	pd->graphics->drawBitmap(bitmap, x - width / 2, y - height / 2, kBitmapUnflipped);
}

// === QUEUE ===
void queueInit(Queue* q)
{
    q->front = -1;
    q->rear = -1;
}

void queueClear(Queue* q) {
	q->front = -1;
    q->rear = -1;
}

bool queueIsEmpty(Queue* q) {
	return q->front == -1;
}

bool queueIsFull(Queue* q) {
	return (q->rear + 1) % QUEUE_MAX_SIZE == q->front;
}

int queueCount(Queue* q) {
	if (queueIsEmpty(q)) {
		return 0;
	}
    if (q->rear >= q->front) {
        return q->rear - q->front + 1;
	} else {
        return QUEUE_MAX_SIZE - (q->front - q->rear - 1);
	}
}

void queuePush(Queue* q, int value)
{
    if (queueIsFull(q)) {
        return;
    }
    if (q->front == -1) {
        q->front = 0;
    }
    q->rear = (q->rear + 1) % QUEUE_MAX_SIZE;
    q->items[q->rear] = value;
}

int queuePop(Queue* q)
{
    if (queueIsEmpty(q)) {
        return -1;
    }
    int value = q->items[q->front];
    if (q->front == q->rear) {
        q->front = q->rear = -1;
    } else {
        q->front = (q->front + 1) % QUEUE_MAX_SIZE;
    }
    return value;
}

int queuePeek(Queue* q, int index)
{
    if (queueIsEmpty(q)) {
        return -1;
    }
    return q->items[(q->front + index) % QUEUE_MAX_SIZE];
}

// === STACK ===
void stackInit(Stack* s) {
    s->top = -1;
}

void stackClear(Stack* s) {
    s->top = -1;
}

bool stackIsEmpty(Stack* s) {
    return s->top == -1;
}

bool stackIsFull(Stack* s) {
    return s->top == STACK_MAX_SIZE - 1;
}

// Push an element onto the stack
void stackPush(Stack* s, int value) {
    if (stackIsFull(s)) {
        return;
    }
    s->items[++s->top] = value;
}

int stackPop(Stack* s) {
    if (stackIsEmpty(s)) {
        return -1;
    }
    return s->items[s->top--];
}

int stackPeek(Stack* s) {
    if (stackIsEmpty(s)) {
        return -1;
    }
    return s->items[s->top];
}

void stackShuffle(Stack* s) {
    if (stackIsEmpty(s)) {
        return;
    }

    // Fisher-Yates shuffle
    for (int i = s->top; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = s->items[i];
        s->items[i] = s->items[j];
        s->items[j] = temp;
    }
}

void stackPrint(Stack* s) {
    if (stackIsEmpty(s)) {
        pd->system->logToConsole("Stack is empty");
        return;
    }
    pd->system->logToConsole("Stack contents:");
    for (int i = 0; i <= s->top; i++) {
        pd->system->logToConsole("%d", s->items[i]);
    }
}

// === SET ===
// Hash function
int hash(int value) {
    return abs(value) % SET_SIZE;
}

// Initialize the set
void setInit(Set* set) {
    for (int i = 0; i < SET_SIZE; i++) {
        set->used[i] = false;
        set->deleted[i] = false;
    }
}

void setClear(Set* set) {
    setInit(set);
}

// Add a value to the set
void setAdd(Set* set, int value) {
    int index = hash(value);

    for (int i = 0; i < SET_SIZE; i++) {
        int probe = (index + i) % SET_SIZE;

        if (!set->used[probe] || set->deleted[probe]) {
            set->data[probe] = value;
            set->used[probe] = true;
            set->deleted[probe] = false;
            return;
        }

        // If value already exists, do nothing
        if (set->used[probe] && set->data[probe] == value) {
            return;
        }
    }
}

// Remove a value from the set
void setRemove(Set* set, int value) {
    int index = hash(value);

    for (int i = 0; i < SET_SIZE; i++) {
        int probe = (index + i) % SET_SIZE;

        if (set->used[probe] && set->data[probe] == value) {
            set->used[probe] = false;
            set->deleted[probe] = true;
            return;
        }

        if (!set->used[probe] && !set->deleted[probe]) {
            break; // Value not found
        }
    }
}

// Check if a value is in the set
bool setContains(Set* set, int value) {
    int index = hash(value);

    for (int i = 0; i < SET_SIZE; i++) {
        int probe = (index + i) % SET_SIZE;

        if (set->used[probe] && set->data[probe] == value) {
            return true;
        }

        if (!set->used[probe] && !set->deleted[probe]) {
            break;
        }
    }

    return false;
}