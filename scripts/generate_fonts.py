"""Rebuild checked-in glyphs: python -m pip install Pillow==13.0.0."""
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
chars=sorted(set(range(32,127))|set(range(0x400,0x460))|{0x2212,0x2116,0x00b0,0x00b1,0x00b2,0x00b3,0x00d7,0x00f7,0x03b1,0x03b2,0x03b3,0x03c0,0x03a9,0x0394,0x221a,0x221e,0x2248,0x2260,0x2264,0x2265,0x2190,0x2192,0xfffd})
fontpath=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
out=['// Generated DejaVu Sans bitmap subset. See assets/FONT-LICENSE.txt.','#pragma once','#include <Arduino.h>','struct Glyph {uint32_t code;uint8_t width;uint32_t offset;};']
for size in (14,18,22):
 f=ImageFont.truetype(str(fontpath),size);data=[];glyphs=[]
 for c in chars:
  w=min(30,max(1,int(f.getlength(chr(c))+.9)));im=Image.new('1',(w,size+5));ImageDraw.Draw(im).text((0,0),chr(c),font=f,fill=1)
  glyphs.append((c,w,len(data)));bits=list(im.getdata())
  data.extend(sum((bits[j+k] if j+k<len(bits) else 0)<<(7-k) for k in range(8)) for j in range(0,len(bits),8))
 out+=['const uint8_t pixels%d[] PROGMEM={%s};'%(size,','.join(map(str,data))),'const Glyph glyphs%d[] PROGMEM={%s};'%(size,','.join('{%d,%d,%d}'%g for g in glyphs))]
Path('src/display/Fonts.h').write_text('\n'.join(out)+'\n')
