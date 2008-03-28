TOOLDIR=../..
PAYLOADDIR=$(TOOLDIR)/payloadgen
SIM65DIR=$(TOOLDIR)/sim65
PHACKDIR=$(TOOLDIR)/st2205tool

SIM65=$(SIM65DIR)/sim65
PHACK=$(PHACKDIR)/phack
RAM2CODE=$(TOOLDIR)/payloadgen/ram2code.py

DEVICE="/dev/disk/by-id/usb-SITRONIX_MULTIMEDIA-0\:0"

all:	payload.bin

clean:
	rm -f $(EXTRA_BINS)
	rm -f *.bin
	rm -f bootcode.asm
	rm -f bootcode.bin.rom
	rm -f bootcode.bin.ram

distclean:	clean
	rm -f firmware.dump
	rm -f memory.dump
	rm -f lcd.ppm

upload:	payload.bin
	$(PHACK) -r payload.bin "$(DEVICE)"

boot:	payload.bin
	test -f bootcode.bin.rom && echo "payload too large for ram-based boot, use make flashboot!" || $(PHACK) -r payload-bootcode.bin "$(DEVICE)"

forceboot:	payload.bin
	$(PHACK) -r payload-bootcode.bin "$(DEVICE)"

flashboot: payload.bin
	$(PHACK) -u bootcode.bin.rom "$(DEVICE)"
	$(PHACK) -r payload-bootcode.bin "$(DEVICE)"

flashinstall:	bootcode.bin.flashinit
	$(PHACK) -u bootcode.bin.flashinit  "$(DEVICE)"
	echo "using Clock Display to trigger code execution"

bootcode.bin.flashinit:	bootcode.bin
	echo splicing the ram-initialization code into the 16kb page of image memory
	cp bootcode.bin.rom bootcode.bin.flashinit || touch bootcode.bin.flashinit
	python "$(RAM2CODE)" bootcode.bin.ram bootcode.bin.initcode || exit 1
	dd if=bootcode.bin.initcode of=bootcode.bin.flashinit bs=1 count=16384 seek=16384 || exit 1

test:	bootcode.bin firmware.dump memory.dump
	rm -f lcd.ppm
	test -f bootcode.bin.rom && $(SIM65) bootcode.bin.ram bootcode.bin.rom || $(SIM65)

trace: bootcode.bin
	rm -f lcd.ppm
	test -f bootcode.bin.rom && SIM_TRACE=1 $(SIM65) bootcode.bin.ram bootcode.bin.rom || SIM_TRACE=1 $(SIM65)

payload.bin:	$(PAYLOADDIR)/mkpayload.py bootcode.bin
	python $(PAYLOADDIR)/mkpayload.py

bootcode.asm:	$(PAYLOADDIR)/bootcode.asm.template Makefile
	cat $(PAYLOADDIR)/bootcode.asm.template | sed -e s/%%PROJECT%%/$(PROJECT)/g > bootcode.asm
bootcode.bin:	bootcode.asm $(PROJECT) $(EXTRA_BINS)
	$(PAYLOADDIR)/assemble $< $@ "$(PROJECT)"

firmware.dump:
	$(PHACK) -df firmware.dump "$(DEVICE)"

memory.dump:
	$(PHACK) -d memory.dump "$(DEVICE)"
