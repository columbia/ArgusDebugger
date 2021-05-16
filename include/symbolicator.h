#if defined(__APPLE__)
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _MYSYMBOLICATOR_LIB_MACH_INFO_H
#define _MYSYMBOLICATOR_LIB_MACH_INFO_H
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <mach-o/fat.h>
#include <mach-o/swap.h>
#include <mach-o/loader.h>
#include <mach-o/dyld_images.h>
#include <string.h>
#include <stdio.h>
#include <mach/message.h>
#include <mach/task_info.h>
#include <mach/mach_traps.h>
#include <mach/mach_vm.h>
#include <libproc.h>
#include <unistd.h>

struct vm_sym {
    uint64_t vm_offset;
    uint64_t str_index;
};

struct mach_o_handler {
    void const *library_address; 
    void const *mach_address; 
    int64_t vm_slide;

    char const *string_table;  
    union{
        struct nlist_64 const *symbol_table_64;  
        struct nlist const *symbol_table_32;
    } symbol_table;
    uint32_t nsyms;
    uint32_t strsize;

    bool is_32bit;
    bool should_swap;
    
    /*output*/
    char * strings;
    struct vm_sym* symbol_arrays; 
};

struct segments_handler {
    union {
        struct segment_command_64 *seg_linkedit_ptr_64;
        struct segment_command *seg_linkedit_ptr_32;
    } seg_linkedit_ptr;
    union {
        struct segment_command_64 *seg_text_ptr_64;
        struct segment_command *seg_text_ptr_32;
    } seg_text_ptr;
    struct symtab_command *symtab_ptr;
    struct dysymtab_command *dysymtab_ptr;
};

bool get_syms_for_libpath(pid_t pid,
    const char *path,
    struct mach_o_handler *handler);
#endif
#ifdef __cplusplus
}
#endif
#endif
