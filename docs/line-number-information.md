# Line Number Information

Instead of storing a giant table mapping instruction addresses to line
numbers, DWARF uses a state machine.
The advantage to this is that complicated instruction mappings (such as when
instructions are re-ordered as an optimization) take much less space to
express.
The disadvantage is that it's possible to construct or corrupt a state machine
so that it does weird and possibly unsafe things, like overwriting an already
defined mapping or assigning a single instruction to multiple lines.

## Standard Opcodes

Limited to 255 opcodes (`0x00` is reserved for beginning extended opcodes).
May take a predefined number of LEB128 arguments.
Unrecognized opcodes can typically be ignored.
Unused opcodes get special meaning.

```
opcode = get8();
assert(opcode != 0x00);
if opcode < num_basic_opcodes then
    length = basic_opcode_lengths[opcode];
    args = Array(length);
    for i in 0..length do
        args[i] = getvar64();
    end
else
    # Just the opcode, no arguments
end
```

### Special Opcodes
Any Standard Opcode greater than `num_basic_opcodes` is a special opcode.
Special opcodes have a pre-calculated and predefined meaning.
In most line number programs, standard opcodes are used the most, so they
only take up one byte of space.

### Caveats
The `0x09` `DW_LNS_fixed_advance_pc` opcode takes a single `u16` argument
instead of a `LEB128`.

## Extended Opcodes

Limited to 256 opcodes.
May take any number of bytes as arguments.
Unrecognized and unused opcodes can be ignored.

```
expect(0x00);
length = getvar64();
opcode = getvar64(); # Get opcode
args = Array(length);
for i in 0..length-1 do # Get arguments
    args[i] = get8();
end
```
