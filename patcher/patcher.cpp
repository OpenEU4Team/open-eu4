#include <elf.h>
#include <link.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/uio.h>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <utility>

extern void replacement_0() asm("_ZNK11CAIStrategy35CalcDesiredAmountOfShipsFromCurrentERiS0_S0_S0_");
extern void replacement_1() asm("_ZN11CAIStrategy35CalcIsDefendedHomeStraitUnderAttackEv");
extern void replacement_2() asm("_ZN13CAISharedEval20CalcAcceptableThreatEPK9CProvincePK5CArmyfffiibbb11CCountryTag");

namespace eu4_override
{
constexpr size_t kStubSize = 12;
constexpr size_t kMaxEntries = 3;
constexpr size_t kMaxPhdrs = 32;
constexpr size_t kMaxMaps = 2048;
constexpr size_t kMaxSections = 64;

enum InstallResult : int
{
    kOk = 0,
    kErrFile = 1,
    kErrImage = 2,
    kErrElf = 3,
    kErrMaps = 4,
    kErrAddress = 5,
    kErrReplacement = 6,
    kErrBytes = 7,
    kErrPartial = 8,
    kErrThread = 9,
    kErrProtect = 10,
    kErrWrite = 11,
    kExitUnsafe = 127,
};

inline void EncodeStub(uintptr_t target, unsigned char out[kStubSize])
{
    out[0] = 0x48;
    out[1] = 0xb8;
    for (size_t i = 0; i < 8; ++i)
        out[2 + i] = static_cast<unsigned char>(target >> (8 * i));
    out[10] = 0xff;
    out[11] = 0xe0;
}

inline bool EqualBytes(const unsigned char *a, const unsigned char *b, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        if (a[i] != b[i])
            return false;
    return true;
}

struct MapRecord
{
    uintptr_t start;
    uintptr_t end;
    unsigned long offset;
    unsigned long dev_major;
    unsigned long dev_minor;
    unsigned long inode;
    int prot;
    bool is_private;
};

struct LinuxDirent64
{
    unsigned long long d_ino;
    long long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

struct OverrideEntry
{
    const char *name;
    uintptr_t address;
    unsigned char expected[kStubSize];
    void (*replacement)();
};

const char *const kOverrideNames[kMaxEntries] = {
    "_ZNK11CAIStrategy35CalcDesiredAmountOfShipsFromCurrentERiS0_S0_S0_",
    "_ZN11CAIStrategy35CalcIsDefendedHomeStraitUnderAttackEv",
    "_ZN13CAISharedEval20CalcAcceptableThreatEPK9CProvincePK5CArmyfffiibbb11CCountryTag",
};
void (*const kReplacements[kMaxEntries])() = {&replacement_0, &replacement_1, &replacement_2};

OverrideEntry gEntries[kMaxEntries];
bool gEntriesReady = false;
constexpr size_t kEntryCount = 3;
static_assert(kEntryCount == kMaxEntries, "EOL override table must have exactly three entries");
}

using namespace eu4_override;

namespace
{
constexpr size_t kIoBuffer = 64 * 1024;
constexpr size_t kMapsBuffer = 1024 * 1024;

struct TargetFile
{
    int fd;
    struct stat st;
    Elf64_Phdr phdrs[kMaxPhdrs];
    size_t phnum;
};

struct Window
{
    uintptr_t start;
    uintptr_t end;
    int prot;
};

struct DsoCheck
{
    const void *anchor;
    const OverrideEntry *entries;
    size_t count;
    bool anchor_found;
    bool all_ok;
};

int Fail(const char *label, unsigned long detail, int code)
{
    char hex[4 + 2 * sizeof(unsigned long)];
    hex[0] = ' ';
    hex[1] = '0';
    hex[2] = 'x';
    for (size_t i = 0; i < 2 * sizeof(unsigned long); ++i)
    {
        const unsigned d = (detail >> (4 * (2 * sizeof(unsigned long) - 1 - i))) & 0xf;
        hex[3 + i] = char(d < 10 ? '0' + d : 'a' + d - 10);
    }
    hex[sizeof(hex) - 1] = '\n';
    const struct iovec v[] = {
        {const_cast<char *>("eu4_override: "), 14},
        {const_cast<char *>(label), strlen(label)},
        {hex, sizeof(hex)},
    };
    (void)writev(2, v, 3);
    return code;
}

void Log(const char *msg)
{
    const struct iovec v = {const_cast<char *>(msg), strlen(msg)};
    (void)writev(2, &v, 1);
}

int CloseFail(int fd, const char *label, unsigned long detail, int code)
{
    Fail(label, detail, code);
    close(fd);
    return code;
}

int CloseOnly(int fd, int code)
{
    close(fd);
    return code;
}

bool LoadFailed(TargetFile *tf, const char *label, unsigned long detail, int code)
{
    Fail(label, detail, code);
    if (tf->fd >= 0)
        close(tf->fd);
    tf->fd = -1;
    return false;
}

int HexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

bool ParseHex(const char *&p, unsigned long &v)
{
    v = 0;
    bool any = false;
    for (int d; (d = HexValue(*p)) >= 0; ++p)
    {
        v = v * 16 + unsigned(d);
        any = true;
    }
    return any;
}

bool ParseDec(const char *&p, unsigned long &v)
{
    v = 0;
    bool any = false;
    while (*p >= '0' && *p <= '9')
    {
        v = v * 10 + unsigned(*p - '0');
        ++p;
        any = true;
    }
    return any;
}

void SkipSpaces(const char *&p)
{
    while (*p == ' ')
        ++p;
}

uintptr_t PageDown(uintptr_t v, size_t p) { return v & ~uintptr_t(p - 1); }
uintptr_t PageUp(uintptr_t v, size_t p) { return (v + p - 1) & ~uintptr_t(p - 1); }
bool ValidPageSize(size_t p) { return p >= 4096 && (p & (p - 1)) == 0; }

bool LoadTarget(const char *path, TargetFile *tf)
{
    tf->fd = -1;
    const int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return LoadFailed(tf, "open target", unsigned(errno), kErrFile);
    tf->fd = fd;
    if (fstat(fd, &tf->st) || !S_ISREG(tf->st.st_mode) || tf->st.st_size <= 0)
        return LoadFailed(tf, "stat target", unsigned(errno), kErrFile);

    Elf64_Ehdr eh;
    if (pread(fd, &eh, sizeof(eh), 0) != ssize_t(sizeof(eh)))
        return LoadFailed(tf, "read ELF header", unsigned(errno), kErrElf);
    const bool eh_ok = eh.e_ident[EI_MAG0] == ELFMAG0 && eh.e_ident[EI_MAG1] == ELFMAG1 &&
                       eh.e_ident[EI_MAG2] == ELFMAG2 && eh.e_ident[EI_MAG3] == ELFMAG3 &&
                       eh.e_ident[EI_CLASS] == ELFCLASS64 && eh.e_ident[EI_DATA] == ELFDATA2LSB &&
                       eh.e_type == ET_EXEC && eh.e_machine == EM_X86_64 &&
                       eh.e_phentsize == sizeof(Elf64_Phdr) && eh.e_phnum > 0 && eh.e_phnum <= kMaxPhdrs;
    if (!eh_ok)
        return LoadFailed(tf, "target not ET_EXEC x86-64", eh.e_type, kErrElf);
    tf->phnum = eh.e_phnum;
    if (pread(fd, tf->phdrs, sizeof(Elf64_Phdr) * tf->phnum, eh.e_phoff) !=
        ssize_t(sizeof(Elf64_Phdr) * tf->phnum))
        return LoadFailed(tf, "read program headers", unsigned(errno), kErrElf);
    const size_t page = size_t(sysconf(_SC_PAGESIZE));
    if (!ValidPageSize(page))
        return LoadFailed(tf, "page size", page, kErrElf);
    for (size_t i = 0; i < tf->phnum; ++i)
    {
        const Elf64_Phdr &p = tf->phdrs[i];
        if (p.p_type != PT_LOAD || !p.p_memsz)
            continue;
        if (p.p_filesz > p.p_memsz || p.p_offset > uint64_t(tf->st.st_size) ||
            p.p_filesz > uint64_t(tf->st.st_size) - p.p_offset || p.p_vaddr < page ||
            p.p_vaddr % page != p.p_offset % page)
            return LoadFailed(tf, "invalid PT_LOAD", unsigned(i), kErrElf);
        const uintptr_t lo = PageDown(uintptr_t(p.p_vaddr), page);
        const uintptr_t hi = PageUp(uintptr_t(p.p_vaddr + p.p_memsz), page);
        for (size_t j = 0; j < i; ++j)
        {
            const Elf64_Phdr &q = tf->phdrs[j];
            if (q.p_type != PT_LOAD || !q.p_memsz)
                continue;
            const uintptr_t qlo = PageDown(uintptr_t(q.p_vaddr), page);
            const uintptr_t qhi = PageUp(uintptr_t(q.p_vaddr + q.p_memsz), page);
            if (lo < qhi && qlo < hi)
                return LoadFailed(tf, "overlapping PT_LOAD", unsigned(i), kErrElf);
        }
    }
    return true;
}

bool ReadMaps(MapRecord *maps, size_t *count)
{
    const int fd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        Fail("open maps", unsigned(errno), kErrMaps);
        return false;
    }
    static unsigned char buf[kMapsBuffer];
    size_t used = 0;
    for (;;)
    {
        if (used >= sizeof(buf) - 1)
        {
            Fail("maps too large", used, kErrMaps);
            close(fd);
            return false;
        }
        const ssize_t n = read(fd, buf + used, sizeof(buf) - used - 1);
        if (n > 0)
            used += size_t(n);
        else if (n == 0)
            break;
        else if (errno == EINTR)
            continue;
        else
        {
            Fail("read maps", unsigned(errno), kErrMaps);
            close(fd);
            return false;
        }
    }
    close(fd);
    buf[used] = '\0';

    *count = 0;
    char *line = reinterpret_cast<char *>(buf);
    while (*line && *count < kMaxMaps)
    {
        char *end = strchr(line, '\n');
        if (!end)
            break;
        const char *p = line;
        unsigned long a = 0, b = 0, off = 0, maj = 0, min = 0, ino = 0;
        bool ok = ParseHex(p, a) && *p++ == '-' && ParseHex(p, b);
        SkipSpaces(p);
        const bool r = p[0] == 'r', w = p[1] == 'w', x = p[2] == 'x';
        const bool private_map = p[3] == 'p';
        ok = ok && (r || p[0] == '-') && (w || p[1] == '-') && (x || p[2] == '-');
        p += 4;
        SkipSpaces(p);
        ok = ok && ParseHex(p, off);
        SkipSpaces(p);
        ok = ok && ParseHex(p, maj) && *p++ == ':' && ParseHex(p, min);
        SkipSpaces(p);
        ok = ok && ParseDec(p, ino);
        if (!ok)
        {
            const char saved_end = *end;
            *end = '\0';
            Log("eu4_override: unparsed maps line: ");
            Log(line);
            Log("\n");
            *end = saved_end;
            return false;
        }
        if (private_map && b > a && (b - a) % 4096 == 0)
            maps[(*count)++] = MapRecord{uintptr_t(a), uintptr_t(b), off, maj, min, ino,
                                         (r ? PROT_READ : 0) | (w ? PROT_WRITE : 0) | (x ? PROT_EXEC : 0), true};
        line = end + 1;
    }
    if (*line)
    {
        Fail("maps truncated", *count, kErrMaps);
        return false;
    }
    return true;
}

bool Covered(const MapRecord *maps, size_t count, uintptr_t start, uintptr_t end, int prot)
{
    uintptr_t cur = start;
    while (cur < end)
    {
        bool found = false;
        for (size_t i = 0; i < count; ++i)
        {
            const MapRecord &m = maps[i];
            if (m.start <= cur && cur < m.end && m.prot == prot && m.is_private)
            {
                cur = m.end < end ? m.end : end;
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

bool SameFileIdentity(const MapRecord &m, const TargetFile &tf)
{
    return m.dev_major == major(tf.st.st_dev) && m.dev_minor == minor(tf.st.st_dev) &&
           m.inode == static_cast<unsigned long>(tf.st.st_ino);
}

const Elf64_Phdr *ExecPtLoad(const TargetFile &tf, uintptr_t addr, size_t len)
{
    for (size_t j = 0; j < tf.phnum; ++j)
    {
        const Elf64_Phdr &p = tf.phdrs[j];
        if (p.p_type == PT_LOAD && (p.p_flags & PF_X) && uint64_t(addr) >= p.p_vaddr &&
            uint64_t(addr) + len <= p.p_vaddr + p.p_filesz)
            return &p;
    }
    return nullptr;
}

bool ResolveEntries(const TargetFile &tf, OverrideEntry *entries)
{
    const size_t size = size_t(tf.st.st_size);
    void *raw = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, tf.fd, 0);
    if (raw == MAP_FAILED)
    {
        Fail("mmap target", unsigned(errno), kErrElf);
        return false;
    }
    const unsigned char *image = static_cast<const unsigned char *>(raw);
    bool ok = false;
    do
    {
        Elf64_Ehdr eh;
        memcpy(&eh, image, sizeof(eh));
        if (eh.e_shentsize != sizeof(Elf64_Shdr) || eh.e_shnum == 0 || eh.e_shnum > kMaxSections ||
            eh.e_shoff < sizeof(Elf64_Ehdr) || eh.e_shoff > size ||
            size_t(eh.e_shnum) * sizeof(Elf64_Shdr) > size - eh.e_shoff)
        {
            Fail("section table", eh.e_shnum, kErrElf);
            break;
        }
        Elf64_Shdr shdrs[kMaxSections];
        for (size_t i = 0; i < eh.e_shnum; ++i)
            memcpy(&shdrs[i], image + eh.e_shoff + i * sizeof(Elf64_Shdr), sizeof(Elf64_Shdr));
        const Elf64_Shdr *sym = nullptr;
        size_t symtabs = 0;
        for (size_t i = 0; i < eh.e_shnum; ++i)
            if (shdrs[i].sh_type == SHT_SYMTAB)
            {
                ++symtabs;
                sym = &shdrs[i];
            }
        if (symtabs != 1)
        {
            Fail("symtab count", symtabs, kErrElf);
            break;
        }
        if (sym->sh_entsize != sizeof(Elf64_Sym) || sym->sh_size < sizeof(Elf64_Sym) ||
            sym->sh_size % sizeof(Elf64_Sym) || sym->sh_offset > uint64_t(size) ||
            sym->sh_size > uint64_t(size) - sym->sh_offset || sym->sh_link >= eh.e_shnum)
        {
            Fail("symtab bounds", symtabs, kErrElf);
            break;
        }
        const Elf64_Shdr &strs = shdrs[sym->sh_link];
        if (strs.sh_type != SHT_STRTAB || strs.sh_size == 0 || strs.sh_offset > uint64_t(size) ||
            strs.sh_size > uint64_t(size) - strs.sh_offset)
        {
            Fail("strtab bounds", sym->sh_link, kErrElf);
            break;
        }
        const char *const strbase = reinterpret_cast<const char *>(image + strs.sh_offset);
        const size_t strsize = size_t(strs.sh_size);
        const size_t nsyms = size_t(sym->sh_size / sizeof(Elf64_Sym));
        bool bad = false;
        for (size_t e = 0; e < kEntryCount && !bad; ++e)
        {
            const char *const want = kOverrideNames[e];
            const size_t want_len = strlen(want);
            size_t matches = 0;
            uintptr_t address = 0;
            for (size_t i = 0; i < nsyms; ++i)
            {
                Elf64_Sym s;
                memcpy(&s, image + sym->sh_offset + i * sizeof(Elf64_Sym), sizeof(s));
                if (ELF64_ST_TYPE(s.st_info) != STT_FUNC || s.st_shndx == SHN_UNDEF || !s.st_value ||
                    s.st_name >= strsize)
                    continue;
                const char *const name = strbase + s.st_name;
                const char *const end = static_cast<const char *>(memchr(name, 0, strsize - size_t(s.st_name)));
                if (!end || size_t(end - name) != want_len || memcmp(name, want, want_len))
                    continue;
                ++matches;
                address = uintptr_t(s.st_value);
                if (s.st_shndx >= eh.e_shnum || s.st_size < kStubSize)
                {
                    Fail("invalid override symbol extent", e, kErrElf);
                    bad = true;
                    break;
                }
            }
            if (bad)
                break;
            if (matches != 1)
            {
                Fail(matches ? "override symbol ambiguous" : "override symbol missing", e, kErrElf);
                bad = true;
                break;
            }
            const Elf64_Phdr *seg = ExecPtLoad(tf, address, kStubSize);
            if (!seg)
            {
                Fail("override outside executable PT_LOAD", e, kErrElf);
                bad = true;
                break;
            }
            const uint64_t off = uint64_t(seg->p_offset) + (uint64_t(address) - uint64_t(seg->p_vaddr));
            entries[e].name = want;
            entries[e].address = address;
            entries[e].replacement = kReplacements[e];
            memcpy(entries[e].expected, image + off, kStubSize);
        }
        ok = !bad;
    } while (false);
    munmap(raw, size);
    return ok;
}

bool CheckEntries(const TargetFile &tf, const MapRecord *maps, size_t count, size_t page, bool file_backed)
{
    for (size_t e = 0; e < kEntryCount; ++e)
    {
        const uintptr_t addr = uintptr_t(gEntries[e].address);
        const Elf64_Phdr *seg = ExecPtLoad(tf, addr, kStubSize);
        if (!seg)
        {
            Fail("entry outside executable PT_LOAD", unsigned(e), kErrAddress);
            return false;
        }
        if (!file_backed)
            continue;
        const uintptr_t lo = PageDown(addr, page);
        const uintptr_t hi = PageUp(addr + kStubSize, page);
        const MapRecord *owner = nullptr;
        for (size_t i = 0; i < count; ++i)
            if (SameFileIdentity(maps[i], tf) && maps[i].start <= addr && addr + kStubSize <= maps[i].end)
                owner = &maps[i];
        if (!owner || lo < owner->start || hi > owner->end ||
            !(owner->prot & PROT_EXEC) || (owner->prot & PROT_WRITE))
        {
            Fail("entry outside verified RX image", unsigned(e), kErrAddress);
            return false;
        }
        const uint64_t file_off = uint64_t(seg->p_offset) + (uint64_t(addr) - uint64_t(seg->p_vaddr));
        if (owner->offset + (uint64_t(addr) - uint64_t(owner->start)) != file_off)
        {
            Fail("entry file offset", unsigned(e), kErrMaps);
            return false;
        }
    }
    return true;
}

const Elf64_Phdr *LoadForMapping(const TargetFile &tf, const MapRecord &m, size_t page)
{
    for (size_t j = 0; j < tf.phnum; ++j)
    {
        const Elf64_Phdr &p = tf.phdrs[j];
        if (p.p_type != PT_LOAD || !p.p_filesz || m.offset < uint64_t(PageDown(uintptr_t(p.p_offset), page)) ||
            m.offset >= uint64_t(PageUp(uintptr_t(p.p_offset + p.p_filesz), page)))
            continue;
        const int64_t candidate = int64_t(p.p_vaddr) + (int64_t(m.offset) - int64_t(p.p_offset));
        if (candidate >= 0 && uint64_t(candidate) == uint64_t(m.start))
            return &p;
    }
    return nullptr;
}

int WantProt(const Elf64_Phdr &p)
{
    return PROT_READ | ((p.p_flags & PF_W) ? PROT_WRITE : 0) | ((p.p_flags & PF_X) ? PROT_EXEC : 0);
}

bool VerifyFileBacked(const TargetFile &tf, const MapRecord *maps, size_t count, size_t page)
{
    bool any = false;
    for (size_t i = 0; i < count; ++i)
    {
        const MapRecord &m = maps[i];
        if (!SameFileIdentity(m, tf))
            continue;
        any = true;
        const Elf64_Phdr *seg = LoadForMapping(tf, m, page);
        if (!seg)
        {
            Fail("mapping outside PT_LOAD", unsigned(i), kErrMaps);
            return false;
        }
        if (!m.is_private || bool(m.prot & PROT_EXEC) != bool(seg->p_flags & PF_X) ||
            ((m.prot & PROT_WRITE) && !(seg->p_flags & PF_W)))
        {
            Fail("mapping vaddr or flags", unsigned(i), kErrMaps);
            return false;
        }
    }
    if (!any)
    {
        Fail("no target file mappings", 0, kErrMaps);
        return false;
    }
    return CheckEntries(tf, maps, count, page, true);
}

bool VerifyImageLayout(const TargetFile &tf, const MapRecord *maps, size_t count, size_t page)
{
    for (size_t j = 0; j < tf.phnum; ++j)
    {
        const Elf64_Phdr &p = tf.phdrs[j];
        if (p.p_type != PT_LOAD || !p.p_memsz)
            continue;
        const uintptr_t vpage = uintptr_t(p.p_vaddr) & ~uintptr_t(page - 1);
        const size_t span = size_t(p.p_memsz) + size_t(uint64_t(p.p_vaddr) - vpage);
        if (!Covered(maps, count, vpage, vpage + span, WantProt(p)))
        {
            Fail("PT_LOAD range or protection", unsigned(j), kErrMaps);
            return false;
        }
    }
    return CheckEntries(tf, maps, count, page, false);
}

bool CompareFileMemory(const TargetFile &tf, uintptr_t addr, uint64_t file_off, size_t len,
                       unsigned char *buf)
{
    while (len)
    {
        const size_t chunk = len < kIoBuffer ? len : kIoBuffer;
        if (pread(tf.fd, buf, chunk, off64_t(file_off)) != ssize_t(chunk))
        {
            Fail("pread image", unsigned(errno), kErrFile);
            return false;
        }
        if (!EqualBytes(buf, reinterpret_cast<const unsigned char *>(addr), chunk))
            return false;
        addr += chunk;
        file_off += chunk;
        len -= chunk;
    }
    return true;
}

bool CompareExecutableImage(const TargetFile &tf, const bool *installed)
{
    unsigned char buf[kIoBuffer];
    Window wins[kMaxEntries];
    size_t nwins = 0;
    for (size_t e = 0; e < kEntryCount; ++e)
    {
        if (!installed[e])
            continue;
        const uintptr_t addr = uintptr_t(gEntries[e].address);
        for (size_t k = 0; k < nwins; ++k)
            if (addr < wins[k].end && wins[k].start < addr + kStubSize)
            {
                Fail("overlapping installed stubs", unsigned(e), kErrAddress);
                return false;
            }
        wins[nwins].start = addr;
        wins[nwins].end = addr + kStubSize;
        ++nwins;
    }
    for (size_t i = 0; i < nwins; ++i)
        for (size_t j = i + 1; j < nwins; ++j)
            if (wins[j].start < wins[i].start)
                std::swap(wins[i], wins[j]);
    for (size_t j = 0; j < tf.phnum; ++j)
    {
        const Elf64_Phdr &p = tf.phdrs[j];
        if (p.p_type != PT_LOAD || !(p.p_flags & PF_X) || !p.p_filesz)
            continue;
        uintptr_t cur = uintptr_t(p.p_vaddr);
        const uintptr_t end = uintptr_t(p.p_vaddr + p.p_filesz);
        for (size_t i = 0; i < nwins; ++i)
        {
            const Window &w = wins[i];
            if (w.start < uintptr_t(p.p_vaddr) || w.end > end)
                continue;
            if (w.start < cur || !CompareFileMemory(tf, cur, uint64_t(p.p_offset) + (uint64_t(cur) - uint64_t(p.p_vaddr)),
                                                      size_t(w.start - cur), buf))
            {
                Fail("image bytes differ outside stub", unsigned(i), kErrImage);
                return false;
            }
            cur = w.end;
        }
        if (cur < end && !CompareFileMemory(tf, cur, uint64_t(p.p_offset) + (uint64_t(cur) - uint64_t(p.p_vaddr)),
                                             size_t(end - cur), buf))
        {
            Fail("image bytes differ", unsigned(j), kErrImage);
            return false;
        }
    }
    return true;
}

bool InExecText(uintptr_t base, const Elf64_Phdr *phdrs, int n, uintptr_t addr)
{
    for (int i = 0; i < n; ++i)
    {
        const Elf64_Phdr &p = phdrs[i];
        if (p.p_type == PT_LOAD && (p.p_flags & PF_X) && addr >= base + uintptr_t(p.p_vaddr) &&
            addr < base + uintptr_t(p.p_vaddr + p.p_memsz))
            return true;
    }
    return false;
}

int DsoCallback(struct dl_phdr_info *info, size_t size, void *data)
{
    DsoCheck *check = static_cast<DsoCheck *>(data);
    if (size < sizeof(*info))
        return 0;
    const uintptr_t base = uintptr_t(info->dlpi_addr);
    if (!InExecText(base, info->dlpi_phdr, info->dlpi_phnum, uintptr_t(check->anchor)))
        return 0;
    check->anchor_found = true;
    for (size_t e = 0; e < check->count; ++e)
        if (uintptr_t(check->entries[e].replacement) &&
            !InExecText(base, info->dlpi_phdr, info->dlpi_phnum, uintptr_t(check->entries[e].replacement)))
        {
            check->all_ok = false;
            break;
        }
    return 1;
}

bool ReplacementPointersInThisDso()
{
    DsoCheck check;
    check.anchor = reinterpret_cast<const void *>(&DsoCallback);
    check.entries = gEntries;
    check.count = kEntryCount;
    check.anchor_found = false;
    check.all_ok = true;
    dl_iterate_phdr(&DsoCallback, &check);
    if (!check.anchor_found || !check.all_ok)
        Fail("replacement outside this DSO", 0, kErrReplacement);
    return check.anchor_found && check.all_ok;
}

bool SingleThreaded()
{
    const int fd = open("/proc/self/task", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
    {
        Fail("open task dir", unsigned(errno), kErrThread);
        return false;
    }
    LinuxDirent64 buf[32];
    size_t threads = 0;
    for (;;)
    {
        const long n = syscall(SYS_getdents64, fd, buf, sizeof(buf));
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            Fail("read task dir", unsigned(errno), kErrThread);
            close(fd);
            return false;
        }
        if (n == 0)
            break;
        for (long off = 0; off < n;)
        {
            const LinuxDirent64 *d = reinterpret_cast<const LinuxDirent64 *>(reinterpret_cast<const char *>(buf) + off);
            if (d->d_reclen <= offsetof(LinuxDirent64, d_name) || d->d_reclen > sizeof(LinuxDirent64))
            {
                Fail("bad task dirent", unsigned(d->d_reclen), kErrThread);
                close(fd);
                return false;
            }
            const char *name = d->d_name;
            if (!(name[0] == '.' && name[1] == '\0') && !(name[0] == '.' && name[1] == '.' && name[2] == '\0'))
                ++threads;
            off += d->d_reclen;
        }
    }
    close(fd);
    if (threads != 1)
    {
        Fail("threads present", threads, kErrThread);
        return false;
    }
    return true;
}

bool MakeWindows(Window *windows, size_t *count, size_t page, const MapRecord *maps, size_t nmaps)
{
    *count = 0;
    for (size_t e = 0; e < kEntryCount; ++e)
    {
        const uintptr_t addr = uintptr_t(gEntries[e].address);
        const uintptr_t lo = PageDown(addr, page);
        const uintptr_t hi = PageUp(addr + kStubSize, page);
        bool mapped = false;
        for (size_t i = 0; i < nmaps; ++i)
            if (maps[i].start <= lo && hi <= maps[i].end && (maps[i].prot & PROT_EXEC))
                mapped = true;
        if (!mapped)
        {
            Fail("patch pages not mapped RX", unsigned(e), kErrAddress);
            return false;
        }
        size_t slot = *count;
        for (size_t i = 0; i < *count; ++i)
            if (!(windows[i].end <= lo || hi <= windows[i].start))
            {
                slot = i;
                break;
            }
        if (slot == *count)
        {
            windows[slot].start = lo;
            windows[slot].end = hi;
            windows[slot].prot = PROT_READ | PROT_EXEC;
            ++*count;
        }
        else
        {
            if (lo < windows[slot].start)
                windows[slot].start = lo;
            if (hi > windows[slot].end)
                windows[slot].end = hi;
        }
    }
    return true;
}

bool RestoreProtections(Window *windows, size_t count)
{
    bool ok = true;
    for (size_t i = count; i > 0; --i)
    {
        const Window &w = windows[i - 1];
        if (mprotect(reinterpret_cast<void *>(w.start), w.end - w.start, w.prot))
            ok = false;
    }
    return ok;
}

void Recover(Window *windows, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        const Window &w = windows[i];
        if (mprotect(reinterpret_cast<void *>(w.start), w.end - w.start, PROT_READ | PROT_WRITE))
        {
            Log("eu4_override: cannot reacquire writable text\n");
            _exit(kExitUnsafe);
        }
    }
    for (size_t e = 0; e < kEntryCount; ++e)
        __builtin_memcpy(reinterpret_cast<void *>(gEntries[e].address), gEntries[e].expected, kStubSize);
    if (!RestoreProtections(windows, count))
    {
        Log("eu4_override: cannot leave safe state\n");
        _exit(kExitUnsafe);
    }
}

int Install(const char *path, bool require_file_backed)
{
    if (!path || !*path)
        return Fail("no mapped ELF path", 0, kErrFile);
    if (!SingleThreaded())
        return kErrThread;

    TargetFile tf;
    if (!LoadTarget(path, &tf))
        return kErrFile;
    const int target_fd = tf.fd;
    if (!ResolveEntries(tf, gEntries))
        return CloseFail(target_fd, "resolve override symbols", 0, kErrElf);
    if (!ReplacementPointersInThisDso())
        return CloseOnly(target_fd, kErrReplacement);
    MapRecord maps[kMaxMaps];
    size_t nmaps = 0;
    if (!ReadMaps(maps, &nmaps))
        return CloseOnly(target_fd, kErrMaps);
    const size_t page = size_t(sysconf(_SC_PAGESIZE));
    if (!ValidPageSize(page))
        return CloseFail(target_fd, "page size", page, kErrElf);

    bool file_backed = false;
    for (size_t i = 0; i < nmaps; ++i)
        if (SameFileIdentity(maps[i], tf))
            file_backed = true;
    if (!file_backed && require_file_backed)
        return CloseFail(target_fd, "no verified image", 0, kErrMaps);
    if (!(file_backed ? VerifyFileBacked(tf, maps, nmaps, page) : VerifyImageLayout(tf, maps, nmaps, page)))
        return CloseOnly(target_fd, kErrMaps);

    unsigned char stubs[kMaxEntries][kStubSize];
    bool installed[kMaxEntries];
    size_t installed_count = 0;
    for (size_t e = 0; e < kEntryCount; ++e)
    {
        const OverrideEntry &entry = gEntries[e];
        const unsigned char *cur = reinterpret_cast<const unsigned char *>(entry.address);
        EncodeStub(uintptr_t(entry.replacement), stubs[e]);
        if (EqualBytes(cur, entry.expected, kStubSize))
            installed[e] = false;
        else if (EqualBytes(cur, stubs[e], kStubSize))
        {
            installed[e] = true;
            ++installed_count;
        }
        else
            return CloseFail(target_fd, "entry bytes neither original nor installed", e, kErrBytes);
    }
    if (!file_backed && !CompareExecutableImage(tf, installed))
        return CloseOnly(target_fd, kErrImage);
    if (installed_count && installed_count != kEntryCount)
        return CloseFail(target_fd, "partial install refused", installed_count, kErrPartial);
    if (installed_count == kEntryCount)
    {
        gEntriesReady = true;
        Log("eu4_override: already installed\n");
        return CloseOnly(target_fd, kOk);
    }

    Window windows[kMaxEntries];
    size_t nwindows = 0;
    if (!MakeWindows(windows, &nwindows, page, maps, nmaps))
        return CloseOnly(target_fd, kErrAddress);
    for (size_t i = 0; i < nwindows; ++i)
        if (mprotect(reinterpret_cast<void *>(windows[i].start), windows[i].end - windows[i].start,
                     PROT_READ | PROT_WRITE))
        {
            CloseFail(target_fd, "mprotect RW", unsigned(errno), kErrProtect);
            Recover(windows, nwindows);
            return kErrProtect;
        }

    for (size_t e = 0; e < kEntryCount; ++e)
        __builtin_memcpy(reinterpret_cast<void *>(gEntries[e].address), stubs[e], kStubSize);
    bool wrote_ok = true;
    for (size_t e = 0; e < kEntryCount; ++e)
        if (!EqualBytes(reinterpret_cast<const unsigned char *>(gEntries[e].address), stubs[e], kStubSize))
            wrote_ok = false;
    if (!wrote_ok)
    {
        CloseFail(target_fd, "stub write verification", 0, kErrWrite);
        Recover(windows, nwindows);
        return kErrWrite;
    }
    if (!RestoreProtections(windows, nwindows))
    {
        CloseFail(target_fd, "mprotect restore failed; rolling back", unsigned(errno), kErrProtect);
        Recover(windows, nwindows);
        return kErrProtect;
    }
    close(target_fd);
    gEntriesReady = true;
    Log("eu4_override: installed\n");
    return kOk;
}

__attribute__((constructor(200))) void AutoInstall()
{
    const char *path = getenv("EU4_OVERRIDE_APP");
    (void)Install(path && *path ? path : "/proc/self/exe", true);
}
}

extern "C" int eu4_override_install(const char *mapped_elf_path)
{
    return Install(mapped_elf_path, false);
}

extern "C" size_t eu4_override_status(uintptr_t *addresses, uintptr_t *replacements, size_t capacity)
{
    if (!gEntriesReady)
        return 0;
    MapRecord maps[kMaxMaps];
    size_t nmaps = 0;
    if (!ReadMaps(maps, &nmaps))
        return 0;
    unsigned char stub[kStubSize];
    size_t installed = 0;
    for (size_t e = 0; e < kEntryCount; ++e)
    {
        const uintptr_t addr = gEntries[e].address;
        bool mapped = false;
        for (size_t i = 0; i < nmaps && !mapped; ++i)
            if (maps[i].start <= addr && addr + kStubSize <= maps[i].end &&
                (maps[i].prot & (PROT_READ | PROT_EXEC)) == (PROT_READ | PROT_EXEC) && maps[i].is_private)
                mapped = true;
        if (!mapped)
            continue;
        EncodeStub(uintptr_t(gEntries[e].replacement), stub);
        if (EqualBytes(reinterpret_cast<const unsigned char *>(addr), stub, kStubSize))
            ++installed;
    }
    if (capacity >= kEntryCount && addresses && replacements)
        for (size_t e = 0; e < kEntryCount; ++e)
        {
            addresses[e] = uintptr_t(gEntries[e].address);
            replacements[e] = uintptr_t(gEntries[e].replacement);
        }
    return installed;
}
