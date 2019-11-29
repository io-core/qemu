0xffffe000:  e7000151 br   Begin
0xffffe004:  00000000 mov  r0 r0
0xffffe008:  00000000 mov  r0 r0
0xffffe00c:  00000000 mov  r0 r0
0xffffe010:  00000000 mov  r0 r0
0xffffe014:  00000000 mov  r0 r0
0xffffe018:  00000000 mov  r0 r0
0xffffe01c:  00000000 mov  r0 r0

                     RecInt:
0xffffe020:  4ee90014 sub  sp sp 14H
0xffffe024:  afe00000 st   lr sp 0
0xffffe028:  a0e00004 st   r0 sp 4
0xffffe02c:  40000000 mov  r0 0H
0xffffe030:  a0e00008 st   r0 sp 8
0xffffe034:  40000004 mov  r0 4H
0xffffe038:  a0e00010 st   r0 sp 16
                     N1:
0xffffe03c:  80e00010 ld   r0 sp 16
0xffffe040:  40090001 sub  r0 r0 1H
0xffffe044:  a0e00010 st   r0 sp 16
                     N2:
0xffffe048:  5000ffcc mov  r0 ffffffccH
0xffffe04c:  80000000 ld   r0 r0 0
0xffffe050:  40030001 ror  r0 r0 1H
0xffffe054:  e8fffffc bpl  N2
0xffffe058:  5000ffc8 mov  r0 ffffffc8H
0xffffe05c:  80000000 ld   r0 r0 0
0xffffe060:  a0e0000c st   r0 sp 12
0xffffe064:  80e00008 ld   r0 sp 8
0xffffe068:  81e0000c ld   r1 sp 12
0xffffe06c:  00080001 add  r0 r0 r1
0xffffe070:  40030008 ror  r0 r0 8H
0xffffe074:  a0e00008 st   r0 sp 8
0xffffe078:  80e00010 ld   r0 sp 16
0xffffe07c:  e9ffffef bne  N1
0xffffe080:  80e00008 ld   r0 sp 8
0xffffe084:  81e00004 ld   r1 sp 4
0xffffe088:  a0100000 st   r0 r1 0
0xffffe08c:  8fe00000 ld   lr sp 0
0xffffe090:  4ee80014 add  sp sp 14H
0xffffe094:  c700000f br   lr

                     LoadFromLine:
0xffffe098:  4ee90010 sub  sp sp 10H
0xffffe09c:  afe00000 st   lr sp 0
0xffffe0a0:  40e80004 add  r0 sp 4H
0xffffe0a4:  f7ffffde br . RecInt
                     L1:
0xffffe0a8:  80e00004 ld   r0 sp 4
0xffffe0ac:  40090000 sub  r0 r0 0H
0xffffe0b0:  e6000012 ble  L3
0xffffe0b4:  40e80008 add  r0 sp 8H
0xffffe0b8:  f7ffffd9 br . RecInt
                     L2:
0xffffe0bc:  40e8000c add  r0 sp cH
0xffffe0c0:  f7ffffd7 br . RecInt
0xffffe0c4:  80e00008 ld   r0 sp 8
0xffffe0c8:  81e0000c ld   r1 sp 12
0xffffe0cc:  a1000000 st   r1 r0 0
0xffffe0d0:  80e00008 ld   r0 sp 8
0xffffe0d4:  40080004 add  r0 r0 4H
0xffffe0d8:  a0e00008 st   r0 sp 8
0xffffe0dc:  80e00004 ld   r0 sp 4
0xffffe0e0:  40090004 sub  r0 r0 4H
0xffffe0e4:  a0e00004 st   r0 sp 4
0xffffe0e8:  80e00004 ld   r0 sp 4
0xffffe0ec:  e9fffff3 bne  L2
0xffffe0f0:  40e80004 add  r0 sp 4H
0xffffe0f4:  f7ffffca br . RecInt
0xffffe0f8:  e7ffffeb br   L1
                     L3:
0xffffe0fc:  8fe00000 ld   lr sp 0
0xffffe100:  4ee80010 add  sp sp 10H
0xffffe104:  c700000f br   lr

                     SPIIdle:
0xffffe108:  4ee90008 sub  sp sp 8H
0xffffe10c:  afe00000 st   lr sp 0
0xffffe110:  a0e00004 st   r0 sp 4
0xffffe114:  5000ffd4 mov  r0 ffffffd4H
0xffffe118:  41000000 mov  r1 0H
0xffffe11c:  a1000000 st   r1 r0 0
                     I1:
0xffffe120:  80e00004 ld   r0 sp 4
0xffffe124:  40090000 sub  r0 r0 0H
0xffffe128:  e600000b ble  I3
0xffffe12c:  80e00004 ld   r0 sp 4
0xffffe130:  40090001 sub  r0 r0 1H
0xffffe134:  a0e00004 st   r0 sp 4
0xffffe138:  5000ffd0 mov  r0 ffffffd0H
0xffffe13c:  5100ffff mov  r1 ffffffffH
0xffffe140:  a1000000 st   r1 r0 0
                     I2:
0xffffe144:  5000ffd4 mov  r0 ffffffd4H
0xffffe148:  80000000 ld   r0 r0 0
0xffffe14c:  40030001 ror  r0 r0 1H
0xffffe150:  e8fffffc bpl  I2
0xffffe154:  e7fffff2 br   I1
                     I3:
0xffffe158:  8fe00000 ld   lr sp 0
0xffffe15c:  4ee80008 add  sp sp 8H
0xffffe160:  c700000f br   lr

                     SPI:
0xffffe164:  4ee90008 sub  sp sp 8H
0xffffe168:  afe00000 st   lr sp 0
0xffffe16c:  a0e00004 st   r0 sp 4
0xffffe170:  5000ffd4 mov  r0 ffffffd4H
0xffffe174:  41000001 mov  r1 1H
0xffffe178:  a1000000 st   r1 r0 0
0xffffe17c:  5000ffd0 mov  r0 ffffffd0H
0xffffe180:  81e00004 ld   r1 sp 4
0xffffe184:  a1000000 st   r1 r0 0
                     P1:
0xffffe188:  5000ffd4 mov  r0 ffffffd4H
0xffffe18c:  80000000 ld   r0 r0 0
0xffffe190:  40030001 ror  r0 r0 1H
0xffffe194:  e8fffffc bpl  P1
0xffffe198:  8fe00000 ld   lr sp 0
0xffffe19c:  4ee80008 add  sp sp 8H
0xffffe1a0:  c700000f br   lr

                     SPICmd:
0xffffe1a4:  4ee90018 sub  sp sp 18H
0xffffe1a8:  afe00000 st   lr sp 0
0xffffe1ac:  a0e00004 st   r0 sp 4
0xffffe1b0:  a1e00008 st   r1 sp 8
                     C1:
0xffffe1b4:  40000001 mov  r0 1H
0xffffe1b8:  f7ffffd3 br . SPIIdle
0xffffe1bc:  5000ff00 mov  r0 ffffff00H
0xffffe1c0:  80000000 ld   r0 r0 0
0xffffe1c4:  a0e00010 st   r0 sp 16
0xffffe1c8:  80e00010 ld   r0 sp 16
0xffffe1cc:  400900ff sub  r0 r0 ffH
0xffffe1d0:  e9fffff8 bne  C1
                     C2:
0xffffe1d4:  400000ff mov  r0 ffH
0xffffe1d8:  f7ffffe2 br . SPI
0xffffe1dc:  5000ffd0 mov  r0 ffffffd0H
0xffffe1e0:  80000000 ld   r0 r0 0
0xffffe1e4:  a0e00010 st   r0 sp 16
0xffffe1e8:  80e00010 ld   r0 sp 16
0xffffe1ec:  400900ff sub  r0 r0 ffH
0xffffe1f0:  e9fffff8 bne  C2
0xffffe1f4:  80e00004 ld   r0 sp 4
0xffffe1f8:  40090008 sub  r0 r0 8H
0xffffe1fc:  e9000003 bne  C3
0xffffe200:  40000087 mov  r0 87H
0xffffe204:  a0e00014 st   r0 sp 20
0xffffe208:  e7000007 br   C5
                     C3:
0xffffe20c:  80e00004 ld   r0 sp 4
0xffffe210:  e9000003 bne  C4
0xffffe214:  40000095 mov  r0 95H
0xffffe218:  a0e00014 st   r0 sp 20
0xffffe21c:  e7000002 br   C5
                     C4:
0xffffe220:  400000ff mov  r0 ffH
0xffffe224:  a0e00014 st   r0 sp 20
                     C5:
0xffffe228:  80e00004 ld   r0 sp 4
0xffffe22c:  4004003f and  r0 r0 3fH
0xffffe230:  40080040 add  r0 r0 40H
0xffffe234:  f7ffffcb br . SPI
0xffffe238:  40000018 mov  r0 18H
                     C6:
0xffffe23c:  41090000 sub  r1 r0 0H
0xffffe240:  e5000008 blt  C7
0xffffe244:  a0e0000c st   r0 sp 12
0xffffe248:  80e00008 ld   r0 sp 8
0xffffe24c:  81e0000c ld   r1 sp 12
0xffffe250:  00030001 ror  r0 r0 r1
0xffffe254:  f7ffffc3 br . SPI
0xffffe258:  80e0000c ld   r0 sp 12
0xffffe25c:  5008fff8 add  r0 r0 fffffff8H
0xffffe260:  e7fffff6 br   C6
                     C7:
0xffffe264:  80e00014 ld   r0 sp 20
0xffffe268:  f7ffffbe br . SPI
0xffffe26c:  40000020 mov  r0 20H
0xffffe270:  a0e0000c st   r0 sp 12
                     C8:
0xffffe274:  400000ff mov  r0 ffH
0xffffe278:  f7ffffba br . SPI
0xffffe27c:  5000ffd0 mov  r0 ffffffd0H
0xffffe280:  80000000 ld   r0 r0 0
0xffffe284:  a0e00010 st   r0 sp 16
0xffffe288:  80e0000c ld   r0 sp 12
0xffffe28c:  40090001 sub  r0 r0 1H
0xffffe290:  a0e0000c st   r0 sp 12
0xffffe294:  80e00010 ld   r0 sp 16
0xffffe298:  40090080 sub  r0 r0 80H
0xffffe29c:  e5000002 blt  C9
0xffffe2a0:  80e0000c ld   r0 sp 12
0xffffe2a4:  e9fffff3 bne  C8
                     C9:
0xffffe2a8:  8fe00000 ld   lr sp 0
0xffffe2ac:  4ee80018 add  sp sp 18H
0xffffe2b0:  c700000f br   lr

                     InitSPI:
0xffffe2b4:  4ee9000c sub  sp sp cH
0xffffe2b8:  afe00000 st   lr sp 0
0xffffe2bc:  40000009 mov  r0 9H
0xffffe2c0:  f7ffff91 br . SPIIdle
0xffffe2c4:  40000000 mov  r0 0H
0xffffe2c8:  41000000 mov  r1 0H
0xffffe2cc:  f7ffffb5 br . SPICmd
0xffffe2d0:  40000008 mov  r0 8H
0xffffe2d4:  410001aa mov  r1 1aaH
0xffffe2d8:  f7ffffb2 br . SPICmd
0xffffe2dc:  5000ffff mov  r0 ffffffffH
0xffffe2e0:  f7ffffa0 br . SPI
0xffffe2e4:  5000ffff mov  r0 ffffffffH
0xffffe2e8:  f7ffff9e br . SPI
0xffffe2ec:  5000ffff mov  r0 ffffffffH
0xffffe2f0:  f7ffff9c br . SPI
                     X1:
0xffffe2f4:  40000037 mov  r0 37H
0xffffe2f8:  41000000 mov  r1 0H
0xffffe2fc:  f7ffffa9 br . SPICmd
0xffffe300:  40000029 mov  r0 29H
0xffffe304:  41000001 mov  r1 1H
0xffffe308:  4111001e lsl  r1 r1 1eH
0xffffe30c:  f7ffffa5 br . SPICmd
0xffffe310:  5000ffd0 mov  r0 ffffffd0H
0xffffe314:  80000000 ld   r0 r0 0
0xffffe318:  a0e00004 st   r0 sp 4
0xffffe31c:  5000ffff mov  r0 ffffffffH
0xffffe320:  f7ffff90 br . SPI
0xffffe324:  5000ffff mov  r0 ffffffffH
0xffffe328:  f7ffff8e br . SPI
0xffffe32c:  5000ffff mov  r0 ffffffffH
0xffffe330:  f7ffff8c br . SPI
0xffffe334:  40002710 mov  r0 2710H
0xffffe338:  f7ffff73 br . SPIIdle
0xffffe33c:  80e00004 ld   r0 sp 4
0xffffe340:  e9ffffec bne  X1
0xffffe344:  40000010 mov  r0 10H
0xffffe348:  41000200 mov  r1 200H
0xffffe34c:  f7ffff95 br . SPICmd
0xffffe350:  40000001 mov  r0 1H
0xffffe354:  f7ffff6c br . SPIIdle
0xffffe358:  8fe00000 ld   lr sp 0
0xffffe35c:  4ee8000c add  sp sp cH
0xffffe360:  c700000f br   lr

                     SDShift:
0xffffe364:  4ee9000c sub  sp sp cH
0xffffe368:  afe00000 st   lr sp 0
0xffffe36c:  a0e00004 st   r0 sp 4
0xffffe370:  4000003a mov  r0 3aH
0xffffe374:  41000000 mov  r1 0H
0xffffe378:  f7ffff8a br . SPICmd
0xffffe37c:  5000ffd0 mov  r0 ffffffd0H
0xffffe380:  80000000 ld   r0 r0 0
0xffffe384:  a0e00008 st   r0 sp 8
0xffffe388:  5000ffff mov  r0 ffffffffH
0xffffe38c:  f7ffff75 br . SPI
0xffffe390:  80e00008 ld   r0 sp 8
0xffffe394:  e9000004 bne  S1
0xffffe398:  5000ffd0 mov  r0 ffffffd0H
0xffffe39c:  80000000 ld   r0 r0 0
0xffffe3a0:  40030007 ror  r0 r0 7H
0xffffe3a4:  e0000005 bmi  S2
                     S1:
0xffffe3a8:  80e00004 ld   r0 sp 4
0xffffe3ac:  80000000 ld   r0 r0 0
0xffffe3b0:  40010009 lsl  r0 r0 9H
0xffffe3b4:  81e00004 ld   r1 sp 4
0xffffe3b8:  a0100000 st   r0 r1 0
                     S2:
0xffffe3bc:  5000ffff mov  r0 ffffffffH
0xffffe3c0:  f7ffff68 br . SPI
0xffffe3c4:  5000ffff mov  r0 ffffffffH
0xffffe3c8:  f7ffff66 br . SPI
0xffffe3cc:  40000001 mov  r0 1H
0xffffe3d0:  f7ffff4d br . SPIIdle
0xffffe3d4:  8fe00000 ld   lr sp 0
0xffffe3d8:  4ee8000c add  sp sp cH
0xffffe3dc:  c700000f br   lr

                     ReadSD:
0xffffe3e0:  4ee90014 sub  sp sp 14H
0xffffe3e4:  afe00000 st   lr sp 0
0xffffe3e8:  a0e00004 st   r0 sp 4
0xffffe3ec:  a1e00008 st   r1 sp 8
0xffffe3f0:  40e80004 add  r0 sp 4H
0xffffe3f4:  f7ffffdb br . SDShift
0xffffe3f8:  40000011 mov  r0 11H
0xffffe3fc:  81e00004 ld   r1 sp 4
0xffffe400:  f7ffff68 br . SPICmd
0xffffe404:  40000000 mov  r0 0H
0xffffe408:  a0e0000c st   r0 sp 12
                     Y1:
0xffffe40c:  5000ffff mov  r0 ffffffffH
0xffffe410:  f7ffff54 br . SPI
0xffffe414:  5000ffd0 mov  r0 ffffffd0H
0xffffe418:  80000000 ld   r0 r0 0
0xffffe41c:  a0e00010 st   r0 sp 16
0xffffe420:  80e0000c ld   r0 sp 12
0xffffe424:  40080001 add  r0 r0 1H
0xffffe428:  a0e0000c st   r0 sp 12
0xffffe42c:  80e00010 ld   r0 sp 16
0xffffe430:  400900fe sub  r0 r0 feH
0xffffe434:  e9fffff5 bne  Y1
0xffffe438:  5000ffd4 mov  r0 ffffffd4H
0xffffe43c:  41000005 mov  r1 5H
0xffffe440:  a1000000 st   r1 r0 0
0xffffe444:  40000000 mov  r0 0H
                     Y2:
0xffffe448:  410901fc sub  r1 r0 1fcH
0xffffe44c:  ee000014 bgt  Y4
0xffffe450:  a0e0000c st   r0 sp 12
0xffffe454:  5000ffd0 mov  r0 ffffffd0H
0xffffe458:  5100ffff mov  r1 ffffffffH
0xffffe45c:  a1000000 st   r1 r0 0
                     Y3:
0xffffe460:  5000ffd4 mov  r0 ffffffd4H
0xffffe464:  80000000 ld   r0 r0 0
0xffffe468:  40030001 ror  r0 r0 1H
0xffffe46c:  e8fffffc bpl  Y3
0xffffe470:  5000ffd0 mov  r0 ffffffd0H
0xffffe474:  80000000 ld   r0 r0 0
0xffffe478:  a0e00010 st   r0 sp 16
0xffffe47c:  80e00008 ld   r0 sp 8
0xffffe480:  81e00010 ld   r1 sp 16
0xffffe484:  a1000000 st   r1 r0 0
0xffffe488:  80e00008 ld   r0 sp 8
0xffffe48c:  40080004 add  r0 r0 4H
0xffffe490:  a0e00008 st   r0 sp 8
0xffffe494:  80e0000c ld   r0 sp 12
0xffffe498:  40080004 add  r0 r0 4H
0xffffe49c:  e7ffffea br   Y2
                     Y4:
0xffffe4a0:  400000ff mov  r0 ffH
0xffffe4a4:  f7ffff2f br . SPI
0xffffe4a8:  400000ff mov  r0 ffH
0xffffe4ac:  f7ffff2d br . SPI
0xffffe4b0:  40000001 mov  r0 1H
0xffffe4b4:  f7ffff14 br . SPIIdle
0xffffe4b8:  8fe00000 ld   lr sp 0
0xffffe4bc:  4ee80014 add  sp sp 14H
0xffffe4c0:  c700000f br   lr

                     LoadFromDisk:
0xffffe4c4:  4ee90014 sub  sp sp 14H
0xffffe4c8:  afe00000 st   lr sp 0
0xffffe4cc:  60000008 mov' r0 8H
0xffffe4d0:  40060004 ior  r0 r0 4H
0xffffe4d4:  a0e00004 st   r0 sp 4
0xffffe4d8:  80e00004 ld   r0 sp 4
0xffffe4dc:  41000000 mov  r1 0H
0xffffe4e0:  f7ffffbf br . ReadSD
0xffffe4e4:  40000010 mov  r0 10H
0xffffe4e8:  80000000 ld   r0 r0 0
0xffffe4ec:  a0e00010 st   r0 sp 16
0xffffe4f0:  80e00004 ld   r0 sp 4
0xffffe4f4:  40080001 add  r0 r0 1H
0xffffe4f8:  a0e00004 st   r0 sp 4
0xffffe4fc:  40000200 mov  r0 200H
0xffffe500:  a0e00008 st   r0 sp 8
                     D1:
0xffffe504:  80e00008 ld   r0 sp 8
0xffffe508:  81e00010 ld   r1 sp 16
0xffffe50c:  00090001 sub  r0 r0 r1
0xffffe510:  ed00000a bge  D2
0xffffe514:  80e00004 ld   r0 sp 4
0xffffe518:  81e00008 ld   r1 sp 8
0xffffe51c:  f7ffffb0 br . ReadSD
0xffffe520:  80e00004 ld   r0 sp 4
0xffffe524:  40080001 add  r0 r0 1H
0xffffe528:  a0e00004 st   r0 sp 4
0xffffe52c:  80e00008 ld   r0 sp 8
0xffffe530:  40080200 add  r0 r0 200H
0xffffe534:  a0e00008 st   r0 sp 8
0xffffe538:  e7fffff2 br   D1
                     D2:
0xffffe53c:  8fe00000 ld   lr sp 0
0xffffe540:  4ee80014 add  sp sp 14H
0xffffe544:  c700000f br   lr

                     Begin:
0xffffe548:  4d000000 mov  sb 0H
0xffffe54c:  5e00ffc0 mov  sp ffffffc0H
0xffffe550:  6e000008 mov' sp 8H
0xffffe554:  4c000020 mov  mt 20H
0xffffe558:  0000000f mov  r0 lr
0xffffe55c:  40090000 sub  r0 r0 0H
0xffffe560:  e9000012 bne  B3
0xffffe564:  40000080 mov  r0 80H
0xffffe568:  5100ffc4 mov  r1 ffffffc4H
0xffffe56c:  a0100000 st   r0 r1 0
0xffffe570:  f7ffff50 br . InitSPI
0xffffe574:  5000ff00 mov  r0 ffffff00H
0xffffe578:  80000000 ld   r0 r0 0
0xffffe57c:  40030001 ror  r0 r0 1H
0xffffe580:  e8000005 bpl  B1
0xffffe584:  40000081 mov  r0 81H
0xffffe588:  5100ffc4 mov  r1 ffffffc4H
0xffffe58c:  a0100000 st   r0 r1 0
0xffffe590:  f7fffec1 br . LoadFromLine
0xffffe594:  e7000004 br   B2
                     B1:
0xffffe598:  40000082 mov  r0 82H
0xffffe59c:  5100ffc4 mov  r1 ffffffc4H
0xffffe5a0:  a0100000 st   r0 r1 0
0xffffe5a4:  f7ffffc7 br . LoadFromDisk
                     B2:
0xffffe5a8:  e7000008 br   B4
                     B3:
0xffffe5ac:  5000ffc4 mov  r0 ffffffc4H
0xffffe5b0:  80000000 ld   r0 r0 0
0xffffe5b4:  40030001 ror  r0 r0 1H
0xffffe5b8:  e8000004 bpl  B4
0xffffe5bc:  40000081 mov  r0 81H
0xffffe5c0:  5100ffc4 mov  r1 ffffffc4H
0xffffe5c4:  a0100000 st   r0 r1 0
0xffffe5c8:  f7fffeb3 br . LoadFromLine
                     B4:
0xffffe5cc:  4000000c mov  r0 cH
0xffffe5d0:  6100000e mov' r1 eH
0xffffe5d4:  41167ef0 ior  r1 r1 7ef0H
0xffffe5d8:  a1000000 st   r1 r0 0
0xffffe5dc:  40000018 mov  r0 18H
0xffffe5e0:  61000008 mov' r1 8H
0xffffe5e4:  a1000000 st   r1 r0 0
0xffffe5e8:  40000084 mov  r0 84H
0xffffe5ec:  5100ffc4 mov  r1 ffffffc4H
0xffffe5f0:  a0100000 st   r0 r1 0
0xffffe5f4:  40000000 mov  r0 0H
0xffffe5f8:  c7000000 br   r0

