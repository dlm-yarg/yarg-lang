#ifndef cyarg_native_h
#define cyarg_native_h

#include "value.h"

bool clockNative(ObjRoutine* routine, int argCount);
bool clock_get_hzNative(ObjRoutine* routine, int argCount);

bool irq_add_shared_handlerNative(ObjRoutine* routine, int argCount);
bool irq_remove_handlerNative(ObjRoutine* routine, int argCount);

bool stdin_getsNative(ObjRoutine* routine, int argCount);
bool stdin_eofNative(ObjRoutine* routine, int argCount);
bool stdout_putsNative(ObjRoutine* routine, int argCount);
bool stderr_putsNative(ObjRoutine* routine, int argCount);
bool stderr_put_newlineNative(ObjRoutine* routine, int argCount);

bool readFileIntoBufferNative(ObjRoutine* routine, int argCount);
bool fileSizeNative(ObjRoutine* routine, int argCount);
bool fileExistsNative(ObjRoutine* routine, int argCount);

bool vm_xip_string_nodeNative(ObjRoutine* routine, int argCount);

#if defined(CYARG_FEATURE_HOSTED_REPL)
bool host_argcNative(ObjRoutine* routine, int argCount);
bool host_argnNative(ObjRoutine* routine, int argCount);
bool host_exitCodeNative(ObjRoutine* routine, int argCount);
#endif

#endif
