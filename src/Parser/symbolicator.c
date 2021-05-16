#if defined(__APPLE__)
#include "symbolicator.h"
#include "dyld_process_info_internal.h"
#include <assert.h>
#include <errno.h>
#define DEBUG 0
#define MAX_PATH 1024

static void *nullptr = NULL;

static inline bool fat_need_swap(uint32_t magic)
{
    if (magic == FAT_CIGAM)
        return true;
    return false;
}

static inline bool mach_need_swap(uint32_t magic)
{
    if (magic == MH_CIGAM_64)
        return true;
    return false;
}

static unsigned char *read_task_memory(mach_port_t task,
    mach_vm_address_t addr,
    mach_msg_type_number_t *size,
    kern_return_t *kr)
{
    mach_msg_type_number_t data_cnt = (mach_msg_type_number_t) *size;
    vm_offset_t read_mem;
    *kr = mach_vm_read(task, addr, data_cnt, &read_mem, size);
    if (*kr) {
#if DEBUG
        printf("fail to read task memory in %llx :  %s !\n",
            addr, mach_error_string(*kr));
#endif
        return nullptr;
    }
    return ((unsigned char *)read_mem);
}

static kern_return_t deallocate_mach_vm(mach_port_t task,
    mach_vm_address_t addr,
    mach_msg_type_number_t size)
{
    kern_return_t kret = mach_vm_deallocate(task,
        addr, size);
    if (kret != KERN_SUCCESS) {
#if DEBUG
        printf("fail to deallocate task memory in %p, %s !\n",
            (void *)addr, mach_error_string(kret));
#endif
    }
    return kret;
}

static kern_return_t dump_data_from_target(mach_port_t target_task,
    void *dest,
    mach_vm_address_t addr,
    mach_msg_type_number_t *size)
{
    kern_return_t kret;
    mach_msg_type_number_t data_cnt = *size;
    if (!addr || !data_cnt)
        return KERN_INVALID_ADDRESS;

    uint8_t *data = read_task_memory(target_task, addr, size, &kret);
    if (!data)
        return kret;

    data_cnt = *size < data_cnt ? *size : data_cnt;
    memcpy(dest, data, (size_t)data_cnt);
    kret = deallocate_mach_vm(mach_task_self(), (mach_vm_address_t)data, *size);
    return kret;
}

/*TODO: if lib is not loaded as fat
 * remove redundant operation
 */
static void *load_mach_from_fat(mach_port_t target_task, void *obj,
    bool should_swap) 
{
    kern_return_t kret;
    int header_size = sizeof(struct fat_header);
    int arch_size = sizeof(struct fat_arch);
    int arch_offset = header_size;
        
    struct fat_header dup_header;
    mach_msg_type_number_t size = header_size;
    
     kret = dump_data_from_target(target_task, &dup_header,
        (mach_vm_address_t)obj, &size);
    if (kret != KERN_SUCCESS)
        return nullptr;
    if (should_swap)
        swap_fat_header(&dup_header, 0);

    uint32_t magic;
    mach_vm_address_t arch;
    for (int i = 0; i < dup_header.nfat_arch; i++, arch_offset += arch_size) {
        arch = (mach_vm_address_t)(obj + arch_offset);
        size = arch_size;
        struct fat_arch dup_arch;
        kret = dump_data_from_target(target_task, &dup_arch, arch, &size);
        if (kret != KERN_SUCCESS)
            return nullptr;
        if (should_swap)
            swap_fat_arch(&dup_arch, 1, 0);

        size = 4;
        kret = dump_data_from_target(target_task, &magic,
            (mach_vm_address_t)(obj + dup_arch.offset), &size);
        if (kret != KERN_SUCCESS)
            return nullptr;

        if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64
            || magic == MH_MAGIC || magic == MH_CIGAM)
            return (void *)(obj + dup_arch.offset);
    }
    return nullptr;
}

static void *get_mach(mach_port_t target_task, void *obj,
    bool *should_swap, bool is_32bit)
{
    if (!obj)
        return nullptr;

    mach_msg_type_number_t size = 4;
    uint32_t magic;
    kern_return_t kret;
    
    kret = dump_data_from_target(target_task, &magic,
        (mach_vm_address_t)obj, &size); 
    if (kret != KERN_SUCCESS)
        return nullptr;

    void *img = obj;
    if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
        img = load_mach_from_fat(target_task, obj, fat_need_swap(magic));
        if (!img)
            return nullptr;

        kret = dump_data_from_target(target_task, &magic,
            (mach_vm_address_t)img, &size); 
        if (kret != KERN_SUCCESS)
            return nullptr;
    }

    *should_swap = mach_need_swap(magic);
    
    if ((!is_32bit && (magic == MH_MAGIC_64 || magic == MH_CIGAM_64))
        || (is_32bit && (magic == MH_MAGIC || magic == MH_CIGAM)))
        return img;

    return nullptr;
}

static bool load_segments_64(mach_port_t target_task,
    struct mach_o_handler *handler, struct segments_handler *segments_info)
{
    kern_return_t kret;
    struct mach_header_64 dup_image;
    mach_msg_type_number_t size = sizeof(struct mach_header_64);

    kret = dump_data_from_target(target_task, &dup_image,
        (mach_vm_address_t)(handler->mach_address), &size);
    if (kret != KERN_SUCCESS)
        return nullptr;
    
    if (handler->should_swap)
        swap_mach_header_64(&dup_image, 0);

    struct load_command * cmd = (struct load_command *)(
                (struct mach_header_64 *)(handler->mach_address) + 1);
    struct load_command dup_cmd;

    for (int index = 0; index < dup_image.ncmds; 
        index++, cmd = (struct load_command*)((void *)cmd + dup_cmd.cmdsize)) { 
        size = sizeof(struct load_command);
        kret = dump_data_from_target(target_task, &dup_cmd,
            (mach_vm_address_t)(cmd), &size);
        if (kret != KERN_SUCCESS)
            return false;
        if (handler->should_swap)
            swap_load_command(&dup_cmd, 0);

        switch(dup_cmd.cmd) {
            case LC_SEGMENT_64: {
                struct segment_command_64 dup_segcmd;
                size = sizeof(struct segment_command_64);
                kret = dump_data_from_target(target_task, &dup_segcmd,
                    (mach_vm_address_t)(cmd), &size);
                if (kret != KERN_SUCCESS)
                    return false;
                if (handler->should_swap)
                    swap_segment_command_64(&dup_segcmd, 0);
#if DEBUG
                printf("\n%s segment64 %p:", dup_segcmd.segname, cmd);
                printf("\n\tvm_addr %p", dup_segcmd.vmaddr);
                printf("\n\tfile_off %p", dup_segcmd.fileoff);
                printf("\n\tvm_size %p", dup_segcmd.vmsize);
                printf("\n\teg size %x\n", dup_segcmd.cmdsize);
#endif

                if (!strcmp(dup_segcmd.segname, SEG_TEXT)) {
                    segments_info->seg_text_ptr.seg_text_ptr_64
                        = (struct segment_command_64 *)cmd;
                    // TODO: check library_address
                    handler->vm_slide = (int64_t)(handler->library_address
                        - dup_segcmd.vmaddr);
                } else if (!strcmp(dup_segcmd.segname, SEG_LINKEDIT))
                    segments_info->seg_linkedit_ptr.seg_linkedit_ptr_64
                        = (struct segment_command_64 *)cmd;
                    break;
            }

            case LC_SYMTAB:
                segments_info->symtab_ptr = (struct symtab_command*)cmd;
                break;

            case LC_DYSYMTAB:
                segments_info->dysymtab_ptr = (struct dysymtab_command*)cmd;
                break;

            case LC_REEXPORT_DYLIB:
                break;

            default:
                break;
        }
    }

#if DEBUG
    printf("mach-o info:\
            \n\t\tlibrary loaded[vm] = %p;\
            \n\t\tmach-o64std::mapped[vm] = %p;\
            \n\t\tvm_slide = %p;\
            \n\t\tsymtab = %p;\
            \n\t\tdysymtab = %p;\
            \n\t\tseg_linkedit = %p;\n",
            handler->library_address,
            handler->mach_address,
            handler->vm_slide,
            segments_info->symtab_ptr,
            segments_info->dysymtab_ptr,
            segments_info->seg_linkedit_ptr);
#endif

    if (segments_info->symtab_ptr== nullptr 
        || segments_info->seg_linkedit_ptr.seg_linkedit_ptr_64 == nullptr)
        return false;

    return true;
}

static bool load_segments_32(mach_port_t target_task,
    struct mach_o_handler *handler, struct segments_handler *segments_info)
{
    kern_return_t kret;
    struct mach_header dup_image;
    mach_msg_type_number_t size = sizeof(struct mach_header);

    kret = dump_data_from_target(target_task, &dup_image,
        (mach_vm_address_t)(handler->mach_address), &size);
    if (kret != KERN_SUCCESS)
        return nullptr;
    
    if (handler->should_swap)
        swap_mach_header(&dup_image, 0);

    struct load_command * cmd = (struct load_command *)(
                (struct mach_header *)(handler->mach_address) + 1);
    struct load_command dup_cmd;

    for (int index = 0; index < dup_image.ncmds; 
        index++, cmd = (struct load_command*)((void *)cmd + dup_cmd.cmdsize)) { 
        size = sizeof(struct load_command);
        kret = dump_data_from_target(target_task, &dup_cmd,
            (mach_vm_address_t)(cmd), &size);
        if (kret != KERN_SUCCESS)
            return false;
        if (handler->should_swap)
            swap_load_command(&dup_cmd, 0);

        switch(dup_cmd.cmd) {
            case LC_SEGMENT: {
                struct segment_command dup_segcmd;
                size = sizeof(struct segment_command);
                kret = dump_data_from_target(target_task, &dup_segcmd,
                    (mach_vm_address_t)(cmd), &size);
                if (kret != KERN_SUCCESS)
                    return false;
                if (handler->should_swap)
                    swap_segment_command(&dup_segcmd, 0);
#if DEBUG
                printf("\n%s segment32 0x%x:", dup_segcmd.segname, cmd);
                printf("\n\tvm_addr 0x%x", dup_segcmd.vmaddr);
                printf("\n\tfile_off 0x%x", dup_segcmd.fileoff);
                printf("\n\tvm_size 0x%x", dup_segcmd.vmsize);
                printf("\n\teg size %x\n", dup_segcmd.cmdsize);
#endif

                if (!strcmp(dup_segcmd.segname, SEG_TEXT)) {
                    segments_info->seg_text_ptr.seg_text_ptr_32
                        = (struct segment_command *)cmd;
                    // TODO: check library_address
                    handler->vm_slide = (int32_t)(handler->library_address
                        - dup_segcmd.vmaddr);
                } else if (!strcmp(dup_segcmd.segname, SEG_LINKEDIT))
                    segments_info->seg_linkedit_ptr.seg_linkedit_ptr_32
                        = (struct segment_command *)cmd;
                    break;
            }

            case LC_SYMTAB:
                segments_info->symtab_ptr = (struct symtab_command*)cmd;
                break;

            case LC_DYSYMTAB:
                segments_info->dysymtab_ptr = (struct dysymtab_command*)cmd;
                break;

            case LC_REEXPORT_DYLIB:
                break;

            default:
                break;
        }
    }

#if DEBUG
    printf("mach-o info:\
            \n\t\tlibrary loaded[vm] = 0x%x;\
            \n\t\tmach-o32std::mapped[vm] = 0x%x;\
            \n\t\tvm_slide = 0x%x;\
            \n\t\tsymtab = 0x%x;\
            \n\t\tdysymtab = 0x%x;\
            \n\t\tseg_linkedit = 0x%x;\n",
            handler->library_address,
            handler->mach_address,
            handler->vm_slide,
            segments_info->symtab_ptr,
            segments_info->dysymtab_ptr,
            segments_info->seg_linkedit_ptr);
#endif

    if (segments_info->symtab_ptr== nullptr 
        || segments_info->seg_linkedit_ptr.seg_linkedit_ptr_32 == nullptr)
        return false;

    return true;
}

static bool get_symbol_tables(mach_port_t target_task,
    struct mach_o_handler *handler)
{
    bool ret = false;
    struct segments_handler segments_info = {{0}, {0}, 0, 0};

    if (handler->is_32bit)
        ret = load_segments_32(target_task, handler, &segments_info);
    else
        ret = load_segments_64(target_task, handler, &segments_info);

    if (ret == false)
        return false;

    kern_return_t kret;
    mach_msg_type_number_t size = sizeof(struct symtab_command);
    struct symtab_command dup_symtab;
    kret = dump_data_from_target(target_task, &dup_symtab,
        (mach_vm_address_t)(segments_info.symtab_ptr), &size);
    if (kret != KERN_SUCCESS)
        return false;

    if (handler->should_swap)
        swap_symtab_command(&dup_symtab, 0);

    char * linkEditBase = nullptr;
    if (handler->is_32bit) {
        size = sizeof(struct segment_command);
        struct segment_command dup_linkedit;
        kret = dump_data_from_target(target_task, &dup_linkedit,
            (mach_vm_address_t)(segments_info.seg_linkedit_ptr\
            .seg_linkedit_ptr_32), &size);
        if (handler->should_swap)
            swap_segment_command(&dup_linkedit, 0);
        linkEditBase = (char *)(dup_linkedit.vmaddr - dup_linkedit.fileoff
                    + handler->vm_slide);
    } else {
        size = sizeof(struct segment_command_64);
        struct segment_command_64 dup_linkedit;
        kret =  dump_data_from_target(target_task, &dup_linkedit,
            (mach_vm_address_t)(segments_info.seg_linkedit_ptr\
            .seg_linkedit_ptr_64), &size);
        if (handler->should_swap)
            swap_segment_command_64(&dup_linkedit, 0);
        linkEditBase = (char *)(dup_linkedit.vmaddr - dup_linkedit.fileoff
                    + handler->vm_slide);
    }

    if (!linkEditBase)
        return false;

#if DEBUG
    printf("\n\nlc_symtab(%u) %u\n", LC_SYMTAB, dup_symtab.cmd);
    printf("\t\tcmd_size = %u\n", dup_symtab.cmdsize);
    printf("\t\tsymoff = %p\n", dup_symtab.symoff);
    printf("\t\tstroff = %p\n", dup_symtab.stroff);
    printf("\t\tstrsize = %u\n", dup_symtab.strsize);
#endif
    
    if (handler->is_32bit) {
        handler->symbol_table.symbol_table_32  = 
            (const struct nlist *)((uintptr_t)(linkEditBase) + dup_symtab.symoff);
        handler->string_table = (char *)((unsigned long)(linkEditBase)\
            + dup_symtab.stroff);
    } else {
        handler->symbol_table.symbol_table_64 =
            (struct nlist_64 *)((uintptr_t)(linkEditBase) + dup_symtab.symoff);
        handler->string_table = (char *)((unsigned long)(linkEditBase)\
            + dup_symtab.stroff);
    }
    handler->nsyms = dup_symtab.nsyms;
    handler->strsize = dup_symtab.strsize;

#if DEBUG
    printf("\n\nsymble table info:\n");
    if (handler->is_32bit)
        printf("\t\tsymbol_table32 = %p\n", handler->symbol_table.symbol_table_32);
    else
        printf("\t\tsymbol_table64 = %p\n", handler->symbol_table.symbol_table_64);
    printf("\t\tstring_table = %p\n", handler->string_table);
    printf("\t\tnumbers of symbols = %x\n", handler->nsyms);
    printf("\t\tsize of sym std::string = %x\n", handler->strsize);
#endif
    return true;
}

#define FUNC        0
#define LOAD_ADDR    1
static bool prepare_symbolication(mach_port_t target_task,
    struct mach_o_handler *handler_ptr,
    void * addr, uint32_t type)
{
    bool ret;
    // Not implemented
    if (type == FUNC)
        return false;

    if (type == LOAD_ADDR) {
        handler_ptr->library_address = addr;
        handler_ptr->mach_address = get_mach(target_task, addr,
            &(handler_ptr->should_swap), handler_ptr->is_32bit);

        if (!handler_ptr->mach_address)
            return false;
    }
    ret = get_symbol_tables(target_task, handler_ptr);
    return ret;
}

static int get_local_syms_in_vm32(mach_port_t target_task,
    struct mach_o_handler *handler_ptr)
{
    struct nlist *symbase =
        (struct nlist *)handler_ptr->symbol_table.symbol_table_32;
    struct nlist *sym = symbase;
    kern_return_t ret;

    mach_msg_type_number_t str_size = handler_ptr->strsize;
    handler_ptr->strings = (char *)malloc(str_size + 1);
    if (!handler_ptr->strings)
        return -ENOMEM;
    
    ret = dump_data_from_target(target_task, handler_ptr->strings,
        (mach_vm_address_t)(handler_ptr->string_table), &str_size);
    if (ret != KERN_SUCCESS) {
        free(handler_ptr->strings);
        handler_ptr->strings = nullptr;
        return -EINVAL;
    }

    handler_ptr->symbol_arrays = (struct vm_sym *)malloc(handler_ptr->nsyms
        * sizeof(struct vm_sym));
    if (!handler_ptr->symbol_arrays) {
        free(handler_ptr->strings);
        handler_ptr->strings = nullptr;
        return -EINVAL;
    }

    struct nlist dup_sym;
    uint64_t cur_vm_offset = 0;
    uint64_t cur_str_index = 0;
    mach_msg_type_number_t size;

    for (int64_t index = 0; index < handler_ptr->nsyms; index++, sym++) {
        size = sizeof(struct nlist);
        ret = dump_data_from_target(target_task, &dup_sym,
            (mach_vm_address_t)sym, &size);
        if (handler_ptr->should_swap)
            swap_nlist(&dup_sym, 1, 0);
        
        cur_vm_offset = handler_ptr->vm_slide + dup_sym.n_value
            - (uint64_t)handler_ptr->mach_address;
        cur_str_index = dup_sym.n_un.n_strx;
        handler_ptr->symbol_arrays[index].vm_offset = cur_vm_offset;
        handler_ptr->symbol_arrays[index].str_index = cur_str_index;
#if DEBUG
        printf("sym: %llx\t%s\n", cur_vm_offset, handler_ptr->strings + cur_str_index);
#endif
        if (cur_vm_offset == handler_ptr->vm_slide
            - (uint64_t)handler_ptr->mach_address)
            break;
    }

    return 0;
}

static int get_local_syms_in_vm64(mach_port_t target_task,
    struct mach_o_handler *handler_ptr)
{
    struct nlist_64 *symbase =
        (struct nlist_64 *)handler_ptr->symbol_table.symbol_table_64;
    struct nlist_64 *sym = symbase;
    kern_return_t ret;

    mach_msg_type_number_t str_size = handler_ptr->strsize;
    handler_ptr->strings = (char *)malloc(str_size + 1);
    if (!handler_ptr->strings)
        return -ENOMEM;
    
    ret = dump_data_from_target(target_task, handler_ptr->strings,
        (mach_vm_address_t)(handler_ptr->string_table), &str_size);
    if (ret != KERN_SUCCESS) {
        free(handler_ptr->strings);
        handler_ptr->strings = nullptr;
        return -EINVAL;
    }

    handler_ptr->symbol_arrays = (struct vm_sym *)malloc(handler_ptr->nsyms
        * sizeof(struct vm_sym));
    if (!handler_ptr->symbol_arrays) {
        free(handler_ptr->strings);
        handler_ptr->strings = nullptr;
        return -EINVAL;
    }

    struct nlist_64 dup_sym;
    uint64_t cur_vm_offset = 0;
    uint64_t cur_str_index = 0;
    mach_msg_type_number_t size;

    for (int64_t index = 0; index < handler_ptr->nsyms; index++, sym++) {
        size = sizeof(struct nlist_64);
        ret = dump_data_from_target(target_task, &dup_sym,
            (mach_vm_address_t)sym, &size);
        if (handler_ptr->should_swap)
            swap_nlist_64(&dup_sym, 1, 0);
        
        cur_vm_offset = handler_ptr->vm_slide + dup_sym.n_value
            - (uint64_t)handler_ptr->mach_address;
        cur_str_index = dup_sym.n_un.n_strx;
        handler_ptr->symbol_arrays[index].vm_offset = cur_vm_offset;
        handler_ptr->symbol_arrays[index].str_index = cur_str_index;
#if DEBUG
        printf("sym: %llx\t%s\n", cur_vm_offset, handler_ptr->strings + cur_str_index);
#endif
        
        if (cur_vm_offset == handler_ptr->vm_slide
            - (uint64_t)handler_ptr->mach_address)
            break;
    }

    return 0;
}

///////////////////////////////////////////
// get lib load address in current system
//////////////////////////////////////////
static uint32_t get_lib_load_addr_32(mach_port_t target_task,
    task_dyld_info_data_t info, const char * path)
{
    uint32_t load_addr_ret = 0;
    struct dyld_all_image_infos_32 all_image_info;
    mach_msg_type_number_t size = sizeof(struct dyld_all_image_infos_32);

    kern_return_t ret = dump_data_from_target(target_task,
        &all_image_info,
        (mach_vm_address_t)info.all_image_info_addr,
        &size);
    if (ret != KERN_SUCCESS)
        return 0;
#if DEBUG
    printf("Dump all lib info : %llx (size = %lu/%d)\n",
        (uint64_t)(info.all_image_info_addr),
        sizeof(struct dyld_all_image_infos_32));
#endif

    size = sizeof(struct dyld_image_info_32) * all_image_info.infoArrayCount;
    struct dyld_image_info_32 *array
        = (struct dyld_image_info_32 *)malloc(size);
    if (!array)
        return 0;

    ret = dump_data_from_target(target_task,
        array,
        (mach_vm_address_t)(all_image_info.infoArray),
        &size);

    if (ret != KERN_SUCCESS) {
        free((void *)array);
        return 0;
    }

#if DEBUG
    printf("Dump libs : %llx (size = %lu) /%lu\n",
        (uint64_t)(all_image_info.infoArray),
        size,
        sizeof(struct dyld_image_info_32) * all_image_info.infoArrayCount);
#endif

    struct dyld_image_info_32 *p = array;
    for (int i = 0; i < all_image_info.infoArrayCount; i++, p++) {
        char fpath_addr[MAX_PATH] = "";
        size = MAX_PATH;
        ret = dump_data_from_target(target_task, fpath_addr,
                (mach_vm_address_t)(p->imageFilePath), &size);

        if (ret == KERN_SUCCESS && strlen(fpath_addr) > 0) {
#if DEBUG
            printf("0x%x %s\n", (uint32_t)p->imageLoadAddress, fpath_addr);
            printf("target_path %s\n", path);
#endif
            if (!strcmp(path, fpath_addr))
                load_addr_ret = (uint32_t)p->imageLoadAddress;

            if (load_addr_ret != 0)
                break;
        }
    }

    free(array);
    return load_addr_ret;
}

static uint64_t get_lib_load_addr_64(mach_port_t target_task,
    task_dyld_info_data_t info, const char * path)
{
    uint64_t load_addr_ret = 0;
    struct dyld_all_image_infos all_image_info;
    mach_msg_type_number_t size = sizeof(struct dyld_all_image_infos);

    kern_return_t ret = dump_data_from_target(target_task,
        &all_image_info,
        (mach_vm_address_t)info.all_image_info_addr,
        &size);
    if (ret != KERN_SUCCESS)
        return 0;

#if DEBUG
    printf("Dump all lib info : %llx (size = %lu/%d)\n",
        (uint64_t)(info.all_image_info_addr),
        sizeof(struct dyld_all_image_infos));
#endif

    size = sizeof(struct dyld_image_info) * all_image_info.infoArrayCount;
    struct dyld_image_info *array = (struct dyld_image_info *)malloc(size);
    if (!array)
        return 0;

    ret = dump_data_from_target(target_task,
        (void *)array,
        (mach_vm_address_t)(all_image_info.infoArray),
        &size);

    if (ret != KERN_SUCCESS) {
        free((void *)array);
        return 0;
    }

#if DEBUG
    printf("Dump libs : %llx (size = %lu) /%lu\n",
        (uint64_t)(all_image_info.infoArray),
        size,
        sizeof(struct dyld_image_info) * all_image_info.infoArrayCount);
#endif

    struct dyld_image_info *p = array;
    for (int i = 0; i < all_image_info.infoArrayCount; i++, p++) {
        char fpath_addr[MAX_PATH] = "";
        size = MAX_PATH;
        ret = dump_data_from_target(target_task, fpath_addr,
                (mach_vm_address_t)(p->imageFilePath), &size);

        if (ret == KERN_SUCCESS && strlen(fpath_addr) > 0) {
#if DEBUG
            printf("%llx %s\n", (uint64_t)p->imageLoadAddress, fpath_addr);
            printf("target_path %s\n", path);
#endif
            if (!strcmp(path, fpath_addr))
                load_addr_ret = (uint64_t)p->imageLoadAddress;

            if (load_addr_ret != 0)
                break;
        }
    }
    free((void *)array);
    return load_addr_ret;
}

static uint64_t get_lib_load_addr(mach_port_t target_task,
    const char * path, bool * is_32bit)
{
    //get the list of dynamic libraries
    task_dyld_info_data_t info;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;

    kern_return_t kret = task_info(target_task, TASK_DYLD_INFO,
        (task_info_t)&info, &count);
    if (kret != KERN_SUCCESS) {
        printf("task_info() failed with message %s!\n",
            mach_error_string(kret));
        return 0;
    }

    uint64_t ret = 0;
    if (info.all_image_info_format == TASK_DYLD_ALL_IMAGE_INFO_32) {
        *is_32bit = true;
        ret = get_lib_load_addr_32(target_task, info, path);
    } else {
        *is_32bit = false;
        ret = get_lib_load_addr_64(target_task, info, path);
    }
    return ret;
}

bool get_syms_for_libpath(pid_t pid, const char *path,
    struct mach_o_handler *handler_ptr)
{
    mach_port_t target_task = MACH_PORT_NULL;
    kern_return_t kret = task_for_pid(mach_task_self(), pid, &target_task);
    bool ret = false;

    if (kret != KERN_SUCCESS) {
        printf("task_for_pid(%d) for symbolicate %s failed with message %s!\n",
            pid, path, mach_error_string(kret));
        return false;
    }

    uint64_t addr = get_lib_load_addr(target_task, path,
        &(handler_ptr->is_32bit));

    if (addr != 0) {
        prepare_symbolication(target_task, handler_ptr,
            (void *)addr, LOAD_ADDR);
        if ((handler_ptr->is_32bit
            && !(get_local_syms_in_vm32(target_task, handler_ptr)))
            || (!(handler_ptr->is_32bit)
                && !(get_local_syms_in_vm64(target_task, handler_ptr))))
            ret = true;
    }
    mach_port_deallocate(mach_task_self(), target_task);
    return ret;
}

/*
const char *get_sym_for_addr(pid_t pid, uint64_t vmoffset, const char *path)
{
    struct mach_o_handler handler;
    uint64_t addr;
    mach_port_t target_task = MACH_PORT_NULL;
    kern_return_t kret = task_for_pid(mach_task_self(), pid, &target_task);
    char symbol[512] = "unknown";

    if (kret != KERN_SUCCESS) {
        printf("task_for_pid() failed with message %s!\n",
            mach_error_string(kret));
        return false;
    }

    addr = get_lib_load_addr(target_task, path);

    #if DEBUG
    printf ("path %s addr %p\n", path, addr);
    #endif

    if (addr != 0) {
        prepare_symbolication(target_task, &handler, (void *)addr, LOAD_ADDR);
    #if DEBUG
        printf("check local sym\n");
    #endif
        addr = get_local_sym_in_vm(target_task, &handler, vmoffset, symbol);
        mach_port_deallocate(mach_task_self(), target_task);
        printf("%s\n[%p]\t%s\n", path, vmoffset, symbol);
    } else {
        printf("not a lib %s\n", path);
    }

    #if DEBUG
    printf("vmaddr %p, %s\n", vmoffset, symbol);
    #endif
    return strdup(symbol);
}
*/
#endif
