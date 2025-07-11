#pragma once

#include "pd_api.h"
#include "globals.h"

// === MATH HELPERS ===
#define ABS(x) (((x) < 0) ? -(x) : (x))

static inline float fastRsqrt(float x)
{
	union {
		float f;
		uint32_t u;
	} f32u32;
	f32u32.f = x;
	f32u32.u = 0x5f3759df - (f32u32.u >> 1);
	f32u32.f *= 1.5f - (x * 0.5f * f32u32.f * f32u32.f);
	return f32u32.f;
}
static inline float fastRcp(float x) { x = fastRsqrt(x); return x * x; }
static inline float fastDiv(float x, float y) { return x * fastRcp(y); }
static inline float fastSqrt(float x) { return x * fastRsqrt(x); }
static inline float distanceSquared(float x1, float y1, float x2, float y2)
{
	return (y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1);
}

static inline uint32_t swap(uint32_t n)
{
#if TARGET_PLAYDATE
	//return __REV(n);
	uint32_t result;

	__asm volatile ("rev %0, %1" : "=l" (result) : "l" (n));
	return(result);
#else
	return ((n & 0xff000000) >> 24) | ((n & 0xff0000) >> 8) | ((n & 0xff00) << 8) | (n << 24);
#endif
}

static inline float lerp(float start, float end, float t) {
	return (1 - t) * start + t * end;
}

static inline float clampFloat(float d, float min, float max) {
	const float t = d < min ? min : d;
	return t > max ? max : t;
}

static inline int clampInt(int d, int min, int max) {
	const int t = d < min ? min : d;
	return t > max ? max : t;
}

static inline float randFloat(float min, float max) {
    return min + (float)rand() / RAND_MAX * (max - min);
}

// Inclusive
static inline int randInt(int min, int max) {
	return (rand() % (max - min + 1)) + min;
}

static inline float radToDeg(float radAngle) {
	return radAngle * (180 / PI);
}

// === DRAWING HELPERS ===
void drawCenteredBitmap(LCDBitmap *bitmap, int x, int y);

// === QUEUE ===
#define QUEUE_MAX_SIZE 30

typedef struct {
    int items[QUEUE_MAX_SIZE];
    int front;
    int rear;
} Queue;

void queueInit(Queue* q);
void queueClear(Queue* q);
bool queueIsEmpty(Queue* q);
bool queueIsFull(Queue* q);
int queueCount(Queue* q);
void queuePush(Queue* q, int value);
int queuePop(Queue* q);
int queuePeek(Queue* q, int index);

// === STACK ===
#define STACK_MAX_SIZE 30

typedef struct {
    int items[STACK_MAX_SIZE];
    int top;
} Stack;

void stackInit(Stack* s);
void stackClear(Stack* s);
bool stackIsEmpty(Stack* s);
bool stackIsFull(Stack* s);
void stackPush(Stack* s, int value);
int stackPop(Stack* s);
int stackPeek(Stack* s);
void stackShuffle(Stack* s);

// === SET ===
#define SET_SIZE 53 // Use a prime number for better distribution

typedef struct {
    int data[SET_SIZE];
    bool used[SET_SIZE];
    bool deleted[SET_SIZE];
} Set;

void setInit(Set* set);
void setAdd(Set* set, int value);
void setRemove(Set* set, int value);
bool setContains(Set* set, int value);