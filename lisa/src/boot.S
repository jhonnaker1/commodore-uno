| boot.S - the Lisa 2's boot block, and the floppy loader in it.
|
| The boot ROM (341-0175-H/0176-H) reads block 0 of the floppy into
| $20000, with its 12 tag bytes just below it at $1FFF4, and starts it
| only if the tag's file ID (bytes 4-5) is $AAAA; then it jumps to $20000
| in supervisor mode (rom.dis, $FE1BCC-$FE1C96). This block loads the
| rest of the program -- the blocks after it, straight on from $20200 --
| through the floppy controller's shared memory, the way the ROM itself
| reads, and jumps to the C start-up.
|
| Low memory the ROM leaves (its $FE066C): $110 is the screen's logical
| address (the top 32K of RAM), $2A8 the end of RAM. The stack goes just
| under the screen.

        .section .boot,"ax"
        .globl  _start
_start:
        move.w  #0x2700,%sr             | no interrupts: everything is polled
        move.l  0x110.w,%sp
        lea     0xFCC001,%a0            | floppy controller shared memory
        moveq   #0,%d7
        move.b  4(%a0),%d7              | the drive the ROM booted from
        ror.l   #8,%d7                  | d7 = drive<<24 (side 0)
        lea     0x20200,%a2             | where block 1 goes
        move.l  #__load_blocks,%d6      | blocks to load (the linker's sum)
        moveq   #0,%d5                  | track
        moveq   #1,%d4                  | sector (block 1 = track 0 sector 1)
        bra.s   2f

1:      move.l  %d7,%d1
        move.w  %d4,%d0
        lsl.w   #8,%d0
        or.w    %d5,%d0
        move.w  %d0,%d1                 | d1 = drive.side.sector.track
        bsr.s   readsec
        bcs.s   fail
        addq.w  #1,%d4                  | next sector; 12 a track on tracks
        moveq   #12,%d0                 | 0-15, 11 on 16-31, ... 8 on 64-79
        move.w  %d5,%d1
        lsr.w   #4,%d1
        sub.w   %d1,%d0
        cmp.w   %d0,%d4
        bne.s   2f
        moveq   #0,%d4
        addq.w  #1,%d5
2:      dbra    %d6,1b
        jmp     _cstart

| Loading failed: a black screen.
fail:   move.l  0x110.w,%a0
        move.w  #32760/4-1,%d0
3:      move.l  #-1,(%a0)+
        dbra    %d0,3b
4:      bra.s   4b

| readsec: read the sector d1 names to (a2)+, 512 bytes. a0 = the shared
| memory. Carry set on an error. The ROM's sequence ($FE1D70): the
| command (0 = read) and the drive/side/sector/track go in, $81 in the go
| byte runs it; the controller raises FDIR (VIA1 port B bit 4) when it is
| done, the status byte says how it went, and $85 with $CC clears FDIR.
| The sector is in the shared memory's odd bytes: 12 tag bytes at $3E8,
| then the 512 data bytes from $400.
readsec:
        movem.l %d2-%d3/%a3-%a4,-(%sp)
        lea     0xFCDD81,%a3            | VIA1 port B
        btst    #4,(%a3)
        beq.s   1f
        bsr.s   clrstat                 | an old FDIR still up
1:      movep.l %d1,4(%a0)
        clr.b   12(%a0)
        clr.b   2(%a0)                  | read
        move.b  #0x81,(%a0)             | execute
        move.l  #0xC00000,%d2
2:      btst    #4,(%a3)                | wait for FDIR
        bne.s   3f
        subq.l  #1,%d2
        bne.s   2b
        bra.s   9f
3:      move.b  16(%a0),%d3             | status
        bsr.s   clrstat
        bcs.s   9f
        tst.b   %d3
        bne.s   9f
        lea     0x400(%a0),%a4
        moveq   #31,%d2
4:      movep.l 0(%a4),%d0
        move.l  %d0,(%a2)+
        movep.l 8(%a4),%d0
        move.l  %d0,(%a2)+
        movep.l 16(%a4),%d0
        move.l  %d0,(%a2)+
        movep.l 24(%a4),%d0
        move.l  %d0,(%a2)+
        lea     32(%a4),%a4
        dbra    %d2,4b
        movem.l (%sp)+,%d2-%d3/%a3-%a4
        and.b   #0xFE,%ccr
        rts
9:      movem.l (%sp)+,%d2-%d3/%a3-%a4
        or.b    #1,%ccr
        rts

| clrstat: clear the controller's status (FDIR) and wait until it has
| taken the command (the go byte back to 0). Carry set on a timeout.
clrstat:
        move.l  %d2,-(%sp)
        move.b  #0xCC,2(%a0)
        move.b  #0x85,(%a0)
        move.l  #0x120000,%d2
1:      tst.b   (%a0)
        beq.s   2f
        subq.l  #1,%d2
        bne.s   1b
        move.l  (%sp)+,%d2
        or.b    #1,%ccr
        rts
2:      move.l  (%sp)+,%d2
        and.b   #0xFE,%ccr
        rts
