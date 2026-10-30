#pragma once

#include "cpu/native_cpu.h"

namespace raven {

// Captures a software exception at the API boundary; guest handler search is not implemented.
NativeResult native_raise_exception(NativeProcess &process);
NativeResult native_raise_rtl_exception(NativeProcess &process, bool status_only);
// The exception Windows' default delay-load failure hook raises: noncontinuable, with the library and procedure names as parameters.
NativeResult native_delay_load_failure(NativeProcess &process);

}
