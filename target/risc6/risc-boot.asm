 BR  Begin
 MOV r0 0H 
 MOV r0 0H 
 MOV r0 0H 
 MOV r0 0H 
 MOV r0 0H 
 MOV r0 0H 
 MOV r0 0H 

RecInt:
 SUB' r14 r14 14H
 STR r15 r14 0H 
 STR r0 r14 4H 
 MOV r0 0H 
 STR r0 r14 8H 
 MOV r0 4H 
 STR r0 r14 10H 
N1:
 LDR r0 r14 10H 
 SUB' r0 r0 1H 
 STR r0 r14 10H 
N2:
 MOV r0 ffccH 
 LDR r0 r0 0H 
 ROR r0 r0 1H 
 BPL N2 
 MOV r0 ffc8H 
 LDR r0 r0 0H 
 STR r0 r14 cH 
 LDR r0 r14 8H 
 LDR r1 r14 cH 
 ADD r0 r0 1H 
 ROR r0 r0 8H 
 STR r0 r14 8H 
 LDR r0 r14 10H 
 BNE N1 
 LDR r0 r14 8H 
 LDR r1 r14 4H 
 STR r0 r1 0H 
 LDR r15 r14 0H 
 ADD r14 r14 14H 
 RTS 

LoadFromLine:
 SUB' r14 r14 10H
 STR r15 r14 0H 
 ADD r0 r14 4H 
 JSR RecInt
L1:
 LDR r0 r14 4H 
 SUB' r0 r0 0H 
 BLE L3
 ADD r0 r14 8H 
 JSR RecInt 
L2:
 ADD r0 r14 cH 
 JSR RecInt 
 LDR r0 r14 8H 
 LDR r1 r14 cH 
 STR r1 r0 0H 
 LDR r0 r14 8H 
 ADD r0 r0 4H 
 STR r0 r14 8H 
 LDR r0 r14 4H 
 SUB' r0 r0 4H 
 STR r0 r14 4H 
 LDR r0 r14 4H 
 BNE L2
 ADD r0 r14 4H 
 JSR RecInt
 BR  L1
L3:
 LDR r15 r14 0H 
 ADD r14 r14 10H 
 RTS 

SPIIdle:
 SUB' r14 r14 8H
 STR r15 r14 0H 
 STR r0 r14 4H 
 MOV r0 ffd4H 
 MOV r1 0H 
 STR r1 r0 0H 
I1:
 LDR r0 r14 4H 
 SUB' r0 r0 0H 
 BLE I3 
 LDR r0 r14 4H 
 SUB' r0 r0 1H 
 STR r0 r14 4H 
 MOV r0 ffd0H 
 MOV r1 ffffH 
 STR r1 r0 0H 
I2:
 MOV' r0 ffd4H 
 LDR r0 r0 0H 
 ROR r0 r0 1H 
 BPL I2 
 BR  I1
I3:
 LDR r15 r14 0H 
 ADD r14 r14 8H 
 RTS 

SPI:
 SUB' r14 r14 8H
 STR r15 r14 0H 
 STR r0 r14 4H 
 MOV r0 ffd4H 
 MOV r1 1H 
 STR r1 r0 0H 
 MOV r0 ffd0H 
 LDR r1 r14 4H 
 STR r1 r0 0H
P1: 
 MOV r0 ffd4H 
 LDR r0 r0 0H 
 ROR r0 r0 1H 
 BPL P1
 LDR r15 r14 0 
 ADD r14 r14 8H 
 RTS

SPICmd:
 SUB' r14 r14 18H
 STR r15 r14 0H 
 STR r0 r14 4H 
 STR r1 r14 8H 
C1:
 MOV r0 1H 
 JSR SPIIdle
 MOV r0 ffd0H 
 LDR r0 r0 0H 
 STR r0 r14 10H 
 LDR r0 r14 10H 
 SUB' r0 r0 ffH 
 BNE C1 
C2:
 MOV r0 ffH 
 JSR SPI
 MOV r0 ffd0H 
 LDR r0 r0 0H 
 STR r0 r14 10H 
 LDR r0 r14 10H 
 SUB' r0 r0 ffH 
 BNE C2 
 LDR r0 r14 4H 
 SUB' r0 r0 8H 
 BNE C3 
 MOV r0 87H 
 STR r0 r14 14H 
 BR  C5 
C3:
 LDR r0 r14 4H 
 BNE C4 
 MOV r0 95H 
 STR r0 r14 20H 
 BR  C5 
C4:
 MOV r0 ffH 
 STR r0 r14 20H 
C5:
 LDR r0 r14 4H 
 AND r0 r0 3fH 
 ADD r0 r0 40H 
 JSR SPI
 MOV r0 18H 
C6:
 SUB' r1 r0 0H 
 BLT C7 
 STR r0 r14 cH 
 LDR r0 r14 8H 
 LDR r1 r14 cH 
 ROR r0 r0 1H 
 JSR SPI
 LDR r0 r14 cH 
 ADD r0 r0 fff8H 
 BR  C6
c7:
 LDR r0 r14 14H 
 JSR SPI
 MOV r0 20H 
 STR r0 r14 cH 
C8:
 MOV r0 ffH 
 JSR SPI
 MOV r0 ffd0H 
 LDR r0 r0 0H 
 STR r0 r14 10H 
 LDR r0 r14 cH 
 SUB' r0 r0 1H 
 STR r0 r14 cH 
 LDR r0 r14 10H 
 SUB' r0 r0 80H 
 BLT C9 
 LDR r0 r14 cH 
 BNE C8 
C9:
 LDR r15 r14 0H 
 ADD r14 r14 18H 
 RTS

InitSPI:
 SUB' r14 r14 cH 
 STR r15 r14 0H 
 MOV r0 9H 
 JSR SPIIdle
 MOV r0 0H 
 MOV r1 0H 
 JSR SPICmd
 MOV r0 8H 
 MOV r1 1aaH 
 JSR SPICmd
 MOV r0 ffffH 
 JSR SPI
 MOV r0 ffffH 
 JSR SPI
 MOV r0 ffffH 
 JSR SPI 
X1:
 MOV r0 37H 
 MOV r1 0H 
 JSR SPICmd
 MOV r0 29H 
 MOV r1 1H 
 LSL r1 r1 1eH 
 JSR SPICmd
 MOV r0 ffd0H 
 LDR r0 r0 0H 
 STR r0 r14 4H 
 MOV r0 ffffH 
 JSR SPI
 MOV r0 ffffH 
 JSR SPI
 MOV r0 ffffH 
 JSR SPI
 MOV r0 2710H 
 JSR SPIIdle
 LDR r0 r14 4H 
 BNE X1
 MOV r0 10H 
 MOV r1 200H 
 JSR SPICmd
 MOV r0 1H 
 JSR SPIIdle
 LDR r15 r14 0H 
 ADD r14 r14 cH 
 RTS

SDShift:
 SUB' r14 r14 cH
 STR r15 r14 0H 
 STR r0 r14 4H 
 MOV r0 3aH 
 MOV r1 0H 
 JSR SPICmd 
 MOV r0 ffd0H 
 LDR r0 r0 0H 
 STR r0 r14 8H 
 MOV r0 ffffH 
 JSR SPI
 LDR r0 r14 8H 
 BNE S1
 MOV r0 ffd0H 
 LDR r0 r0 0H 
 ROR r0 r0 7H 
 BMI S2
S1:
 LDR r0 r14 4H 
 LDR r0 r0 0H 
 LSL r0 r0 9H 
 LDR r1 r14 4H 
 STR r0 r1 0H 
S2:
 MOV r0 ffffH 
 JSR SPI
 MOV r0 ffffH 
 JSR SPI
 MOV r0 1H 
 JSR SPIIdle
 LDR r15 r14 0H 
 ADD r14 r14 cH 
 RTS 

ReadSD:
 SUB' r14 r14 14H
 STR r15 r14 0H 
 STR r0 r14 4H 
 STR r1 r14 8H 
 ADD r0 r14 4H 
 JSR SDShift 
 MOV r0 11H 
 LDR r1 r14 4H 
 JSR SPICmd
 MOV r0 0H 
 STR r0 r14 cH 
Y1:
 MOV r0 ffffH 
 JSR SPI
 MOV r0 ffd0H 
 LDR r0 r0 0H 
 STR r0 r14 10H 
 LDR r0 r14 cH
 ADD r0 r0 1H 
 STR r0 r14 cH 
 LDR r0 r14 10H 
 SUB' r0 r0 feH 
 BNE Y1
 MOV r0 ffd4H 
 MOV r1 5H 
 STR r1 r0 0H 
 MOV r0 0H 
Y2:
 SUB' r1 r0 1fcH 
 BGT Y4
 STR r0 r14 cH
 MOV r0 ffd0H 
 MOV r1 ffffH 
 STR r1 r0 0H 
Y3:
 MOV r0 ffd4H 
 LDR r0 r0 0H 
 ROR r0 r0 1H 
 BPL Y3
 MOV r0 ffd0H 
 LDR r0 r0 0H 
 STR r0 r14 10H 
 LDR r0 r14 8H 
 LDR r1 r14 10H 
 STR r1 r0 0H 
 LDR r0 r14 8H 
 ADD r0 r0 4H 
 STR r0 r14 8H 
 LDR r0 r14 cH 
 ADD r0 r0 4H 
 BR  Y2
Y4:
 MOV r0 ffH 
 JSR SPI
 MOV r0 ffH 
 JSR SPI
 MOV r0 1H 
 JSR SPIIdle
 LDR r15 r14 0H 
 ADD r14 r14 14H 
 RTS

LoadFromDisk:
 SUB' r14 r14 14H 
 STR r15 r14 0H 
 MOV' r0 8H 
 IOR r0 r0 4H 
 STR r0 r14 4H 
 LDR r0 r14 4H 
 MOV r1 0H 
 JSR ReadSD 
 MOV r0 10H 
 LDR r0 r0 0H 
 STR r0 r14 10H 
 LDR r0 r14 4H 
 ADD r0 r0 1H 
 STR r0 r14 4H 
 MOV r0 200H 
 STR r0 r14 8H 
D1:
 LDR r0 r14 8H 
 LDR r1 r14 10H 
 SUB r0 r0 r1 
 BGE D2
 LDR r0 r14 4H 
 LDR r1 r14 8H 
 JSR ReadSD
 LDR r0 r14 4H 
 ADD r0 r0 1H
 STR r0 r14 4H 
 LDR r0 r14 8H 
 ADD r0 r0 200H 
 STR r0 r14 8H 
 BR  D1
D2:
 LDR r15 r14 0H 
 ADD r14 r14 14H
 RTS
 
Begin:
 MOV r13 0H 
 MOV r14 ffc0H 
 MOV' r14 8H 
 MOV r12 20H 
 MOV r0 r15 
 SUB' r0 r0 0H 

Loop:
 BR Loop

 BNE B3
 MOV r0 80H 
 MOV r1 ffc4H 
 STR r0 r1 0H 
 JSR InitSPI
 MOV r0 ffc4H 
 LDR r0 r0 0H 
 ROR r0 r0 1H 
 BPL B1
 MOV r0 81H 
 MOV r1 ffc4H 
 STR r0 r1 0H 
 JSR LoadFromLine
 BR  B2
B1:
 MOV r0 82H 
 MOV r1 ffc4H 
 STR r0 r1 0H 
 JSR LoadFromDisk
B2:
 BR  B4
B3: 
 MOV r0 ffc4H 
 LDR r0 r0 0H 
 ROR r0 r0 1H 
 BPL B4
 MOV r0 81H 
 MOV r1 ffc4H 
 STR r0 r1 0H 
 JSR LoadFromLine
B4:
 MOV r0 cH 
 MOV' r1 eH 
 IOR r1 r1 7ef0H 
 STR r1 r0 0H 
 MOV r0 18H 
 MOV' r1 8H 
 STR r1 r0 0H 
 MOV r0 84H 
 MOV r1 ffc4H 
 STR r0 r1 0H 
 MOV r0 0H 
 BR  r0 
