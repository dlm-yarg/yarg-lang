#ifndef cyarg_sync_group_h
#define cyarg_sync_group_h

#include <stdio.h>
#include "object_type.h"
#include "platform_hal.h"

ObjPtr newSyncGroup(ObjPtr routine, ObjPtr items);

void freeSyncGroup(ObjPtr group);
void markSyncGroup(ObjPtr group);

void printSyncGroup(FILE* op, ObjPtr group);

ObjPtr receiveSyncGroup(ObjPtr group);

platform_critical_section* getSyncGroupLock(ObjPtr group);

#endif
