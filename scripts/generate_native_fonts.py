from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
chars=sorted(set(range(32,127))|set(range(0x400,0x460))|{0x3b1,0x3b2,0x3b3,0x2116,0x2190,0x2192,0xb0,0xb1,0xb2,0xb3,0xb7,0xd7,0xf7,0x3c0,0x394,0x3a9,0x2212,0x221a,0x221e,0x2248,0x2260,0x2264,0x2265,0xfffd})
out=['// DejaVu Sans subset, see ../../../assets/FONT-LICENSE.txt.','#pragma once','#include <cstdint>','struct NativeGlyph{uint32_t code,offset;uint8_t width;};']
for size in [7,9,11]:
 font=ImageFont.truetype(str(next(p for p in [Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'),Path('/usr/share/fonts/TTF/DejaVuSans.ttf')] if p.is_file())),size);data=[];glyphs=[]
 for c in chars:
  w=max(1,int(font.getlength(chr(c))+.9));im=Image.new('1',(w,size+3));ImageDraw.Draw(im).text((0,-1),chr(c),font=font,fill=1);bits=list(im.get_flattened_data());glyphs.append((c,len(data),w));data.extend(sum((bits[j+k] if j+k<len(bits) else 0)<<(7-k) for k in range(8)) for j in range(0,len(bits),8))
 out+=['const uint8_t native_pixels%d[]={%s};'%(size,','.join(map(str,data))),'const NativeGlyph native_glyphs%d[]={%s};'%(size,','.join('{%d,%d,%d}'%g for g in glyphs))]
(Path(__file__).resolve().parents[1]/'firmware/components/tiko/NativeFonts.h').write_text('\n'.join(out)+'\n')
