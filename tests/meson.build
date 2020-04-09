nasm = find_program('nasm')

tests = [
    '00_no_debug.asm',
    '01_no_abbrev.asm',
    '02_no_info.asm',
    '03_empty_abbrev.asm',
    '04_empty_info.asm'
    ]

foreach test : tests
    elf = custom_target(''.join([test.split('.')[0], '.32.elf']),
                        input : test,
                        output : ''.join([test.split('.')[0], '.32.elf']),
                        command : [nasm, '-f', 'elf32', '-o', '@OUTPUT@', '@INPUT@'])
    test(test.split('.')[0] + '.32', dwarfdump, args : [elf])

    elf = custom_target(''.join([test.split('.')[0], '.64.elf']),
                        input : test,
                        output : ''.join([test.split('.')[0], '.64.elf']),
                        command : [nasm, '-f', 'elf64', '-o', '@OUTPUT@', '@INPUT@'])
    test(test.split('.')[0] + '.64', dwarfdump, args : [elf])
endforeach

hello = executable('hello', files('hello.c'))

foreach example : examples
    test('examples/' + example.full_path().split('/')[-1], example, args : [hello])
endforeach
