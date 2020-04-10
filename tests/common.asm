%ifidn __OUTPUT_FORMAT__, elf32
%define ADDRESS_SIZE 0x04
%elifidn __OUTPUT_FORMAT__, elf64
%define ADDRESS_SIZE 0x08
%endif
