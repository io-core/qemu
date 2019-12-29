  br  Begin
 mov  r0 r0
 mov  r0 r0
 mov  r0 r0
 mov  r0 r0
 mov  r0 r0
 mov  r0 r0
 mov  r0 r0

RecInt:
 sub  sp sp 14H
 st   lr sp 0
 st   r0 sp 4
 mov  r0 0H
 st   r0 sp 8
 mov  r0 4H
 st   r0 sp 16
N1:
 ld   r0 sp 16
 sub  r0 r0 1H
 st   r0 sp 16
N2:
 mov  r0 ffffffccH
 ld   r0 r0 0
 ror  r0 r0 1H
 bpl  N2
 mov  r0 ffffffc8H
 ld   r0 r0 0
 st   r0 sp 12
 ld   r0 sp 8
 ld   r1 sp 12
 add  r0 r0 r1
 ror  r0 r0 8H
 st   r0 sp 8
 ld   r0 sp 16
 bne  N1
 ld   r0 sp 8
 ld   r1 sp 4
 st   r0 r1 0
 ld   lr sp 0
 add  sp sp 14H
  br  lr

LoadFromLine:
 sub  sp sp 10H
 st   lr sp 0
 add  r0 sp 4H
  br. RecInt
L1:
 ld   r0 sp 4
 sub  r0 r0 0H
 ble  L3
 add  r0 sp 8H
  br. RecInt
L2:
 add  r0 sp cH
  br. RecInt
 ld   r0 sp 8
 ld   r1 sp 12
 st   r1 r0 0
 ld   r0 sp 8
 add  r0 r0 4H
 st   r0 sp 8
 ld   r0 sp 4
 sub  r0 r0 4H
 st   r0 sp 4
 ld   r0 sp 4
 bne  L2
 add  r0 sp 4H
  br. RecInt
  br  L1
L3:
 ld   lr sp 0
 add  sp sp 10H
  br  lr

SPIIdle:
 sub  sp sp 8H
 st   lr sp 0
 st   r0 sp 4
 mov  r0 ffffffd4H
 mov  r1 0H
 st   r1 r0 0
I1:
 ld   r0 sp 4
 sub  r0 r0 0H
 ble  I3
 ld   r0 sp 4
 sub  r0 r0 1H
 st   r0 sp 4
 mov  r0 ffffffd0H
 mov  r1 ffffffffH
 st   r1 r0 0
I2:
 mov  r0 ffffffd4H
 ld   r0 r0 0
 ror  r0 r0 1H
 bpl  I2
  br  I1
I3:
 ld   lr sp 0
 add  sp sp 8H
  br  lr

SPI:
 sub  sp sp 8H
 st   lr sp 0
 st   r0 sp 4
 mov  r0 ffffffd4H
 mov  r1 1H
 st   r1 r0 0
 mov  r0 ffffffd0H
 ld   r1 sp 4
 st   r1 r0 0
P1:
 mov  r0 ffffffd4H
 ld   r0 r0 0
 ror  r0 r0 1H
bpl  P1
 ld   lr sp 0
 add  sp sp 8H
  br  lr

SPICmd:
 sub  sp sp 18H
 st   lr sp 0
 st   r0 sp 4
 st   r1 sp 8
C1:
 mov  r0 1H
  br. SPIIdle
 mov  r0 ffffffd0H
 ld   r0 r0 0
 st   r0 sp 16
 ld   r0 sp 16
 sub  r0 r0 ffH
 bne  C1
C2:
 mov  r0 ffH
  br. SPI
 mov  r0 ffffffd0H
 ld   r0 r0 0
 st   r0 sp 16
 ld   r0 sp 16
 sub  r0 r0 ffH
 bne  C2
 ld   r0 sp 4
 sub  r0 r0 8H
 bne  C3
 mov  r0 87H
 st   r0 sp 20
  br  C5
C3:
 ld   r0 sp 4
 bne  C4
 mov  r0 95H
 st   r0 sp 20
  br  C5
C4:
 mov  r0 ffH
 st   r0 sp 20
C5:
 ld   r0 sp 4
 and  r0 r0 3fH
 add  r0 r0 40H
  br. SPI
 mov  r0 18H
C6:
 sub  r1 r0 0H
 blt  C7
 st   r0 sp 12
 ld   r0 sp 8
 ld   r1 sp 12
 ror  r0 r0 r1
  br. SPI
 ld   r0 sp 12
 add  r0 r0 fffffff8H
  br  C6
C7:
 ld   r0 sp 20
  br. SPI
 mov  r0 20H
 st   r0 sp 12
C8:
 mov  r0 ffH
  br. SPI
 mov  r0 ffffffd0H
 ld   r0 r0 0
 st   r0 sp 16
 ld   r0 sp 12
 sub  r0 r0 1H
 st   r0 sp 12
 ld   r0 sp 16
 sub  r0 r0 80H
 blt  C9
 ld   r0 sp 12
 bne  C8
C9:
 ld   lr sp 0
 add  sp sp 18H
  br  lr

InitSPI:
 sub  sp sp cH
 st   lr sp 0
 mov  r0 9H
  br. SPIIdle
 mov  r0 0H
 mov  r1 0H
  br. SPICmd
 mov  r0 8H
 mov  r1 1aaH
  br. SPICmd
 mov  r0 ffffffffH
  br. SPI
 mov  r0 ffffffffH
  br. SPI
 mov  r0 ffffffffH
  br. SPI
X1:
 mov  r0 37H
 mov  r1 0H
  br. SPICmd
 mov  r0 29H
 mov  r1 1H
 lsl  r1 r1 1eH
  br. SPICmd
 mov  r0 ffffffd0H
 ld   r0 r0 0
 st   r0 sp 4
 mov  r0 ffffffffH
  br. SPI
 mov  r0 ffffffffH
  br. SPI
 mov  r0 ffffffffH
  br. SPI
 mov  r0 2710H
  br. SPIIdle
 ld   r0 sp 4
 bne  X1
 mov  r0 10H
 mov  r1 200H
  br. SPICmd
 mov  r0 1H
  br. SPIIdle
 ld   lr sp 0
 add  sp sp cH
  br  lr

SDShift:
 sub  sp sp cH
 st   lr sp 0
 st   r0 sp 4
 mov  r0 3aH
 mov  r1 0H
  br. SPICmd
 mov  r0 ffffffd0H
 ld   r0 r0 0
 st   r0 sp 8
 mov  r0 ffffffffH
  br. SPI
 ld   r0 sp 8
 bne  S1
 mov  r0 ffffffd0H
 ld   r0 r0 0
 ror  r0 r0 7H
 bmi  S2
S1:
 ld   r0 sp 4
 ld   r0 r0 0
 lsl  r0 r0 9H
 ld   r1 sp 4
 st   r0 r1 0
S2:
 mov  r0 ffffffffH
  br. SPI
 mov  r0 ffffffffH
  br. SPI
 mov  r0 1H
  br. SPIIdle
 ld   lr sp 0
 add  sp sp cH
  br  lr

ReadSD:
 sub  sp sp 14H
 st   lr sp 0
 st   r0 sp 4
 st   r1 sp 8
 add  r0 sp 4H
  br. SDShift
 mov  r0 11H
 ld   r1 sp 4
  br. SPICmd
 mov  r0 0H
 st   r0 sp 12
Y1:
 mov  r0 ffffffffH
  br. SPI
 mov  r0 ffffffd0H
 ld   r0 r0 0
 st   r0 sp 16
 ld   r0 sp 12
 add  r0 r0 1H
 st   r0 sp 12
 ld   r0 sp 16
 sub  r0 r0 feH
 bne  Y1
 mov  r0 ffffffd4H
 mov  r1 5H
 st   r1 r0 0
 mov  r0 0H
Y2:
 sub  r1 r0 1fcH
 bgt  Y4
 st   r0 sp 12
 mov  r0 ffffffd0H
 mov  r1 ffffffffH
 st   r1 r0 0
Y3:
 mov  r0 ffffffd4H
 ld   r0 r0 0
 ror  r0 r0 1H
 bpl  Y3
 mov  r0 ffffffd0H
 ld   r0 r0 0
 st   r0 sp 16
 ld   r0 sp 8
 ld   r1 sp 16
 st   r1 r0 0
 ld   r0 sp 8
 add  r0 r0 4H
 st   r0 sp 8
 ld   r0 sp 12
 add  r0 r0 4H
  br  Y2
Y4:
 mov  r0 ffH
  br. SPI
 mov  r0 ffH
  br. SPI
 mov  r0 1H
  br. SPIIdle
 ld   lr sp 0
 add  sp sp 14H
  br  lr

LoadFromDisk:
 sub  sp sp 14H
 st   lr sp 0
 mov' r0 8H
 ior  r0 r0 4H
 st   r0 sp 4
 ld   r0 sp 4
 mov  r1 0H
  br. ReadSD
 mov  r0 10H
 ld   r0 r0 0
 st   r0 sp 16
 ld   r0 sp 4
 add  r0 r0 1H
 st   r0 sp 4
 mov  r0 200H
 st   r0 sp 8
D1:
 ld   r0 sp 8
 ld   r1 sp 16
 sub  r0 r0 r1
 bge  D2
 ld   r0 sp 4
 ld   r1 sp 8
  br. ReadSD
 ld   r0 sp 4
 add  r0 r0 1H
 st   r0 sp 4
 ld   r0 sp 8
 add  r0 r0 200H
 st   r0 sp 8
  br  D1
D2:
 ld   lr sp 0
 add  sp sp 14H
  br  lr

Begin:
 mov  sb 0H
 mov  sp ffffffc0H
 mov' sp 8H
 mov  mt 20H
 mov  r0 lr
 sub  r0 r0 0H
 bne  B3
 mov  r0 80H
 mov  r1 ffffffc4H
 st   r0 r1 0
  br. InitSPI
 mov  r0 ffffffc4H
 ld   r0 r0 0
 ror  r0 r0 1H
 bpl  B1
 mov  r0 81H
 mov  r1 ffffffc4H
 st   r0 r1 0
  br. LoadFromLine
  br  B2
B1:
 mov  r0 82H
 mov  r1 ffffffc4H
 st   r0 r1 0
  br. LoadFromDisk
B2:
  br  B4
B3:
 mov  r0 ffffffc4H
 ld   r0 r0 0
 ror  r0 r0 1H
 bpl  B4
 mov  r0 81H
 mov  r1 ffffffc4H
 st   r0 r1 0
  br. LoadFromLine
B4:
 mov  r0 cH
 mov' r1 eH
 ior  r1 r1 7ef0H
 st   r1 r0 0
 mov  r0 18H
 mov' r1 8H
 st   r1 r0 0
 mov  r0 84H
 mov  r1 ffffffc4H
 st   r0 r1 0


 mov  r0 ffffffe4H
 ld   r0 r0 0

 mov' r0 5369H
 ior  r0 r0 7A66H
 mov' r1 eH
 ior  r1 r1 7f00H
 st   r0 r1 0
 mov  r2 ffffffe0H
 mov  r3 ffffffc4H
 ld   r0 r2 0
 st   r0 r3 0
 div  r0 r0 1000H
 div  r0 r0 100H
 st   r0 r3 0
 st   r0 r1 4

 ld   r0 r2 0
 div  r0 r0 100H
 and  r0 r0 fffH
 st   r0 r1 8
 st   r0 r3 0

 mov  r0 0H
  br  r0

