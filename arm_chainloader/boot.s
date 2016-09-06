/*
At this point, the kernel is already ready in memory;
all that is left is setting up some registers and jumping.
However, it's kind of ugly to do it in inline asm.. so here
*/

  /*
   r0: 0
   r1: machine ID (3138/3189 for Pi 1/2)
   r2: DTB address
   r3: kernel address
   */

.globl bootLinux
bootLinux:
    //cli /* TODO: figure out how to disable interrupts correctly */

    /* jump to kernel */
    mov pc, r3
