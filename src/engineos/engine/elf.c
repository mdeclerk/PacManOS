#include "engineos/engine/elf.h"

#include "engineos/engine/uapi.h"
#include "engineos/kernel/heap.h"
#include "engineos/kernel/log.h"
#include "stdlib/string.h"

/* ELF constants */

#define EI_NIDENT   16
#define EI_CLASS    4
#define EI_DATA     5

#define ELFCLASS32  1u
#define ELFDATA2LSB 1u
#define EV_CURRENT  1u

#define ET_REL 1u
#define EM_386 3u

#define SHN_UNDEF 0u
#define SHN_ABS   0xFFF1u

#define SHT_SYMTAB 2u
#define SHT_STRTAB 3u
#define SHT_NOBITS 8u
#define SHT_REL    9u

#define SHF_ALLOC 0x2u

#define R_386_32    1u
#define R_386_PC32  2u
#define R_386_PLT32 4u

#define ELF32_R_SYM(info)  ((uint32_t)(info) >> 8u)
#define ELF32_R_TYPE(info) ((uint32_t)(info) & 0xFFu)

#define ELF_NAME_MAX 128u

/* ELF structures */

struct elf32_ehdr {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed));

struct elf32_shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} __attribute__((packed));

struct elf32_sym {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
} __attribute__((packed));

struct elf32_rel {
    uint32_t r_offset;
    uint32_t r_info;
} __attribute__((packed));

struct elf_layout {
    uintptr_t load_start;
    uintptr_t load_end;
    uintptr_t reserve;
    uint32_t  align;
};

struct elf_image_priv {
    struct elf_image         pub;
    const uint8_t           *bytes;
    size_t                   size;
    const struct elf32_ehdr *eh;
    const struct elf32_shdr *shdrs;
    const char              *shstr;
    size_t                   shstr_size;
    uintptr_t               *sec_addr;
    const struct elf32_sym  *syms;
    size_t                   sym_count;
    const char              *strtab;
    size_t                   strtab_size;
};

/* Helpers */

static bool check_range(size_t start, size_t len, size_t total)
{
    return start <= total && len <= (total - start);
}

static uintptr_t align_up(uintptr_t v, uint32_t a)
{
    uintptr_t mask = (uintptr_t)a - 1u;
    return (v + mask) & ~mask;
}

static const char *strtab_get(const char *tab, size_t tab_size, uint32_t off)
{
    if ((size_t)off >= tab_size)
        return nullptr;
    size_t max = tab_size - (size_t)off;
    return strnlen(tab + off, max) < max ? tab + off : nullptr;
}

/* Header validation & init */

static bool validate_header(const struct elf32_ehdr *eh, size_t size)
{
    if (size < sizeof(*eh))
        return false;
    if (memcmp(eh->e_ident, "\x7f""ELF", 4) != 0)
        return false;
    if (eh->e_ident[EI_CLASS] != ELFCLASS32 ||
        eh->e_ident[EI_DATA]  != ELFDATA2LSB)
        return false;
    if (eh->e_type != ET_REL || eh->e_machine != EM_386 ||
        eh->e_version != EV_CURRENT || eh->e_ehsize != sizeof(*eh))
        return false;
    if (eh->e_shentsize != sizeof(struct elf32_shdr) || eh->e_shnum == 0u)
        return false;
    return check_range(eh->e_shoff,
                       (size_t)eh->e_shnum * sizeof(struct elf32_shdr), size);
}

static struct elf_image_priv *alloc_init(const void *data, size_t size)
{
    if (!data)
        PANIC("ELF data is nullptr");

    struct elf_image_priv *priv = (struct elf_image_priv *)heap_alloc(sizeof(*priv));
    memset(priv, 0, sizeof(*priv));

    priv->bytes = (const uint8_t *)data;
    priv->size  = size;
    priv->eh    = (const struct elf32_ehdr *)data;

    if (!validate_header(priv->eh, size))
        PANIC("invalid ELF header");

    priv->shdrs = (const struct elf32_shdr *)(priv->bytes + priv->eh->e_shoff);

    uint16_t si = priv->eh->e_shstrndx;
    if (si >= priv->eh->e_shnum)
        PANIC("invalid ELF shstrndx");

    const struct elf32_shdr *ss = &priv->shdrs[si];
    if (ss->sh_type != SHT_STRTAB ||
        !check_range(ss->sh_offset, ss->sh_size, size))
        PANIC("invalid ELF shstrtab");

    priv->shstr      = (const char *)(priv->bytes + ss->sh_offset);
    priv->shstr_size = ss->sh_size;

    size_t n = (size_t)priv->eh->e_shnum * sizeof(uintptr_t);
    priv->sec_addr = (uintptr_t *)heap_alloc(n);
    memset(priv->sec_addr, 0, n);

    return priv;
}

/* Section layout & copy */

static bool layout_alloc_sections(struct elf_image_priv *priv,
                                  uintptr_t base, uintptr_t limit,
                                  struct elf_layout *out)
{
    uintptr_t cursor = base, lo = (uintptr_t)-1, hi = 0u;
    uint32_t max_align = 1u;

    for (uint16_t i = 0u; i < priv->eh->e_shnum; i++) {
        const struct elf32_shdr *sh = &priv->shdrs[i];
        if (!(sh->sh_flags & SHF_ALLOC))
            continue;

        uint32_t a = sh->sh_addralign ? sh->sh_addralign : 1u;
        if ((a & (a - 1u)) != 0u)
            return false;
        if (a > max_align)
            max_align = a;

        cursor = align_up(cursor, a);
        uintptr_t end = cursor + (uintptr_t)sh->sh_size;
        if (end < cursor || end > limit)
            return false;

        priv->sec_addr[i] = cursor;
        if (sh->sh_size != 0u) {
            if (cursor < lo) lo = cursor;
            if (end    > hi) hi = end;
        }
        cursor = end;
    }

    if (hi == 0u) /* no ALLOC section with non-zero size */
        return false;

    *out = (struct elf_layout){ lo, hi, cursor - base, max_align };
    return true;
}

static bool copy_alloc_sections(const struct elf_image_priv *priv)
{
    for (uint16_t i = 0u; i < priv->eh->e_shnum; i++) {
        const struct elf32_shdr *sh = &priv->shdrs[i];
        if (!(sh->sh_flags & SHF_ALLOC) || sh->sh_size == 0u)
            continue;

        void *dst = (void *)priv->sec_addr[i];
        if (sh->sh_type == SHT_NOBITS) {
            memset(dst, 0, sh->sh_size);
        } else {
            if (!check_range(sh->sh_offset, sh->sh_size, priv->size))
                return false;
            memcpy(dst, priv->bytes + sh->sh_offset, sh->sh_size);
        }
    }
    return true;
}

/* Symbol table */

static bool find_symtab(struct elf_image_priv *priv)
{
    const struct elf32_shdr *sym_sh = nullptr;

    for (uint16_t i = 0u; i < priv->eh->e_shnum; i++) {
        if (priv->shdrs[i].sh_type == SHT_SYMTAB) {
            if (sym_sh)
                return false; /* multiple SYMTAB sections */
            sym_sh = &priv->shdrs[i];
        }
    }

    if (!sym_sh ||
        sym_sh->sh_entsize != sizeof(struct elf32_sym) ||
        sym_sh->sh_size % sizeof(struct elf32_sym) != 0u ||
        !check_range(sym_sh->sh_offset, sym_sh->sh_size, priv->size))
        return false;

    if (sym_sh->sh_link >= priv->eh->e_shnum)
        return false;

    const struct elf32_shdr *str_sh = &priv->shdrs[sym_sh->sh_link];
    if (str_sh->sh_type != SHT_STRTAB ||
        !check_range(str_sh->sh_offset, str_sh->sh_size, priv->size))
        return false;

    priv->syms        = (const struct elf32_sym *)(priv->bytes + sym_sh->sh_offset);
    priv->sym_count   = sym_sh->sh_size / sizeof(struct elf32_sym);
    priv->strtab      = (const char *)(priv->bytes + str_sh->sh_offset);
    priv->strtab_size = str_sh->sh_size;
    return true;
}

static bool resolve_symbol_value(const struct elf_image_priv *priv,
                                 uint32_t idx, uintptr_t *out)
{
    if ((size_t)idx >= priv->sym_count)
        return false;

    const struct elf32_sym *sym = &priv->syms[idx];

    if (sym->st_shndx == SHN_UNDEF) {
        const char *name = strtab_get(priv->strtab, priv->strtab_size,
                                      sym->st_name);
        *out = name ? uapi_resolve_symbol(name) : 0u;
        return *out != 0u;
    }

    if (sym->st_shndx == SHN_ABS) {
        *out = (uintptr_t)sym->st_value;
        return true;
    }

    if (sym->st_shndx >= priv->eh->e_shnum)
        return false;

    const struct elf32_shdr *sh = &priv->shdrs[sym->st_shndx];
    if (!(sh->sh_flags & SHF_ALLOC) || sym->st_value > sh->sh_size)
        return false;

    *out = priv->sec_addr[sym->st_shndx] + sym->st_value;
    return true;
}

/* Relocations */

static bool apply_one_rel(const struct elf_image_priv *priv,
                          const struct elf32_rel *rel,
                          const struct elf32_shdr *target,
                          uintptr_t target_base)
{
    if (!check_range(rel->r_offset, sizeof(uint32_t), target->sh_size))
        return false;

    uint32_t *place = (uint32_t *)(target_base + rel->r_offset);
    uintptr_t S;
    if (!resolve_symbol_value(priv, ELF32_R_SYM(rel->r_info), &S))
        return false;

    uint32_t A = *place, P = (uint32_t)(uintptr_t)place;
    switch (ELF32_R_TYPE(rel->r_info)) {
    case R_386_32:    *place = (uint32_t)S + A;     break;
    case R_386_PC32:
    case R_386_PLT32: *place = (uint32_t)S + A - P; break;
    default:          return false;
    }
    return true;
}

static bool apply_relocations(const struct elf_image_priv *priv)
{
    for (uint16_t i = 0u; i < priv->eh->e_shnum; i++) {
        const struct elf32_shdr *rs = &priv->shdrs[i];
        if (rs->sh_type != SHT_REL || rs->sh_size == 0u)
            continue;
        if (rs->sh_entsize != sizeof(struct elf32_rel) ||
            rs->sh_size % sizeof(struct elf32_rel) != 0u ||
            !check_range(rs->sh_offset, rs->sh_size, priv->size))
            return false;
        if (rs->sh_info >= priv->eh->e_shnum)
            return false;

        const struct elf32_shdr *ts = &priv->shdrs[rs->sh_info];
        if (!(ts->sh_flags & SHF_ALLOC))
            continue;

        uintptr_t base = priv->sec_addr[rs->sh_info];
        const struct elf32_rel *rels =
            (const struct elf32_rel *)(priv->bytes + rs->sh_offset);
        size_t count = rs->sh_size / sizeof(struct elf32_rel);

        for (size_t j = 0u; j < count; j++) {
            if (!apply_one_rel(priv, &rels[j], ts, base))
                return false;
        }
    }
    return true;
}

/* Public interface */

struct elf_image *elf_load(const void *data, size_t size)
{
    struct elf_image_priv *priv = alloc_init(data, size);

    struct elf_layout lay;
    if (!layout_alloc_sections(priv, 0u, (uintptr_t)-1, &lay))
        PANIC("failed to layout ELF alloc sections");

    if (lay.reserve > (uintptr_t)-1 - ((uintptr_t)lay.align - 1u))
        PANIC("ELF image size overflow");

    void     *raw  = heap_alloc((size_t)(lay.reserve + (lay.align - 1u)));
    uintptr_t base = align_up((uintptr_t)raw, lay.align);

    for (uint16_t i = 0u; i < priv->eh->e_shnum; i++)
        priv->sec_addr[i] += base;

    if (!copy_alloc_sections(priv))
        PANIC("failed to copy ELF alloc sections");
    if (!find_symtab(priv))
        PANIC("failed to locate ELF symbol table");
    if (!apply_relocations(priv))
        PANIC("failed to apply ELF relocations");

    priv->pub.load_start = lay.load_start + base;
    priv->pub.load_end   = lay.load_end + base;
    return &priv->pub;
}

void elf_free(struct elf_image *img)
{
    struct elf_image_priv *priv = (struct elf_image_priv *)img;
    heap_free(priv->sec_addr);
    heap_free(priv);
}

uintptr_t elf_symbol(const struct elf_image *img, const char *name)
{
    const struct elf_image_priv *priv = (const struct elf_image_priv *)img;
    if (!priv || !name)
        return 0u;

    for (size_t i = 0u; i < priv->sym_count; i++) {
        const char *sn = strtab_get(priv->strtab, priv->strtab_size,
                                    priv->syms[i].st_name);
        if (!sn || !strneq(sn, name, ELF_NAME_MAX))
            continue;
        uintptr_t addr;
        if (resolve_symbol_value(priv, (uint32_t)i, &addr))
            return addr;
    }
    return 0u;
}
