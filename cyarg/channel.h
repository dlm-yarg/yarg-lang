#ifndef cyarg_channel_h
#define cyarg_channel_h

typedef struct Obj Obj;
typedef struct ObjRoutine ObjRoutine;
typedef struct ObjChannelContainer ObjChannelContainer;
typedef struct ObjSyncGroup ObjSyncGroup;
typedef struct AbstractValue *Value;

ObjChannelContainer* newChannel(ObjRoutine* routine, size_t capacity);

void freeChannelObject(Obj* channel);
void markChannel(ObjChannelContainer* channel);

void printChannel(FILE* op, ObjChannelContainer* channel);

void sendChannel(ObjChannelContainer* channel, Value const data);
void receiveChannel(ObjChannelContainer* channel, Value r);
Value peekChannel(ObjChannelContainer* channel);
bool shareChannel(ObjChannelContainer* channel, Value const data);

void collectFromChannel(ObjChannelContainer* channel, Value r);
void joinSyncGroup(ObjChannelContainer* channel, ObjSyncGroup* group);
void leaveSyncGroup(ObjChannelContainer* channel, ObjSyncGroup* group);

#endif
