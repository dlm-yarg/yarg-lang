#ifndef cyarg_channel_h
#define cyarg_channel_h

#include "object_type.h"

#include <stdio.h>
#include <stdbool.h>

typedef struct Obj Obj;
typedef struct ObjRoutine ObjRoutine;
typedef struct ObjChannelContainer ObjChannelContainer;
typedef struct ObjSyncGroup ObjSyncGroup;

ObjChannelContainer* newChannel(ObjRoutine* routine, size_t capacity);

void freeChannelObject(Obj* channel);
void markChannel(ObjChannelContainer* channel);

void printChannel(FILE* op, ObjChannelContainer* channel);

void sendChannel(ObjChannelContainer* channel, ObjPtr data);
ObjPtr receiveChannel(ObjChannelContainer* channel);
ObjPtr peekChannel(ObjChannelContainer* channel);
bool shareChannel(ObjChannelContainer* channel, ObjPtr data);

ObjPtr collectFromChannel(ObjChannelContainer* channel);
void joinSyncGroup(ObjChannelContainer* channel, ObjSyncGroup* group);
void leaveSyncGroup(ObjChannelContainer* channel, ObjSyncGroup* group);

#endif
