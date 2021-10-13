#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include "bh_platform.h"
#include "wasm_export.h"

#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include "bh_platform.h"
#include "wasm_export.h"
#include "wasm_c_api_internal.h"
#include "wasm_memory.h"
#include "wasm_runtime_common.h"
#include "wasm_runtime.h"

char*
read_file_to_buffer(const char *filename, uint32 *ret_size)
{
    char *buffer;
    int file;
    uint32 file_size, buf_size, read_size;
    struct stat stat_buf;

    if (!filename || !ret_size) {
        printf("Read file to buffer failed: invalid filename or ret size.\n");
        return NULL;
    }

    if ((file = open(filename, O_RDONLY, 0)) == -1) {
        printf("Read file to buffer failed: open file %s failed.\n",
               filename);
        return NULL;
    }

    //file_size is not right
#if 0
    if (fstat(file, &stat_buf) != 0) {
        printf("Read file to buffer failed: fstat file %s failed.\n",
               filename);
        close(file);
        return NULL;
    }

    file_size = (uint32)stat_buf.st_size;

    /* At lease alloc 1 byte to avoid malloc failed */
    buf_size = file_size > 0 ? file_size : 1;
#endif
    file_size = 1 << 20;

    if (!(buffer = BH_MALLOC(file_size))) {
        printf("Read file to buffer failed: alloc memory failed.\n");
        close(file);
        return NULL;
    }

    read_size = (uint32)read(file, buffer, file_size);
    close(file);

    if (read_size == file_size) {
        printf("Read file to buffer failed: read file content failed. %d\n", read_size);
        BH_FREE(buffer);
        return NULL;
    }

    printf("Read file, total size: %u\n", read_size);

    *ret_size = read_size;
    return buffer;
}

int wamr(char *file)
{
    char error_buf[128] = { 0 };
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    uint32 stack_size = 16 * 1024, heap_size = 16 * 1024;

    const char *exception;

    if (!wasm_runtime_init()) {
        printf("wasm_runtime_init failed\n");
        goto error;
    }

    if (!(wasm_file_buf =
              (uint8 *)read_file_to_buffer(file, &wasm_file_size)))
    {
        printf("bh_read_file_to_buffer failed\n");
        goto error;
    }

    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("wasm_runtime_load failed %s\n", error_buf);
        goto error;
    }

    if (!(wasm_module_inst =
            wasm_runtime_instantiate(wasm_module, stack_size, heap_size,
                                     error_buf, sizeof(error_buf)))) {
        printf("wasm_runtime_instantiate failed %s\n", error_buf);
        goto error;
    }

    wasm_application_execute_main(wasm_module_inst, 0, NULL);
    if ((exception = wasm_runtime_get_exception(wasm_module_inst)))
        printf("%s\n", exception);

    wasm_runtime_deinstantiate(wasm_module_inst);

    printf("bye!\n");

error:
    return 0;
}
