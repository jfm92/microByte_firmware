COMPONENT_OBJS := freetype2/src/base/ftsystem.o \
				freetype2/src/base/ftinit.o \
				freetype2/src/base/ftdebug.o \
				freetype2/src/base/ftbase.o \
				freetype2/src/truetype/truetype.o \
				freetype2/src/sfnt/sfnt.o \
				freetype2/src/smooth/smooth.o \
				font_render.o \
				

COMPONENT_SRCDIRS := freetype2/src/base \
					freetype2/src/truetype \
					freetype2/src/sfnt \
					freetype2/src/smooth \
					.	

COMPONENT_ADD_INCLUDEDIRS := include \
							freetype2/include \
							.
							
CFLAGS += -DFT2_BUILD_LIBRARY -Wno-unused-function -DIS_LITTLE_ENDIAN