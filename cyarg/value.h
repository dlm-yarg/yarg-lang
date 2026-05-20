#ifndef cyarg_value_h
#define cyarg_value_h

// Objs are used for globals, constants or dynamic (stack/heap)
// up values point to objects further down the stack.
// Object carry their own type if required - struct, packed array.
// pointer objects reference objects.

//before -- Debug,Address sanitiser,Stack after return, Undef behaviour,Main thread,Malloc scribble,Zombie
//gh.yarg-lang(44713,0x1fcbe2240) malloc: enabling scribbling to detect mods to free blocks
//int-perform benchmark: job size: 100, max job length: 10secs
//Generated 100 non-zero random numbers in 100 calls to int_rand()
//Add (x+y) time: 11secs, ops: 6612000, 601ops/ms
//Sub (x-y) time: 10secs, ops: 7000000, 700ops/ms
//Mul (x*y) time: 11secs, ops: 442200, 40ops/ms
//Div (x/y) time: 11secs, ops: 676200, 61ops/ms
//Mod (x%y) time: 11secs, ops: 795600, 72ops/ms
//Negate (-x) time: 10secs, ops: 7000000, 700ops/ms
//Program ended with exit code: 0

#include "object.h"

#include <stdio.h>

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
#define IS_64BIT 1
#define IS_32BIT 0
#else
#define IS_64BIT 0
#define IS_32BIT 1
#endif


ObjPtr copyValue(ObjPtr);

bool is_positive_integer32(ObjPtr);
uint32_t as_positive_integer32(ObjPtr);

bool valuesEqual(ObjPtr, ObjPtr);

void printValue(ObjPtr);
void fprintValue(FILE *, ObjPtr);

bool is_uniformarray(ObjPtr);
bool is_struct(ObjPtr);
bool is_nil(ObjPtr);
bool is_channel(ObjPtr);

uintptr_t pinUniformArray(ObjPtr);

#endif
