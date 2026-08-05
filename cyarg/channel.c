#include "channel.h"

#include "platform_hal.h"
#include "vm.h"
#include "debug.h"
#include "yargtype.h"
#include "sync_group.h"

#include <stdio.h>
#ifdef CYARG_PTHREADS_SYNC
#include <semaphore.h>
#include <fcntl.h>   // for O_CREAT
#include <sys/stat.h> // for S_IRUSR and S_IWUSR
#endif

typedef struct ObjChannelContainer {
    Obj obj;
    bool overflow;
    size_t writeCursor;
    platform_critical_section lock;
    platform_critical_section* lock_access;
#ifdef CYARG_PTHREADS_SYNC
    sem_t* access;
#endif

    ObjPtr* buffer;
    size_t bufferSize;
    volatile size_t occupied;
} ObjChannelContainer;

ObjChannelContainer* newChannel(ObjRoutine* routine, size_t capacity) {
    ObjChannelContainer* channel = ALLOCATE_OBJ(ObjChannelContainer, OBJ_CHANNELCONTAINER);
    tempRootPush(&channel->obj);
    channel->overflow = false;

    channel->buffer = ALLOCATE(ObjPtr, capacity);
    channel->bufferSize = capacity;

    for (int i = 0; i < capacity; i++) {
        NIL_VAL(channel->buffer[i]);
    }
    platform_critical_section_init(&channel->lock);
    channel->lock_access = &channel->lock;
#ifdef CYARG_PTHREADS_SYNC
    channel->access = sem_open("/semaphore", O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (channel->access == SEM_FAILED) {
        tempRootPop();
        runtimeError(routine, "Semaphore open failed.");
    }
#endif
    tempRootPop();
    return channel;
}

void freeChannelObject(Obj* object) {
    ObjChannelContainer* channel = (ObjChannelContainer*)object;
    platform_critical_section_deinit(&channel->lock);
#ifdef CYARG_PTHREADS_SYNC    
    sem_close(channel->access);
#endif

    FREE_ARRAY(ObjPtr, channel->buffer, channel->bufferSize);
    FREE(ObjChannelContainer, object); 
}

size_t readCursor(ObjChannelContainer const *channel) {
    size_t count = channel->occupied;
    size_t size = channel->bufferSize;
    size_t cursor = (channel->writeCursor + size - count) % size;
    return cursor;
}

static void channelMutexEnter(ObjChannelContainer* channel) {
    platform_critical_section_enter_blocking(channel->lock_access);
}

static void channelMutexLeave(ObjChannelContainer* channel) {
    platform_critical_section_exit(channel->lock_access);
}
 
void markChannel(ObjChannelContainer* channel) {
    if (channel->buffer != 0) {
        size_t cursor = readCursor(channel);
        for (int i = 0; i < channel->occupied; i++) {
            markValue(channel->buffer[cursor]);
            cursor = (cursor + 1) % channel->bufferSize;
        }
    }
}

void printChannel(FILE* op, ObjChannelContainer const *channel) {
    FPRINTMSG(op, "channel{");
    size_t cursor = readCursor(channel);
    for (int i = 0; i < channel->occupied; i++) {
        fprintValue(op, channel->buffer[cursor]);
        if (i < channel->occupied - 1) {
            FPRINTMSG(op, ", ");
        }
        cursor = (cursor + 1) % channel->bufferSize;
    }
    FPRINTMSG(op, "}");
}

void sendChannel(ObjChannelContainer* channel, ObjPtr data) {
#ifdef CYARG_PICO_BUSY_SYNC
    while (channel->occupied == channel->bufferSize) {
        // stall/block until space
        tight_loop_contents();
    }
#endif

    channelMutexEnter(channel);
    channel->buffer[channel->writeCursor] = data;
    channel->occupied++;
    channel->writeCursor = (channel->writeCursor + 1) % channel->bufferSize;
    channel->overflow = false;
    channelMutexLeave(channel);

#ifdef CYARG_PTHREADS_SYNC
    sem_post(channel->access);
#endif
}

ObjPtr collectFromChannel(ObjChannelContainer* channel) {
    ObjPtr result = NIL_VAL;

    channelMutexEnter(channel);
    size_t cursor = readCursor(channel);
    result = channel->buffer[cursor];
    channel->occupied--;
    channel->overflow = false;
    channelMutexLeave(channel);

    return result;
}

ObjPtr receiveChannel(ObjChannelContainer* channel) {

#if defined (CYARG_PICO_BUSY_SYNC)
    while (channel->occupied == 0) {
        // stall/block until data
        tight_loop_contents();
    }
#elif defined(CYARG_PTHREADS_SYNC)
    sem_wait(channel->access);
#else
#error "No channel synchronization implementation for this build."
#endif
    return collectFromChannel(channel);
}

bool shareChannel(ObjChannelContainer* channel, ObjPtr data) {
    bool result = false;

    channelMutexEnter(channel);
    channel->buffer[channel->writeCursor] = data;
    channel->occupied++;
    channel->writeCursor = (channel->writeCursor + 1) % channel->bufferSize;
    if (channel->occupied > channel->bufferSize) {
        channel->overflow = true;
        channel->occupied = channel->bufferSize;
    }
    result = channel->overflow;
    channelMutexLeave(channel);

#ifdef CYARG_PTHREADS_SYNC
    if (!result) {
        sem_post(channel->access);
    }
#endif

    return result;
}

ObjPtr peekChannel(ObjChannelContainer* channel) {
    ObjPtr result = 0;

    if (channel->occupied > 0) {
        result = channel->buffer[readCursor(channel)];
    }

    return result;
}

void joinSyncGroup(ObjChannelContainer* channel, ObjSyncGroup* group) {
    platform_critical_section_deinit(&channel->lock);
    channel->lock_access = getSyncGroupLock(group);
}

void leaveSyncGroup(ObjChannelContainer* channel, ObjSyncGroup* group) {
    platform_critical_section_init(&channel->lock);
    channel->lock_access = &channel->lock;
}
