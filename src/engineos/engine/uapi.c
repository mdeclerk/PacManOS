#include "engineos/engine/uapi.h"
#include "engineos/engine/screen.h"
#include "engineos/include/ramfs.h"
#include "engineos/include/fb.h"
#include "engineos/include/heap.h"
#include "engineos/include/log.h"
#include "engineos/include/screen.h"
#include "engineos/include/timer.h"
#include "stdlib/string.h"

#define UAPI_SYM_NAME_MAX 64u

struct uapi_export {
    const char *name;
    uintptr_t addr;
};

#define UAPI_EXPORT_SYMBOL(fn, export_name) { ((export_name) != NULL ? (export_name) : #fn), (uintptr_t)(fn) },

static const struct uapi_export uapi_exports[] = {
#include "engineos/engine/uapi.def"
};

uintptr_t uapi_resolve_symbol(const char *name)
{
    for (size_t i = 0; i < sizeof(uapi_exports) / sizeof(uapi_exports[0]); i++) {
        if (!strneq(name, uapi_exports[i].name, UAPI_SYM_NAME_MAX))
            continue;
        return uapi_exports[i].addr;
    }
    
    return 0u;
}
