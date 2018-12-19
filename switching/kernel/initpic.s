	.text
	.global initPIC

        .equ    IRQ_BASE,   0x20        # lowest hw irq number

        #------------------------------------------------------------------
        # Initialize PIC:
        # Configure standard 8259 programmable interrupt controller
        # to remap hardware irqs 0x0-0xf into the range 0x20-0x2f.
        .equ    PIC_1, 0x20
        .equ    PIC_2, 0xa0

        # Send ICWs (initialization control words) to initialize PIC.
        # TODO: Some sources suggest that there should be a delay between
        # each output byte ... but I don't see that in the datasheet ...
        .macro  initpic port, base, info, init
        movb    $0x11, %al
        outb    %al, $\port     # ICW1: Initialize + will be sending ICW4

        movb    $\base, %al     # ICW2: Interrupt vector offset 
        outb    %al, $(\port+1)

        movb    $\info, %al     # ICW3: Configure for two PICs
        outb    %al, $(\port+1)

        movb    $0x01, %al      # ICW4: 8086 mode
        outb    %al, $(\port+1)

        movb    $\init, %al     # OCW1: set initial mask
        outb    %al, $(\port+1)
        .endm

initPIC:initpic PIC_1, IRQ_BASE,   0x04, 0xfb  # all but IRQ2 masked out
        initpic PIC_2, IRQ_BASE+8, 0x02, 0xff
	ret

