#include "Cmd_parser.h"

Cmd_parser::Cmd_parser(Elf_parser &elf_file) : elf_file_(elf_file), cur_addr_ptr_(0), L_label_counter_(0) {
    std::vector<Elf32_Sym> sym = elf_file_.get_symtab();

    for (size_t i = 0; i < sym.size(); i++) {
        const char *label = elf_file_.get_symbol_name(sym[i].st_name);
        // Save symbols from .text section
        if (sym[i].st_shndx == elf_file_.get_text_section_idx()) {
            symtab_[sym[i].st_value] = std::string(label);
        }
    }
}

bool Cmd_parser::get_bit(Elf32_Word value, size_t pos) {
    return value & (1 << pos);
}

void Cmd_parser::set_bit(Elf32_Word& dst, size_t pos, bool bit) {
    if (get_bit(dst, pos) != bit) {
        dst ^= (1 << pos);
    }
}

void Cmd_parser::write_bits(Elf32_Word src, size_t start, size_t end, Elf32_Word& dst, size_t pos) {
    for (size_t i = 0; i <= end - start; i++) {
        set_bit(dst, pos + i, get_bit(src, start + i));
    }
}

Elf32_Word Cmd_parser::read_bits_unsigned(Elf32_Word src, size_t start, size_t end) {
    Elf32_Word result = 0;
    write_bits(src, start, end, result, 0);
    return result;
}

Elf32_Word Cmd_parser::get_opcode(Elf32_Word src) {
    return read_bits_unsigned(src, 0, 6);
}

std::string Cmd_parser::get_label_name(Elf32_Addr addr) {
    // Trying to find label in symtab
    if (symtab_.find(addr) != symtab_.end()) {
        return symtab_[addr];
    }
    // Create new label
    std::string new_label = "L" + std::to_string(L_label_counter_++);
    symtab_.insert(std::make_pair(addr, new_label));
    return new_label;
}

std::pair<std::string, std::string> Cmd_parser::get_succ_pred(Elf32_Word cmd) {
    Elf32_Word succ = read_bits_unsigned(cmd, 20, 23);
    Elf32_Word pred = read_bits_unsigned(cmd, 24, 27);
    std::string succ_letters = "";
    std::string pred_letters = "";

    if (get_bit(succ, 3)) {
        succ_letters += "i";
    }
    if (get_bit(succ, 2)) {
        succ_letters += "o";
    }
    if (get_bit(succ, 1)) {
        succ_letters += "r";
    }
    if (get_bit(succ, 0)) {
        succ_letters += "w";
    }

    if (get_bit(pred, 3)) {
        pred_letters += "i";
    }
    if (get_bit(pred, 2)) {
        pred_letters += "o";
    }
    if (get_bit(pred, 1)) {
        pred_letters += "r";
    }
    if (get_bit(pred, 0)) {
        pred_letters += "w";
    }
    return std::make_pair(pred_letters, succ_letters);
}

std::vector<std::string> Cmd_parser::parse_cmds() {
    std::vector<Elf32_Word> cmds = elf_file_.get_text();
    std::vector<std::string> result;
    cur_addr_ptr_ = elf_file_.get_text_start_addr();

    // Parse cmds
    for (size_t i = 0; i < cmds.size(); i++) {
        char fmt_string[1000];
        Elf32_Word cmd = cmds[i];
        std::string parsed_cmd = parse_cmd(cmd);
        sprintf(fmt_string, "   %05x:\t%08x\t%s", cur_addr_ptr_, cmd, parsed_cmd.c_str());
        result.push_back(std::string(fmt_string));
        cur_addr_ptr_ += 4;    // += sizeof(Elf32_Word)
    }

    // Add label strings
    std::vector<std::string>::iterator it;
    cur_addr_ptr_ = elf_file_.get_text_start_addr();
    size_t i = 0;
    for (it = result.begin(); it < result.end(); it++) {
        if (symtab_.find(cur_addr_ptr_) != symtab_.end()) {
            char fmt_string2[1000];
            sprintf(fmt_string2, "\n%08x \t<%s>:", cur_addr_ptr_, symtab_[cur_addr_ptr_].c_str());
            result.insert(it, std::string(fmt_string2));
            it++;
        }
        cur_addr_ptr_ += 4;
        i++;
    }

    return result;
}

std::string Cmd_parser::parse_R_type(Elf32_Word cmd) {
    Elf32_Word rd = read_rd(cmd);
    Elf32_Word funct3 = read_funct3(cmd);
    Elf32_Word rs1 = read_rs1(cmd);
    Elf32_Word rs2 = read_rs2(cmd);
    Elf32_Word funct2 = read_funct2(cmd);
    Elf32_Word funct5 = read_funct5(cmd);

    std::string cmd_name = get_cmd_name_R_type(funct3, funct2, funct5);

    if (cmd_name == "invalid_instruction") {
        char fmt[1000];
        sprintf(fmt, "%-7s", "invalid_instruction");
        return std::string(fmt);
    }

    char fmt[1000];
    sprintf(fmt, "%7s\t%s, %s, %s", cmd_name.c_str(), get_register(rd).c_str(), get_register(rs1).c_str(), get_register(rs2).c_str());

    return std::string(fmt);

}

std::string Cmd_parser::parse_S_type(Elf32_Word cmd) {
    Elf32_Word funct3 = read_funct3(cmd);
    Elf32_Word rs1 = read_rs1(cmd);
    Elf32_Word rs2 = read_rs2(cmd);
    std::string cmd_name = get_cmd_name_S_type(funct3);

    if (cmd_name == "invalid_instruction") {
        char fmt[1000];
        sprintf(fmt, "%-7s", "invalid_instruction");
        return std::string(fmt);
    }

    // Build imm
    Elf32_Word imm = 0;
    write_bits(cmd, 7, 11, imm, 0);
    write_bits(cmd, 25, 31, imm, 5);
    for (size_t i = 12; i < 32; i++) {
        write_bits(cmd, 31, 31, imm, i);
    }
    int32_t signed_imm = imm;

    char fmt[1000];
    sprintf(fmt, "%7s\t%s, %d(%s)", cmd_name.c_str(), get_register(rs2).c_str(), signed_imm, get_register(rs1).c_str());
    return std::string(fmt);
}

std::string Cmd_parser::parse_Fence(Elf32_Word cmd) {
    Elf32_Word fence_bits = read_fence_bits(cmd);  // [31..20] bits

    if (fence_bits == 0b000000010000) {
        char fmt[1000];
        sprintf(fmt, "%7s", "pause");
        return std::string(fmt);
    }
    else if (fence_bits == 0b100000110011) {
        char fmt[1000];
        sprintf(fmt, "%7s", "fence.tso");
        return std::string(fmt);
    }
    else {
        auto pred_succ_values = get_succ_pred(cmd);
        char fmt[1000];
        sprintf(fmt, "%7s\t%s, %s", "fence", pred_succ_values.first.c_str(), pred_succ_values.second.c_str());
        return std::string(fmt);
    }
}

std::string Cmd_parser::parse_J_type(Elf32_Word cmd) {
    Elf32_Word rd = read_rd(cmd);

    // Build imm
    Elf32_Word imm = 0;
    write_bits(cmd, 12, 19, imm, 12);
    write_bits(cmd, 20, 20, imm, 11);
    write_bits(cmd, 21, 30, imm, 1);
    write_bits(cmd, 31, 31, imm, 20);
    for (size_t i = 21; i < 32; i++) {
        write_bits(cmd, 31, 31, imm, i);
    }
    int32_t signed_imm = imm;
    Elf32_Addr label_addr = signed_imm + cur_addr_ptr_;
    char fmt[1000];
    sprintf(fmt, "%7s\t%s, 0x%x <%s>", "jal", get_register(rd).c_str(), label_addr, get_label_name(label_addr).c_str());
    return std::string(fmt);

}

std::string Cmd_parser::parse_U_type(Elf32_Word cmd) {
    Elf32_Word rd = read_rd(cmd);
    std::string cmd_name = get_cmd_name_U_type(get_opcode(cmd));

    if (cmd_name == "invalid_instruction") {
        char fmt[1000];
        sprintf(fmt, "%-7s", "invalid_instruction");
        return std::string(fmt);
    }

    // Parse imm from cmd
    Elf32_Word imm = 0;
    write_bits(cmd, 12, 31, imm, 0);
    int32_t signed_imm = imm;

    char fmt[1000];
    sprintf(fmt, "%7s\t%s, 0x%x", cmd_name.c_str(), get_register(rd).c_str(), signed_imm);
    return std::string(fmt);
}

std::string Cmd_parser::parse_B_type(Elf32_Word cmd) {
    Elf32_Word funct3 = read_funct3(cmd);
    Elf32_Word rs1 = read_rs1(cmd);
    Elf32_Word rs2 = read_rs2(cmd);
    std::string cmd_name = get_cmd_name_B_type(funct3);

    if (cmd_name == "invalid_instruction") {
        char fmt[1000];
        sprintf(fmt, "%-7s", "invalid_instruction");
        return std::string(fmt);
    }

    Elf32_Word imm = 0;
    write_bits(cmd, 8, 11, imm, 1);
    write_bits(cmd, 25, 30, imm, 5);
    write_bits(cmd, 7, 7, imm, 11);
    write_bits(cmd, 31, 31, imm, 12);
    for (size_t i = 13; i < 32; i++) {
        write_bits(cmd, 31, 31, imm, i);
    }
    int32_t signed_imm = imm;

    Elf32_Addr label_addr = signed_imm + cur_addr_ptr_;
    char fmt[1000];
    sprintf(fmt, "%7s\t%s, %s, 0x%x, <%s>", cmd_name.c_str(), get_register(rs1).c_str(), get_register(rs2).c_str(), label_addr, get_label_name(label_addr).c_str());
    return std::string(fmt);

}

std::string Cmd_parser::parse_I_type(Elf32_Word cmd) {
    Elf32_Word opcode = get_opcode(cmd);
    Elf32_Word rd = read_rd(cmd);
    Elf32_Word funct3 = read_funct3(cmd);
    Elf32_Word rs1 = read_rs1(cmd);

    Elf32_Word imm = 0;
    write_bits(cmd, 20, 31, imm, 0);
    for (size_t i = 12; i < 32; i++) {
        write_bits(cmd, 31, 31, imm, i);
    }
    int32_t signed_imm = imm;

    if (opcode == 0b1110011) {
        if (rd == 0 && rs1 == 0 && funct3 == 0 && signed_imm == 0) {
            char fmt[1000];
            sprintf(fmt, "%7s", "ecall");
            return std::string(fmt);
        }
        else if (rd == 0 && rs1 == 0 && funct3 == 0 && signed_imm == 1) {
            char fmt[1000];
            sprintf(fmt, "%7s", "ebreak");
            return std::string(fmt);
        }
    }

    else if (opcode == 0b1100111 && funct3 == 0) {
        char fmt[1000];
        sprintf(fmt, "%7s\t%s, %d(%s)", "jalr", get_register(rd).c_str(), signed_imm, get_register(rs1).c_str());
        return std::string(fmt);
    }

    else if (opcode == 0b0010011) {
        std::string cmd_name = get_cmd_name_I_type(opcode, funct3, read_funct5(cmd));

        if (cmd_name == "invalid_instruction") {
            char fmt[1000];
            sprintf(fmt, "%-7s", "invalid_instruction");
            return std::string(fmt);
        }

        char fmt[1000];
        sprintf(fmt, "%7s\t%s, %s, %s", cmd_name.c_str(), get_register(rd).c_str(), get_register(rs1).c_str(), std::to_string(signed_imm).c_str());
        return std::string(fmt);
    }
    else if (opcode == 0b0000011) {
        std::string cmd_name = get_cmd_name_I_type(opcode, funct3, read_funct5(cmd));

        if (cmd_name == "invalid_instruction") {
            char fmt[1000];
            sprintf(fmt, "%-7s", "invalid_instruction");
            return std::string(fmt);
        }

        char fmt[1000];
        sprintf(fmt, "%7s\t%s, %d(%s)", cmd_name.c_str(), get_register(rd).c_str(), signed_imm, get_register(rs1).c_str());
        return std::string(fmt);
    }
    else {
        char fmt[1000];
        sprintf(fmt, "%-7s", "invalid_instruction");
        return std::string(fmt);
    }
}

std::string Cmd_parser::get_cmd_name_R_type(Elf32_Word funct3, Elf32_Word funct2, Elf32_Word funct5) {
    if (funct2 == 0b0) {                    // 32I instruction
        switch (funct3) {
            case 0b000:
                if (funct5 == 0b01000) {
                    return "sub";
                }
                return "add";
            case 0b001:
                return "sll";
            case 0b010:
                return "slt";
            case 0b011:
                return "sltu";
            case 0b100:
                return "xor";
            case 0b101:
                if (funct5 == 0b01000) {
                    return "sra";
                }
                return "srl";
            case 0b110:
                return "or";
            case 0b111:
                return "and";
            default:
                return "invalid_instruction";
        }
    }
    else if (funct5 == 0b0 && funct2 == 0b1) {   // 32M instruction
        switch (funct3) {
            case 0b000:
                return "mul";
            case 0b001:
                return "mulh";
            case 0b010:
                return "mulhsu";
            case 0b011:
                return "mulhu";
            case 0b100:
                return "div";
            case 0b101:
                return "divu";
            case 0b110:
                return "rem";
            case 0b111:
                return "remu";
            default:
                return "invalid_instruction";
        }
    }

}

std::string Cmd_parser::get_cmd_name_S_type(Elf32_Word funct3) {
    switch (funct3) {
        case 0b000:
            return "sb";
        case 0b001:
            return "sh";
        case 0b010:
            return "sw";
        default:
            return "invalid_instruction";
    }
}

std::string Cmd_parser::get_cmd_name_B_type(Elf32_Word funct3) {
    switch (funct3) {
        case 0b000:
            return "beq";
        case 0b001:
            return "bne";
        case 0b100:
            return "blt";
        case 0b101:
            return "bge";
        case 0b110:
            return "bltu";
        case 0b111:
            return "bgeu";
        default:
            return "invalid_instruction";
    }
}

std::string Cmd_parser::get_cmd_name_U_type(Elf32_Word opcode) {
    switch (opcode) {
        case 0b0110111:
            return "lui";
        case 0b0010111:
            return "auipc";
        default:
            return "invalid_instruction";
    }
}

std::string Cmd_parser::get_cmd_name_I_type(Elf32_Word opcode, Elf32_Word funct3, Elf32_Word funct5) {
    if (opcode == 0b1100111 && funct3 == 0) {
        return "jalr";
    }
    else if (opcode == 0b0000011) {
        switch (funct3) {
            case 0b000:
                return "lb";
            case 0b001:
                return "lh";
            case 0b010:
                return "lw";
            case 0b100:
                return "lbu";
            case 0b101:
                return "lhu";
            default:
                return "invalid_instruction";
        }
    }
    else if (opcode == 0b0010011) {
        switch (funct3) {
            case 0b000:
                return "addi";
            case 0b010:
                return "slti";
            case 0b011:
                return "sltiu";
            case 0b100:
                return "xori";
            case 0b110:
                return "ori";
            case 0b111:
                return "andi";
            case 0b001:
                return "slli";
            case 0b101:
                if (funct5 == 0b01000) {
                    return "srai";
                }
                return "srli";
            default:
                return "invalid_instruction";
        }
    }
}

Elf32_Word Cmd_parser::read_rd(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 7, 11);
}

Elf32_Word Cmd_parser::read_rs1(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 15 ,19);
}

Elf32_Word Cmd_parser::read_rs2(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 20, 24);
}

Elf32_Word Cmd_parser::read_funct3(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 12, 14);
}

Elf32_Word Cmd_parser::read_funct2(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 25, 26);
}

Elf32_Word Cmd_parser::read_funct5(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 27, 31);
}

Elf32_Word Cmd_parser::read_fence_bits(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 20, 31);
}

Elf32_Word Cmd_parser::read_fence_fm(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 28, 31);
}

Elf32_Word Cmd_parser::read_fence_pred(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 24, 27);
}

Elf32_Word Cmd_parser::read_fence_succ(Elf32_Word cmd) {
    return read_bits_unsigned(cmd, 20, 23);
}

std::string Cmd_parser::parse_cmd(Elf32_Word cmd) {
    Elf32_Word cmd_opcode = get_opcode(cmd);
    if (cmd_opcode == 0b0110111 || cmd_opcode == 0b0010111) {
        return parse_U_type(cmd);
    }
    else if (cmd_opcode == 0b1101111) {  // Only one cmd - jal
        return parse_J_type(cmd);
    }
    else if (cmd_opcode == 0b1100011) {
        return parse_B_type(cmd);
    }
    else if (cmd_opcode == 0b1100111 || cmd_opcode == 0b0000011 || cmd_opcode == 0b0010011 || cmd_opcode == 0b1110011) {
        return parse_I_type(cmd);
    }
    else if (cmd_opcode == 0b0110011) {
        return parse_R_type(cmd);
    }
    else if (cmd_opcode == 0b0100011) {
        return parse_S_type(cmd);
    }
    else if (cmd_opcode == 0b0001111) {
        return parse_Fence(cmd);
    }
}

std::string Cmd_parser::get_register(Elf32_Word reg) {
    switch (reg) {
        case 0: return "zero";
        case 1: return "ra";
        case 2: return "sp";
        case 3: return "gp";
        case 4: return "tp";
        case 5: return "t0";
        case 6: return "t1";
        case 7: return "t2";
        case 8: return "s0";
        case 9: return "s1";
        default: {
            if (reg <= 17) {
                return "a" + std::to_string(reg - 10);
            }
            else if (reg <= 27) {
                return "s" + std::to_string(reg - 16);
            }
            return "t" + std::to_string(reg - 25);
        }
    }
}
