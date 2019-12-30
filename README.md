qemu-risc6 README
=================

This is a clone of https://github.com/qemu/qemu
Work on the Oberon RISC backend is being done on the risc6 branch, so after
you clone this (git clone https://github.com/io-core/qemu-risc6) you will want to do the following within the repo directory:

git checkout risc6

Otherwise you might not see the Oberon RISC target.

<img src="https://github.com/io-core/qemu-risc6/blob/risc6/Oberon.png?raw=true">

The target is named risc6 here in an attempt to avoid confusion with the already existing riscv target in qemu and because in one communication (<a href="https://inf.ethz.ch/personal/wirth/ProjectOberon/RISC5.Update.pdf">An Update of the RISC5 Implementation</a>) Professor Wirth defines module RISC6 to introduce interrupts into the architecture. Everywhere else this architecture is called RISC5.

This is very much a work in progress. A number of opcodes are not fully implememented (floating point in particular) and interrupts are not wired up yet.

QEMU is a large project and there are lots of files in it. For the Oberon RISC architectur some key files are:

* disas/risc6.h
* disas/risc6.c
* target/risc6/cpu.h
* target/risc6/cpu.c
* target/risc6/translate.c
* hw/risc6/boot.c
* hw/risc6/fpga_devboard.c
* hw/timer/risc6_timer.c

[TODO: describe build proess.]

The target qemu-system-risc6 has been succesfully compiled on Linux and on OSX. It should also be possible to compile it on Windows with some effort.

An example command line running Oberon from the unmodified disk image downloaded from <a href="http://projectoberon.com">http://projectoberon.com</a>:

./risc6-softmmu/qemu-system-risc6 -machine fpga-risc -display gtk -g 1024x768x8 -monitor stdio -device loader,file=hw/risc6/risc-boot.bin,addr=0xfffff800 -drive format=raw,file=RISC.img -smp 1

<img src="https://github.com/io-core/qemu-risc6/blob/risc6/ClassicOberon.png?raw=true">

