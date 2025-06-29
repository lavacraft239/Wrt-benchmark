/*
#include "wrt.h"
*/
import "C"
import "unsafe"

func StartWrt(url string) {
    cUrl := C.CString(url)
    defer C.free(unsafe.Pointer(cUrl))

    var config C.wrt_config_t
    C.wrt_init_config(&config)
    C.strncpy(&config.url[0], cUrl, C.sizeof_char*len(url))
    C.wrt_start(&config)
}
