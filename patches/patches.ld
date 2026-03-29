/* Pokemon Snap Recomp - Patches Linker Script */
/* Patches are placed in extended RAM beyond the original game's address space */

RAMBASE = 0x80801000;
PATCH_RAM_END = 0x81000000;

SECTIONS {
    .text RAMBASE : {
        *(.text)
        *(.text.*)
        *(.recomp_patch)
        *(.recomp_force_patch)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        *(.bss)
        *(.bss.*)
    }

    .recomp_export : {
        *(.recomp_export)
    }

    .recomp_event : {
        *(.recomp_event)
    }

    .recomp_callback : {
        *(.recomp_callback)
    }

    /DISCARD/ : {
        *(.MIPS.abiflags)
        *(.MIPS.options)
        *(.reginfo)
        *(.comment)
        *(.note.*)
        *(.gnu.attributes)
    }
}
