# A mini OS kernel
Simple multiboot kernel written in assembly and C

## Features
* **Multiboot Compliant:** Designed to be loaded by any standard Multiboot-compliant bootloader (like GRUB).
* **Custom VGA Text Mode Driver:** Includes a basic driver to handle screen clearing, colored text rendering, and simulated cursor tracking directly in video memory (`0xB8000`).
* **Dynamic Hardware Detection:** Utilizes x86 inline assembly (`cpuid`) within C to directly query the processor and retrieve the CPU Vendor ID at boot.

## Prerequisites
To build and emulate this project, you will need the following tools installed on your Linux system:
* `gcc`
* `nasm`
* `ld`
* `qemu-system-i386` (For emulation)

## Project Structure
* 'boot.s' The assembly entry point. Defines the MultiBoot header, sets up the initial stack pointer then launch C kernel.
* 'kernel.c' The main kernel logic. It contains the VGA memory manipulation functions and the CPU detection routine.
* 'linker.ld' Linker script that ensures the kernel is correctly structured and loaded at the standard 1MB memory mark.
* 'Makefile' Automates compilation, linking, and emulation commands.  

## Acknowledgments
* [OSDev.org](https://wiki.osdev.org/)
