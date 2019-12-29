#!/bin/env python

import sys
import os.path

def tohex(val, nbits):
  return hex((val + (1 << nbits)) % (1 << nbits))


f0 = {
		"mov" : 0,
		"lsl" : 1,
		"asr" : 2,
		"ror" : 3,
		"and" : 4,
                "ann" : 5,
                "ior" : 6,
                "xor" : 7,
                "add" : 8,
                "sub" : 9,
                "mul" : 10,
                "div" : 11,
                "fad" : 12,
                "fsb" : 13,
                "fml" : 14,
                "fdv" : 15,
                "mov'" : 16,
                "add'" : 24,
                "sub'" : 25,
                "mul'" : 26
     }

f2 = {
                "ld" : 8,
                "ldb" : 9,
                "st" : 10,
                "stb" : 11
     }

f3 = {
                "bmi" : 0,
                "beq" : 1,
                "bcs" : 2,
                "bvs" : 3,
                "bls" : 4,
                "blt" : 5,
                "ble" : 6,
                "br" : 7,
                "bpl" : 8,
                "bne" : 9,
                "bcc" : 10,
                "bvc" : 11,
                "bhi" : 12,
                "bge" : 13,
                "bgt" : 14,
                "nop" : 15,
                "bmi." : 16,
                "beq." : 17,
                "bcs." : 18,
                "bvs." : 19,
                "bls." : 20,
                "blt." : 21,
                "ble." : 22,
                "br." : 23,
                "bpl." : 24,
                "bne." : 25,
                "bcc." : 26,
                "bvc." : 27,
                "bhi." : 28,
                "bge." : 29,
                "bgt." : 30,
                "nop." : 31,
                "rts"  : 32
     }

reg = {
                "r0" : 0,
                "r1" : 1,
                "r2" : 2,
                "r3" : 3,
                "r4" : 4,
                "r5" : 5,
                "r6" : 6,
                "r7" : 7,
                "r8" : 8,
                "r9" : 9,
                "ra" : 10,
                "rb" : 11,
                "mt" : 12,
                "sb" : 13,
                "sp" : 14,
                "lr" : 15,
                "rh" : 16,
                "fl" : 17

     }

label = {}

image = []

hdec="0123456789ABCDEF"

if len(sys.argv) < 2:
  print('Nothing to assemble.')
else:
  if os.path.isfile(sys.argv[1])==False:
    print('Input file '+sys.argv[1]+' not found')
  else:
    infile=sys.argv[1]
    outfile=sys.argv[1]+".bin"
    print('Assembling '+infile+' as '+outfile)

    ok = True

    fileHandler = open (infile, "r")
    while True:
      
      line = fileHandler.readline()
      if not line :
        break;

      elems=line.split()
      if len(elems)>0:
        op = elems[0]
        if op.endswith(':'):
          x=elems.pop(0)[:-1]
          if x in label:
                print("redefined label "+x)
                ok = False
                break;
          else:
            label[ x ] = [-1,[]]

    fileHandler.close()
    fileHandler = open (infile, "r")

    spos=0
    cpos=0
    if ok:
     while True:
      line = fileHandler.readline()
      if not line :
        break;

     

      elems=line.split()
      if len(elems)>0:
        op = elems[0]
        if op.endswith(':'):
          x=elems.pop(0)[:-1]
          if not x in label:
                print("label "+x+" not found")
                ok = False
                break;
          else:
            label[ x ] = [cpos,label[x][1]]

      if len(elems)>0:
        op = elems[0]
        if op in f0:
          aux = False
          if len(elems)>1:
              
            if f0[op]==0 or f0[op]==16:
              if not len(elems)>2:
                print("Bad format")
                ok = False
              else:
                dst=reg[elems[1]]
                crv=elems[2]
                tnib = "0"
                opc=0
                breg=0
                if f0[op]==16:
                  aux = True
          
                if crv in reg:
                  if crv == "rh":
                    tnib = "2"
                    image.append([tnib+hdec[dst]+hdec[breg]+hdec[opc]+"0000",""])
                    print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[dst]+hdec[breg]+hdec[opc]+"0000"+"  "+op+" r"+str(dst)+" "+crv)
                  elif crv == "fl":
                    tnib = "3"
                    image.append([tnib+hdec[dst]+hdec[breg]+hdec[opc]+"0000",""])
                    print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[dst]+hdec[breg]+hdec[opc]+"0000"+"  "+op+" r"+str(dst)+" "+crv)
                  else:    
                    image.append([tnib+hdec[dst]+hdec[breg]+hdec[opc]+"000"+hdec[reg[crv]],""])
                    print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[dst]+hdec[breg]+hdec[opc]+"000"+hdec[reg[crv]]+"  "+op+" r"+str(dst)+" "+crv)
                else:
                  if aux:
                    tnib="6"
                  else:
                    tnib="4"
                  if crv.endswith('H'):
                    val=crv[:-1]
                    if len(val)==3:
                      val="0"+val
                    if len(val)==2:
                      val="00"+val
                    if len(val)==1:
                      val="000"+val
                    if len(val)>4:
                      val=val[-4:];
                      #print("Constant too large")
                      #ok = False
                    if val[0]>"7":
                      if tnib=="6":
                        tnib="7"
                      else:
                        tnib="5"
                    image.append([tnib+hdec[dst]+hdec[breg]+hdec[opc]+val,""])
                    print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[dst]+hdec[breg]+hdec[opc]+val+"  "+op+" r"+str(dst)+" "+crv)
                  else:
                      print("Bad constant: "+crv)
                      ok = False
            else:
              if not len(elems)>2:
                print("Bad format")
                ok = False
              else:
                dst=reg[elems[1]]
                breg=reg[elems[2]]
                crv=elems[3]
                opc=f0[op]
                if opc > 16:
                  opc = opc - 16
                  aux = True
                else:
                  aux = False
                if crv in reg:
                  tnib = "0"
                  image.append([tnib+hdec[dst]+hdec[breg]+hdec[opc]+"000"+hdec[reg[crv]],""])
                  print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[dst]+hdec[breg]+hdec[opc]+"000"+hdec[reg[crv]]+"  "+op+" r"+str(dst)+" "+crv)
                else:
                  tnib="4"
                  if crv.endswith('H'):
                    val=crv[:-1]
                    if len(val)==3:
                      val="0"+val
                    if len(val)==2:
                      val="00"+val
                    if len(val)==1:
                      val="000"+val
                    if len(val)>4:
                      val=val[-4:];
                      #print("Constant too large")
                      #ok = False
                    if val[0]>"7":
                      tnib="5"
                    image.append([tnib+hdec[dst]+hdec[breg]+hdec[opc]+val,""])
                    print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[dst]+hdec[breg]+hdec[opc]+val+"  "+op+" r"+str(dst)+" "+crv)
                  else:
                      print("Bad constant: "+crv)
                      ok = False
#                print(op+" r"+str(dst)+" r"+str(regb))
          else:
            print("Bad format")
            ok = False
          cpos = cpos + 4


        elif op in f2:
          tnib = hdec[f2[op]]
          
          areg=reg[elems[1]]
          breg=reg[elems[2]]
          crv=elems[3]
          val="00000"
          if crv.endswith('H'):
                     print("Constant must be decimal not hex")
                     ok = False
          else:
#                    print("-->",hex(int(crv))[2:],"<--")
                    val=hex(int(crv))[2:];
                    if len(val)==4:
                      val="0"+val
                    if len(val)==3:
                      val="00"+val
                    if len(val)==2:
                      val="000"+val
                    if len(val)==1:
                      val="0000"+val
                    if len(val)>5:
                      val=val[-5:];
                      print("Constant too large")
                      ok = False
          

          image.append([tnib+hdec[areg]+hdec[breg]+val,""])
          print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[areg]+hdec[breg]+val+"  "+op+" r"+str(areg)+" r"+str(breg)+" "+crv)
          cpos = cpos + 4


        elif op in f3:
          aux = False
          bcond=f3[op]
          val = ""
          crv=""
          if len(elems)>1:
            crv=elems[1]
          if crv.endswith('H'):
                    val=crv[:-1]
                    if len(val)==5:
                      val="0"+val
                    if len(val)==4:
                      val="00"+val
                    if len(val)==3:
                      val="000"+val
                    if len(val)==2:
                      val="0000"+val
                    if len(val)==1:
                      val="00000"+val
                    if len(val)>6:
                      val=val[-6:];
                      #print("Constant too large")
                      #ok = False
          if bcond==32:
            bcond=7
            tnib="C"
            if len(val) > 0:
              image.append([tnib+hdec[bcond]+val,""])
              print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[bcond]+val+"  "+op)
            else:
              image.append([tnib+hdec[bcond]+"00000F",crv])
              print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[bcond]+"00000F"+"  "+op)
          elif bcond > 15:
#            crv=elems[1]
            bcond = bcond - 16
            aux = True
            tnib="F"
            if crv in reg:
              image.append([tnib+hdec[bcond]+"00000"+hdec[reg[crv]],""])
              print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[bcond]+"00000"+hdec[reg[crv]]+"  "+op+" "+crv)
            else:
              if len(val) > 0:
                image.append([tnib+hdec[bcond]+val,""])
                print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[bcond]+val+"  "+op+" "+crv)
              else:
                image.append([tnib+hdec[bcond]+"------",crv])
                print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[bcond]+"------"+"  "+op+" "+crv)
          else:
#            crv=elems[1]
            if crv in reg:
              tnib="C"
              image.append([tnib+hdec[bcond]+"00000"+hdec[reg[crv]],""])
              print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[bcond]+"00000"+hdec[reg[crv]]+"  "+op+" "+crv)
            else:
              tnib="E"
              if len(val) > 0:
                image.append([tnib+hdec[bcond]+val,""])
                print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[bcond]+val+"  "+op+" "+crv)
              else:
                image.append([tnib+hdec[bcond]+"000000",crv])
                print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[bcond]+"000000"+"  "+op+" "+crv)
          cpos = cpos + 4
        else:
          print("unknown op",op)
          ok = False
      if not ok:
        break;
    fileHandler.close()  

    f = open(outfile, 'wb')

    print(label)
    l = len(image)
    cpos = 0
    for i in image:
      word = "00000000"
      if i[1] == "":
        word = i[0]
      else:
        word = i[0][:2]+tohex(  (label[i[1]][0] - (cpos+4))>>2 ,24 )[2:].zfill(6)
      cpos = cpos + 4
#      print(word[6:8]+" "+word[4:6]+" "+word[2:4]+" "+word[:2])

      ba=bytearray.fromhex(word[6:8]+" "+word[4:6]+" "+word[2:4]+" "+word[:2])
      for byte in ba:
        f.write(byte.to_bytes(1, byteorder='big'))
      print(hex(cpos-4)[2:].zfill(8)+"   "+word)
    f.close()
