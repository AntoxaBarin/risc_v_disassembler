#include "Elf_parser.h"
#include "Cmd_parser.h"
#include <iostream>
#include <string>
using namespace std;

void write_cmds(FILE *output, Elf_parser& elf_src) {
    fprintf(output, ".text\n");
    std::vector<std::string> cmds = Cmd_parser(elf_src).parse_cmds();
    for (size_t i = 0; i < cmds.size(); i++) {
        fprintf(output, "%s\n", cmds[i].c_str());
    }
}

void write_symtab_in_file(FILE *output, Elf_parser& elf_src) {
    fprintf(output, ".symtab\n");
    fprintf(output, "\nSymbol Value              Size Type     Bind     Vis       Index Name\n");
    std::vector<Elf32_Sym> symtab = elf_src.get_symtab();

    for (size_t i = 0; i < symtab.size(); i++) {
        Elf32_Sym symbol = symtab[i];
        unsigned char sym_info  = symbol.st_info;
        unsigned char sym_other = symbol.st_other;
        Elf32_Addr    sym_value = symbol.st_value;
        Elf32_Word    sym_size  = symbol.st_size;
        const char*   sym_type  = elf_src.get_symbol_type(ELF32_ST_TYPE(sym_info));
        const char*   sym_bind  = elf_src.get_symbol_bind(ELF32_ST_BIND(sym_info));
        const char*   sym_vis   = elf_src.get_symbol_visibility(ELF32_ST_VISIBILITY(sym_other));
        std::string   sym_index = elf_src.get_symbol_index(symbol.st_shndx);
        const char*   sym_name  = elf_src.get_symbol_name(symbol.st_name);

        fprintf(output, "[%4i] 0x%-15X %5i %-8s %-8s %-8s %6s %s\n",
                (int)i, sym_value, sym_size, sym_type, sym_bind, sym_vis, sym_index.c_str(), sym_name);
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Wrong number of arguments.\n" << std::endl;
        return 1;
    }

    FILE *input_file = fopen(argv[1], "rb");
    if (input_file == nullptr) {
        std::cerr << "Invalid input file.\n";
        return 1;
    }

    FILE *output_file = fopen(argv[2], "w");
    if (output_file == nullptr) {
        std::cerr << "Invalid output file.\n" << std::endl;
        return 1;
    }

    try {
        Elf_parser parser = Elf_parser(input_file);
        write_cmds(output_file, parser);
        fprintf(output_file, "\n\n");
        write_symtab_in_file(output_file, parser);
    } catch (std::exception &e) {
        std::cout << std::string(e.what()) << std::endl;
    }

    return 0;
}
