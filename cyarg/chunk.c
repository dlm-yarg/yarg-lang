#include "chunk.h"

#include "vm.h"

#include <stdlib.h>
#include <assert.h>

void initChunk(Chunk* chunk) {
    daInit(chunk->code, sizeof (uint8_t));
    daInit(chunk->lines, sizeof (int));
    daInit(&chunk->constants, sizeof (ObjPtr));
}

void freeChunk(Chunk* chunk) {
    daFree(chunk->code);
    daFree(chunk->lines);
    daFree(&chunk->constants); // todo how!
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
    daPushBack(chunk->code, &byte);
}

int addConstant(Chunk* chunk, ObjPtr o) {
    daPushBack(&chunk->constants, &o);
    return chunk->constants.arrayLength - 1;
}
