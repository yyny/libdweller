section .debug_info
debug_info:
    dd (.end - $ - 4) ; .length
    dw 0x04           ; .version
    dd 0x00           ; .debug_abbrev_offset
    db 0x04           ; .address_size
.end:
