./risc6-softmmu/qemu-system-risc6 -machine fpga-risc -g 1024x768x8 -monitor stdio -device loader,file=hw/risc6/risc-boot.bin,addr=0xfffff800 -drive format=raw,file=Oberon-2019-01-21.dsk
#./risc6-softmmu/qemu-system-risc6 -machine fpga-risc -g 1024x768x8 -monitor stdio -device loader,file=hw/risc6/risc-boot.bin,addr=0xfffff800 -drive format=raw,file=/home/cperkins/go/src/github.com/pdewacht/oberon-risc-emu/DiskImage/Oberon-2019-01-21.dsk
#
#./risc6-softmmu/qemu-system-risc6 -machine fpga-risc -vga cg3 -g 1024x768x8 -monitor stdio -device loader,file=hw/risc6/risc-boot.bin,addr=0xffffe000 -S
# 
