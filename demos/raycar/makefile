
ALL_CONTENT := \
	content/raycar-unified.bin \
	content/raycar.scn

all:	content $(ALL_CONTENT)

content:
	mkdir content

content/%.bin:	art/%.obj
	../../ode-0.12/build/vs2010/Debug/raycar.exe --scale 1/40 --objtobin $< $@

content/%.scn:	art/%.scn
	cp $< $@
