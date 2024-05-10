# risc_v_disassembler
Simple RISC-V disassembler. Supported command sets: [RV32I](https://msyksphinz-self.github.io/riscv-isadoc/html/rvi.html), [RV32M](https://msyksphinz-self.github.io/riscv-isadoc/html/rvm.html).
The test ELF file is in the `test_data` folder. 


## Build 
```
make
```
## Usage
```
./risc_disasm <input_elf_file> <output_file>
```

## Example
```
./risc_disasm test_data/test_elf test_data/output_test.txt
```

Disassembler gets text and symtab sections from ELF file, parses commands and writes result in text file.

```
.text

00010074 	<main>:
   10074:	ff010113	   addi	sp, sp, -16
   10078:	00112623	     sw	ra, 12(sp)
   1007c:	030000ef	    jal	ra, 0x100ac <mmul>
   10080:	00c12083	     lw	ra, 12(sp)
   10084:	00000513	   addi	a0, zero, 0
...
   
 .symtab

Symbol Value              Size Type     Bind     Vis       Index Name
[   0] 0x0                   0 NOTYPE   LOCAL    DEFAULT   UNDEF 
[   1] 0x10074               0 SECTION  LOCAL    DEFAULT       1 
[   2] 0x11124               0 SECTION  LOCAL    DEFAULT       2 
[   3] 0x0                   0 SECTION  LOCAL    DEFAULT       3 
[   4] 0x0                   0 SECTION  LOCAL    DEFAULT       4 
[   5] 0x0                   0 FILE     LOCAL    DEFAULT     ABS test.c
...
```
