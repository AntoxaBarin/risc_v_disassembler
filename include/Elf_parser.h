#pragma once

#include "Elf.h"
#include <string>
#include <vector>

// Standard way to extract symbol data
#define ELF32_ST_BIND(info)         ((info) >> 4)
#define ELF32_ST_TYPE(info)         ((info) & 0xf)
#define ELF32_ST_VISIBILITY(info)   ((info) & 0x3)

class Elf_parser {
public:
    Elf_parser(FILE *elf_file);
    ~Elf_parser();

    std::vector<Elf32_Sym> get_symtab();
    Elf32_Word  get_text_section_idx();
    std::vector<Elf32_Word> get_text();
    Elf32_Addr  get_text_start_addr();

    const char* get_symbol_bind(char byte);
    const char* get_symbol_type(char byte);
    const char* get_symbol_visibility(char byte);
    std::string get_symbol_index(Elf32_Half ndx);
    const char* get_symbol_name(Elf32_Word st_name);

private:
    FILE *elf_src_;
    Elf32_Ehdr elf_header_;
    std::vector<Elf32_Word> text_;
    std::vector<Elf32_Sym> symtab_;
    char *symbol_names_;
    size_t text_section_idx;
    Elf32_Addr text_start_addr;

    void read_text_section(Elf32_Shdr& text_section_hdr);
    void read_symtable_section(Elf32_Shdr& symtable_section_hdr);
    void read_strtab_section(Elf32_Shdr& strtab_section_hdr);

    bool check_magic_bytes();
    bool check_bit_depth();
    bool check_endianness();
    bool check_isa();
};