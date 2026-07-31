# Cartridge header + loader, resident in the >6000 ROM window.
#
# The game doesn't fit in a plain 8K cartridge (it's about 13K), so this is a
# 16K "paged16k" cart: two 8K banks sharing the one >6000-7FFF window, with
# the bank selected by writing to >6000 (bank 0) or >6002 (bank 1). Rather
# than make the C code bank-aware, the cart is just a loader -- it copies the
# whole program out of both banks into the 32K expansion RAM at >A000 and
# jumps there, after which the ROM is never touched again and the game runs as
# ordinary flat code.
#
# The one trick that matters: this stub is byte-for-byte identical at the same
# address in BOTH banks (tools/pack.py writes it into each). Switching banks
# swaps the memory out from under the program counter, so the instruction
# stream has to continue unchanged on the other side of the switch.
#
# The parameter block below is filled in by tools/pack.py once it knows how
# the linked game actually laid out; pack.py finds these by symbol name.

  def _start
  def ldest
  def lsize
  def lbssad
  def lbsssz
  def lentry
  def lchunk
  def payload

# GROM header: magic, version, number of programs, unused
  byte 0xAA, 0x01, 0x00, 0x00
  data 0x0000           # power-up chain (unused)
  data program_record   # program chain
  data 0x0000           # DSR chain (unused)
  data 0x0000           # subprogram list (unused; also ends the program chain)
program_record:
  data 0x0000
  data _start
  nstring "UNO"
  even

# Patched by tools/pack.py.
ldest:  data 0x0000     # where the program runs from (>A000)
lsize:  data 0x0000     # bytes of text+data to copy
lbssad: data 0x0000     # start of .bss
lbsssz: data 0x0000     # size of .bss
lentry: data 0x0000     # address of main()
lchunk: data 0x0000     # how many payload bytes fit in bank 0

_start:
  limi 0                # interrupts off; the game re-enables them via the VDP
  lwpi >8300            # workspace in scratchpad -- kscan() requires exactly this
  li   r10, >4000       # stack at the top of the low 8K, growing down

  li   r1, payload      # ROM source (same offset in both banks)
  mov  @ldest, r2       # RAM destination
  mov  @lsize, r4       # total bytes still to copy
  mov  @lchunk, r5      # capacity of bank 0

  c    r4, r5
  jh   split            # more than bank 0 holds: use both banks
  mov  r4, r5           # otherwise copy the lot from bank 0
  clr  r4
  jmp  copy0
split:
  s    r5, r4           # r4 = the remainder, left for bank 1
copy0:
  mov  r5, r5
  jeq  copy0done
c0loop:
  movb *r1+, *r2+
  dec  r5
  jne  c0loop
copy0done:

  mov  r4, r4
  jeq  zerobss
# Swap in bank 1 and carry on. r2 is already at the right point in RAM; the
# source restarts at the top of the payload window, which now shows bank 1.
  clr  @>6002
  li   r1, payload
c1loop:
  movb *r1+, *r2+
  dec  r4
  jne  c1loop

zerobss:
  mov  @lbssad, r2
  mov  @lbsssz, r4
  clr  r5
  mov  r4, r4
  jeq  launch
bzloop:
  movb r5, *r2+
  dec  r4
  jne  bzloop

launch:
  mov  @lentry, r1
  b    *r1

# The payload is appended here by pack.py, filling the rest of each bank.
  even
payload:
