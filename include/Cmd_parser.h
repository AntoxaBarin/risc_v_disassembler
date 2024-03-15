#pragma once

#include "Elf_parser.h"
#include <vector>
#include <string>
#include <map>
#include <utility>

class Cmd_parser {
public:
    Cmd_parser(Elf_parser& elf_file);
    std::vector<std::string> parse_cmds();

private:
    Elf_parser& elf_file_;
    std::map<Elf32_Word, std::string> symtab_;
    Elf32_Addr cur_addr_ptr_;
    Elf32_Word L_label_counter_;

    bool get_bit(Elf32_Word value, size_t pos);
    void set_bit(Elf32_Word& dst, size_t pos, bool bit);
    void write_bits(Elf32_Word src, size_t start, size_t end, Elf32_Word& dst, size_t pos);
    Elf32_Word read_bits_unsigned(Elf32_Word src, size_t start, size_t end);
    Elf32_Word get_opcode(Elf32_Word src);
    std::string parse_cmd(Elf32_Word cmd);
    std::pair<std::string, std::string> get_succ_pred(Elf32_Word cmd);

    std::string parse_R_type(Elf32_Word cmd);
    std::string parse_S_type(Elf32_Word cmd);
    std::string parse_U_type(Elf32_Word cmd);
    std::string parse_J_type(Elf32_Word cmd);
    std::string parse_B_type(Elf32_Word cmd);
    std::string parse_I_type(Elf32_Word cmd);
    std::string parse_Fence(Elf32_Word cmd);


    std::string get_label_name(Elf32_Addr addr);

    Elf32_Word read_rd(Elf32_Word cmd);
    Elf32_Word read_rs1(Elf32_Word cmd);
    Elf32_Word read_rs2(Elf32_Word cmd);
    Elf32_Word read_funct3(Elf32_Word cmd);
    Elf32_Word read_funct2(Elf32_Word cmd);
    Elf32_Word read_funct5(Elf32_Word cmd);
    Elf32_Word read_fence_bits(Elf32_Word cmd);
    Elf32_Word read_fence_fm(Elf32_Word cmd);
    Elf32_Word read_fence_pred(Elf32_Word cmd);
    Elf32_Word read_fence_succ(Elf32_Word cmd);

    std::string get_cmd_name_R_type(Elf32_Word funct3, Elf32_Word funct2, Elf32_Word funct5);
    std::string get_cmd_name_S_type(Elf32_Word funct3);
    std::string get_cmd_name_B_type(Elf32_Word funct3);
    std::string get_cmd_name_I_type(Elf32_Word opcode, Elf32_Word funct3, Elf32_Word funct5);
    std::string get_cmd_name_U_type(Elf32_Word opcode);

    std::string get_register(Elf32_Word reg);
};