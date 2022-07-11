EE_BIN = main.elf
EE_OBJS = main.o draw_3D.o
EE_LIBS = -ldraw -lgraph -lmath3d -lpacket -ldma -lpad -linput
EE_DVP = dvp-as
EE_VCL = openvcl

ifndef WIN_DIR
WIN_DIR = .
endif

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
	cp $(EE_BIN) $(WIN_DIR)/$(EE_BIN)

clean:
	rm *.o
	rm *.elf

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
