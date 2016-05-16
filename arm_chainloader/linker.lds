MEMORY
{
    ram : ORIGIN = 0x0, LENGTH = 0x100000
}

SECTIONS
{
    .text : { *(.text*) } > ram
    .bss : { *(.bss*) } > ram
}