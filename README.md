qemu-risc6 README
=================

This is a clone of https://github.com/qemu/qemu
Work on the risc6 backend is being done on the risc6 branch, so after
you clone this (git clone https://github.com/io-core/qemu-risc6) you will want to do the following within the repo directory:

git checkout risc6

Otherwise you will not see the RISC6 target.

<img src="https://github.com/io-core/qemu-risc6/blob/risc6/Oberon.png?raw=true">

The target is named risc6 to avoid confusion with the already existing risc-v target in qemu and because in one communication (<a href="https://inf.ethz.ch/personal/wirth/ProjectOberon/RISC5.Update.pdf">An Update of the RISC5 Implementation</a>) Professor Wirth defines module RISC6 to introduce interrupts into the architecture.
