#ifndef cyarg_native_h
#define cyarg_native_h

#include "object.h"

typedef struct ObjRoutine ObjRoutine;

bool clockNative(ObjRoutine* routine, int argCount, ObjPtr result);
bool clock_get_hzNative(ObjRoutine* routine, int argCount, ObjPtr result);

bool irq_add_shared_handlerNative(ObjRoutine* routine, int argCount, ObjPtr result);
bool irq_remove_handlerNative(ObjRoutine* routine, int argCount, ObjPtr result);

bool stdin_getsNative(ObjRoutine* routine, int argCount, ObjPtr result);
bool stdin_eofNative(ObjRoutine* routine, int argCount, ObjPtr result);
bool stdout_putsNative(ObjRoutine* routine, int argCount, ObjPtr result);

#if defined(CYARG_FEATURE_HOSTED_REPL)
bool host_argcNative(ObjRoutine* routine, int argCount, ObjPtr result);
bool host_argnNative(ObjRoutine* routine, int argCount, ObjPtr result);
bool host_exitCodeNative(ObjRoutine* routine, int argCount, ObjPtr result);
#endif

#endif
