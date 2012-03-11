
ALL_CONTENT := \
	content/raycar-unified.bin \
	content/raycar.scn \
	content/crate1.bin

TOOL := ../../ode-0.12/build/vs2010/Debug/raycar.exe

all::	content $(ALL_CONTENT)

content:
	mkdir content

content/%.bin:	art/%.obj
	$(TOOL) --scale 1/40 --build-asset $<

content/%.tga:	art/%.tga
	$(TOOL) --build-asset $<

content/%.scn:	art/%.scn
	$(TOOL) --build-asset $<

-include $(wildcard content/*.d)
