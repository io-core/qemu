./risc6-softmmu/qemu-system-risc6 -machine fpga-risc -vga cg3 -g 1024x768x8 -monitor stdio -device loader,file=hw/risc6/risc-boot.bin,addr=0xffffe000 -S
 
