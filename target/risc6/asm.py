#!/bin/env python

import sys
import os.path

def tohex(val, nbits):
  return hex((val + (1 << nbits)) % (1 << nbits))


f0 = {
		"MOV" : 0,
		"LSL" : 1,
		"ASR" : 2,
		"ROR" : 3,
		"AND" : 4,
                "ANN" : 5,
                "IOR" : 6,
                "XOR" : 7,
                "ADD" : 8,
                "SUB" : 9,
                "MUL" : 10,
                "DIV" : 11,
                "FAD" : 12,
                "FSB" : 13,
                "FML" : 14,
                "FDV" : 15,
                "MOV'" : 16,
                "ADD'" : 24,
                "SUB'" : 25,
                "MUL'" : 26
     }

f2 = {
                "LDR" : 8,
                "LDB" : 9,
                "STR" : 10,
                "STB" : 11
     }

f3 = {
                "BMI" : 0,
                "BEQ" : 1,
                "BCS" : 2,
                "BVS" : 3,
                "BLS" : 4,
                "BLT" : 5,
                "BLE" : 6,
                "BR" : 7,
                "BPL" : 8,
                "BNE" : 9,
                "BCC" : 10,
                "BVC" : 11,
                "BHI" : 12,
                "BGE" : 13,
                "BGT" : 14,
                "NOP" : 15,
                "BMIL" : 16,
                "BEQL" : 17,
                "BCSL" : 18,
                "BVSL" : 19,
                "BLSL" : 20,
                "BLTL" : 21,
                "BLEL" : 22,
                "JSR" : 23,
                "BPLL" : 24,
                "BNEL" : 25,
                "BCCL" : 26,
                "BVCL" : 27,
                "BHIL" : 28,
                "BGEL" : 29,
                "BGTL" : 30,
                "NOPL" : 31,
                "RTS"  : 32
     }

reg = {
                "R0" : 0,
                "R1" : 1,
                "R2" : 2,
                "R3" : 3,
                "R4" : 4,
                "R5" : 5,
                "R6" : 6,
                "R7" : 7,
                "R8" : 8,
                "R9" : 9,
                "R10" : 10,
                "R11" : 11,
                "R12" : 12,
                "R13" : 13,
                "R14" : 14,
                "R15" : 15,
                "BP" : 13,
                "SP" : 14,
                "LNK" : 15,
                "H" : 16,
                "NZCV" : 17

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

      elems=line.upper().split()
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

     

      elems=line.upper().split()
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
                  if crv == "H":
                    tnib = "2"
                    image.append([tnib+hdec[dst]+hdec[breg]+hdec[opc]+"0000",""])
                    print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[dst]+hdec[breg]+hdec[opc]+"0000"+"  "+op+" r"+str(dst)+" "+crv)
                  elif crv == "NZCV":
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
                      print("Constant too large")
                      ok = False
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
                      print("Constant too large")
                      ok = False
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
                    val=crv[:-1]
                    if len(val)==4:
                      val="0"+val
                    if len(val)==3:
                      val="00"+val
                    if len(val)==2:
                      val="000"+val
                    if len(val)==1:
                      val="0000"+val
                    if len(val)>5:
                      print("Constant too large")
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
                      print("Constant too large")
                      ok = False
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
              image.append([tnib+hdec[bcond]+"----0"+hdec[reg[crv]],""])
              print(hex(cpos)[2:].zfill(8)+" "+tnib+hdec[bcond]+"----0"+hdec[reg[crv]]+"  "+op+" "+crv)
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
          print("unknown op")
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
      ba=bytearray.fromhex(word[6:8]+" "+word[4:6]+" "+word[2:4]+" "+word[:2])
      for byte in ba:
        f.write(byte.to_bytes(1, byteorder='big'))
      print(hex(cpos-4)[2:].zfill(8)+"   "+word)
    f.close()
