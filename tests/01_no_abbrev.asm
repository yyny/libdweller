%include "common.asm"

section .debug_info
debug_info:
    dd (.end - $ - 4) ; .length
    dw 0x04           ; .version
    dd 0x00           ; .debug_abbrev_offset
    db ADDRESS_SIZE   ; .address_size
.end:
