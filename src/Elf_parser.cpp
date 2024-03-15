#include "Elf_parser.h"
#include <stdexcept>
#include <cstring>

#define EI_MAG0  0x7f  // Elf magic bytes
#define EI_MAG1  'E'
#define EI_MAG2  'L'
#define EI_MAG3  'F'
#define EI_CLASS 1     // 32bit Elf file
#define EI_DATA  1     // little endian
#define ISA      0xf3  // RISC-V architecture

Elf_parser::Elf_parser(FILE *elf_file) {
    elf_src_ = elf_file;
    fread(&elf_header_, sizeof(Elf32_Ehdr), 1, elf_file);

    if (!check_magic_bytes()) {
        throw std::runtime_error("Not an Elf file.");
    }

    if (!check_bit_depth()) {
        throw std::runtime_error("Bit depth of file is not 32bit.");
    }

    if (!check_endianness()) {
        throw std::runtime_error("Not little endian file.");
    }

    if (!check_isa()) {
        throw std::runtime_error("Not RISC-V architecture file.");
    }

    Elf32_Shdr shstrtab;
    // e_shoff - section header table's file offset in bytes
    // e_shstrndx - section header table index
    fseek(elf_file, elf_header_.e_shoff + elf_header_.e_shstrndx * sizeof(Elf32_Shdr), SEEK_SET);

    // reads section header
    fread(&shstrtab, sizeof(Elf32_Shdr), 1, elf_file);

    char *section_names = new char[shstrtab.sh_size];
    fseek(elf_file, shstrtab.sh_offset, SEEK_SET);
    fread(section_names, shstrtab.sh_size, 1, elf_file);
    Elf32_Shdr text_section_hdr, symtab_section_hdr, strtab_section_hdr;

    // Iterate through all section headers
    for (size_t i = 0; i < elf_header_.e_shnum; i++) {
        Elf32_Shdr cur_section_hdr;

        fseek(elf_file, elf_header_.e_shoff + i * sizeof(Elf32_Shdr), SEEK_SET);
        fread(&cur_section_hdr, sizeof(Elf32_Shdr), 1, elf_file);

        if (strcmp(section_names + cur_section_hdr.sh_name, ".text") == 0) {
            text_section_idx = i;
            text_start_addr = cur_section_hdr.sh_addr;
            text_section_hdr = cur_section_hdr;
        }

        else if (strcmp(section_names + cur_section_hdr.sh_name, ".symtab") == 0) {
            symtab_section_hdr = cur_section_hdr;
        }

        else if (strcmp(section_names + cur_section_hdr.sh_name, ".strtab") == 0) {
            strtab_section_hdr = cur_section_hdr;

        }
    }

    read_text_section(text_section_hdr);
    read_symtable_section(symtab_section_hdr);
    read_strtab_section(strtab_section_hdr);

    delete[] section_names;
}

Elf_parser::~Elf_parser() {
    fclose(elf_src_);
    delete[] symbol_names_;
}


void Elf_parser::read_text_section(Elf32_Shdr& text_section_hdr) {
    Elf32_Addr command_ptr = text_section_hdr.sh_offset;
    while (command_ptr != text_section_hdr.sh_offset + text_section_hdr.sh_size) {
        Elf32_Word command;
        // Go to start byte of command
        fseek(elf_src_, command_ptr, SEEK_SET);
        // Read command
        fread(&command, sizeof(Elf32_Word), 1, elf_src_);
        text_.push_back(command);
        // Move pointer to the next command
        command_ptr += sizeof(Elf32_Word);
    }
}

void Elf_parser::read_symtable_section(Elf32_Shdr &symtable_section_hdr) {
    // Calculate number of symbols in .symtab
    size_t number_of_symbols = symtable_section_hdr.sh_size / sizeof(Elf32_Sym);
    // Move pointer to the start of .symtab section
    fseek(elf_src_, symtable_section_hdr.sh_offset, SEEK_SET);
    symtab_.resize(number_of_symbols);

    // Read every symbol
    for (size_t i = 0; i < number_of_symbols; i++) {
        fread(&symtab_[i], sizeof(Elf32_Sym), 1, elf_src_);
    }
}

void Elf_parser::read_strtab_section(Elf32_Shdr &strtab_section_hdr) {
    symbol_names_ = new char[strtab_section_hdr.sh_size];
    // Move pointer to the start of strtab
    fseek(elf_src_, strtab_section_hdr.sh_offset, SEEK_SET);
    // Read all symbol names from strtab
    fread(symbol_names_, strtab_section_hdr.sh_size, 1, elf_src_);
}


// checks first 4 magic bytes: [0x7f, E, L, F]
bool Elf_parser::check_magic_bytes() {
    auto e_ident = elf_header_.e_ident;
    if (e_ident[0] != EI_MAG0) {
        return false;
    }
    if (e_ident[1] != EI_MAG1) {
        return false;
    }
    if (e_ident[2] != EI_MAG2) {
        return false;
    }
    if (e_ident[3] != EI_MAG3) {
        return false;
    }
    return true;
}

// checks if Elf file's bit depth is 32
bool Elf_parser::check_bit_depth() {
    if (elf_header_.e_ident[4] != EI_CLASS) {
        return false;
    }
    return true;
}

// checks is little endian Elf file
bool Elf_parser::check_endianness() {
    if (elf_header_.e_ident[5] != EI_DATA) {
        return false;
    }
    return true;
}

// checks is file built for RISC-V architecture
bool Elf_parser::check_isa() {
    if (elf_header_.e_machine != ISA) {
        return false;
    }
    return true;
}

std::vector<Elf32_Sym> Elf_parser::get_symtab() {
    return symtab_;
}

Elf32_Word Elf_parser::get_text_section_idx() {
    return text_section_idx;
}

std::vector<Elf32_Word> Elf_parser::get_text() {
    return text_;
}

Elf32_Addr Elf_parser::get_text_start_addr() {
    return text_start_addr;
}

const char* Elf_parser::get_symbol_bind(char byte) {
    switch (byte) {
        case 0:
            return "LOCAL";
        case 1:
            return "GLOBAL";
        case 2:
            return "WEAK";
        case 10:
            return "LOOS";
        case 12:
            return "HIOS";
        case 13:
            return "LOPROC";
        case 15:
            return "HIPROC";
    }
}

std::string Elf_parser::get_symbol_index(Elf32_Half ndx) {
    switch (ndx) {
        case 0:
            return "UNDEF";
        case 0xff00:
            return "LOPROC";
        case 0xff1f:
            return "HIPROC";
        case 0xff20:
            return "LOOS";
        case 0xff3f:
            return "HIOS";
        case 0xfff1:
            return "ABS";
        case 0xfff2:
            return "COMMON";
        case 0xffff:
            return "XINDEX";
        default:
            return std::to_string(ndx);
    }
}

const char* Elf_parser::get_symbol_type(char byte) {
    switch (byte) {
        case 0:
            return "NOTYPE";
        case 1:
            return "OBJECT";
        case 2:
            return "FUNC";
        case 3:
            return "SECTION";
        case 4:
            return "FILE";
        case 5:
            return "COMMON";
        case 6:
            return "TLS";
        case 10:
            return "LOOS";
        case 12:
            return "HIOS";
        case 13:
            return "LOPROC";
        case 15:
            return "HIPROC";
    }
}

const char* Elf_parser::get_symbol_visibility(char byte) {
    switch (byte) {
        case 0:
            return "DEFAULT";
        case 1:
            return "INTERNAL";
        case 2:
            return "HIDDEN";
        case 3:
            return "PROTECTED";
    }
}

const char* Elf_parser::get_symbol_name(Elf32_Word st_name) {
    return (symbol_names_ + st_name);
}