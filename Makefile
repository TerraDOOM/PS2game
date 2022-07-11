EE_BIN = main.elf
EE_OBJS = main.o
EE_LIBS = -ldraw -lgraph -lmath3d -lpacket -ldma -lpad -linput
EE_DVP = dvp-as
EE_VCL = openvcl

# Original VCL tool preferred. 
# It can be runned on WSL, but with some tricky commands: 
# https://github.com/microsoft/wsl/issues/2468#issuecomment-374904520
%.vsm: %.vcl
	$(EE_VCL) $< >> $@

%.o: %.vsm
	$(EE_DVP) $< -o $@

.PHONY: clean
all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)
ifdef WIN_DIR
	cp $(EE_BIN) $(WIN_DIR)/$(EE_BIN)
endif

clean:
	rm *.o
	rm *.elf

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
