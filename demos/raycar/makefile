
ALL_CONTENT := \
	content/raycar-unified.bin \
	content/crate1.bin \
	content/rustybarrel.bin \
	content/oval-track.bin \
	content/lostvalley-skybox.bin \
	content/raycar.scn \
	content/default-vs.glsl \
	content/default-fs.glsl \
	content/custom-vs.glsl \
	content/custom-fs.glsl

TOOL := ../../ode-0.12/build/vs2010/Debug/raycar.exe

all::	content $(ALL_CONTENT)

clean:
	rm -rf content

content:
	mkdir content

content/%.bin:	art/%.obj
	$(TOOL) --scale 1/40 --build-asset $<

content/%.scn:	art/%.scn
	$(TOOL) --build-asset $<

content/%.tga:	art/%.tga
	$(TOOL) --build-asset $<

content/%.tga:	art/%.png
	$(TOOL) --build-asset $<

content/%.tga:	art/%.jpg
	$(TOOL) --build-asset $<

content/%.tga:	art/%.gif
	$(TOOL) --build-asset $<

content/%.tga:	art/%.tif
	$(TOOL) --build-asset $<

content/%.tga:	art/%.bmp
	$(TOOL) --build-asset $<

content/%.glsl:	art/%.glsl
	cp $< $@

-include $(wildcard content/*.d)
