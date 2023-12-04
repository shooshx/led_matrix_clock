#!/usr/bin/python3
import sys, os
import re
import glob

class Glyph:
    font_bb_w = 0
    font_bb_h = 0
    font_bb_offx = 0
    font_bb_offy = 0    
    encoding = -1
    rows = []
    comment = ""
    offset = -1
    width = 0
    height = 0
    advance = 0
    xoffs = 0
    yoffs = 0
    bits_width = 0
    bytes_width = 0
    def __init__(self, comment):
        self.comment = comment
        self.rows = []

class Font:
    font_family = None
    font_size = 0
    fontName = None
    font_bb_h = 0
    header_filename = ''
    def __init__(self):
        self.chars = []


def open_bdf(filename):
    myfile = open(filename)

    processing = 0
    getting_rows = 0    
    bitmapData = []
    is_atari_encoding = False
    f = Font()

    def bytes(integer):
        return divmod(integer, 0x100)

    for line in myfile.readlines():

        if 'FONT ' in line:
            values = line[5:]
            f.fontName = re.sub('[\W_]+', '', values) 

        elif 'FAMILY_NAME' in line:
            values = line[12:]
            f.font_family = values.strip().strip('"')

        elif line.startswith('SIZE '):
            values = line.split()
            f.font_size = int(values[1])

        elif 'FONTBOUNDINGBOX' in line:
            values = line.split()
            font_bb_w = int(values[1])
            font_bb_h = int(values[2])
            f.font_bb_h = font_bb_h
            font_bb_offx = int(values[3])
            font_bb_offy = int(values[4])

        elif 'STARTCHAR' in line:
            processing = 1
            vals = line.split()
            g = Glyph(vals[1])
            #g.width = 8  #in this example always 8 bits wide

        elif 'ENDCHAR' in line:
            # dataByteCompressed = 0
            # dataByteCompressedIndex = 8
            if g.encoding == -1:
                processing = 0
                getting_rows = 0
                bitmapData.clear()
                continue  # final end character
            g.height = len(bitmapData)
            if len(bitmapData) > 0:
                g.bytes_width = len(bitmapData[0]) / 2
                g.bits_width = g.bytes_width * 8
                if g.bytes_width > 2:
                    return None  # more than 2 bytes
                # all the same bit length
                for value in bitmapData:
                    assert len(value) == len(bitmapData[0])
            else:
                g.bytes_width = 1 # avoid division by zero

            for value in bitmapData:
                # split data into high and low hex
                high, low = bytes(int(value, 16))
                if g.bits_width > 8:
                    g.rows.append(high)
                g.rows.append(low)   

            # y offset from top
            # if glyph does not fill the whole box set the offset from the top
            if ((len(g.rows) / g.bytes_width) + g.decent) < (font_bb_h + font_bb_offy):
                g.yoffs =  (font_bb_h + font_bb_offy) - ((len(g.rows) / g.bytes_width) + g.decent)

            while g.encoding >= len(f.chars):
                f.chars.append(None)
            f.chars[g.encoding] = g  #append the completed glyph into list
            processing = 0
            getting_rows = 0
            bitmapData.clear()

        if processing:
            if 'ENCODING' in line:
                # The word ENCODING followed by one of the following forms:
                # <n> − the glyph index, that is, a positive integer representing the character code used to
                # access the glyph in X requests, as defined by the encoded character set given by the
                # CHARSET_REGISTRY-CHARSET_ENCODING font properties for XLFD conforming
                # fonts. If these XLFD font properties are not defined, the encoding scheme is font-depen-
                # dent.            
                vals = line.split()
                g.encoding = int(vals[1])
                if g.encoding == 33 and g.comment == 'A':
                    print('Atari encoding!')
                    is_atari_encoding = True
            elif 'DWIDTH ' in line:
                # The word DWIDTH followed by the width in x and y of the character in device units. Like the
                # SWIDTH, this width information is a vector indicating the position of the next character’s ori-
                # gin relative to the origin of this character. Note that the DWIDTH of a given ‘‘hand-tuned’’
                # WYSIWYG glyph may deviate slightly from its ideal device-independent width given by
                # SWIDTH in order to improve its typographic characteristics on a display. The DWIDTH y
                # value should always be zero for a standard X font.
                vals = line.split()
                g.advance = int(vals[1])  #cursor advance seems to be the first number in DWIDTH
            elif 'BBX' in line:
                # The word BBX followed by the width in x (BBw), height in y (BBh), and x and y displacement
                # (BBox, BBoy) of the lower left corner from the origin of the character.
                vals = line.split()
                g.xoffs = int(vals[3]) + 1
                g.yoffs = 0
                g.decent = (int(vals[4]))
                # g.advance = (int(vals[1]) + 1)  #x bounding box + 1
                g.width = int(vals[1])
            elif 'BITMAP' in line:
                getting_rows = 1
            elif getting_rows:
                # h lines of hex-encoded bitmap, padded on the right with zeros to the nearest byte (that is, multi-
                # ple of 8).
                bitmapData.append(line.strip())  #append pixel rows into glyph's list of rows
                # zero pad right
                #bitmapData.append(int(line.rstrip().ljust(4, '0'), 16))
    
    if is_atari_encoding:
        nchars = [None]*128
        unknowns = []
        for c in f.chars:
            if c.encoding in ATARI_MAP:
                c.encoding = ord(ATARI_MAP[c.encoding])
                nchars[c.encoding] = c
            else:
                unknowns.append(c)
        # find places for the unknowns
        i = 0
        for uc in unknowns:
            while nchars[i] is not None:
                i += 1
            uc.encoding = i
            nchars[i] = uc
        f.chars = nchars

    for i in range(0, len(f.chars)):
        if f.chars[i] is None:
            f.chars[i] = Glyph("empty")
            f.chars[i].encoding = i

    # leave only 32-127
    nchars = []
    for i, c in enumerate(f.chars):
        if i >= 32 and i <= 127:
            nchars.append(c)
            assert i == c.encoding, "ehh %s,%s" % (i, c.encoding)
    f.chars = nchars

    return f

ATARI_MAP_RAW = [
0, ' ',1, '!',2, '"',3, '#',4, '$',5, '%',6, '&',7, "'",8, '(',9, ')',10, '*',11, '+',12, ',',13, '-',14, '.',15, '/',
16, '0',17, '1',18, '2',19, '3',20, '4',21, '5',22, '6',23, '7',24, '8',25, '9',
26, ':',27, ';',28, '<',29, '=',30, '>',31, '?',32, '@',
33, 'A',34, 'B',35, 'C',36, 'D',37, 'E',38, 'F',39, 'G',40, 'H',41, 'I',42, 'J',43, 'K',44, 'L',45, 'M',46, 'N',
47, 'O',48, 'P',49, 'Q',50, 'R',51, 'S',52, 'T',53, 'U',54, 'V',55, 'W',56, 'X',57, 'Y',58, 'Z',
59, '[',60, '\\',61, ']',62, '^',63, '_',64, '|',
97, 'a',98, 'b',99, 'c',100, 'd',101, 'e',102, 'f',103, 'g',104, 'h',105, 'i',106, 'j',107, 'k',108, 'l',109, 'm',
110, 'n',111, 'o',112, 'p',113, 'q',114, 'r',115, 's',116, 't',117, 'u',118, 'v',119, 'w',120, 'x',121, 'y',122, 'z',
]
ATARI_MAP = {}
for i in range(0, len(ATARI_MAP_RAW), 2):
    ATARI_MAP[ATARI_MAP_RAW[i]] = ATARI_MAP_RAW[i+1]


def output_header(f, filename):
    out = open(filename, 'w')
    #print("#include <Adafruit_GFX.h>\n\n", end='', file=out)
    print("#ifndef ARDUINO\n  #define PROGMEM\n#endif\n", file=out)
    print("const uint8_t %sBitmap[] PROGMEM = {	\n" %(f.fontName), file=out)

    min_enc = 99999
    max_enc = 0
    i = 0
    for char in f.chars:
        min_enc = min(char.encoding, min_enc)
        max_enc = max(char.encoding, max_enc)
        #print("// doing", char.encoding)
        char.offset = i
        if char.rows:
            print ("\t", end='', file=out)

        for row in char.rows:
            print ("0x%02X," %(row), end = '', file=out),
            i+=1

        if char.rows:
            print ("\t/* 0x%02X %s '%s' */" %(char.encoding, char.comment, chr(char.encoding) if char.encoding >= 0x20 else ''), file=out)

    print ("};\n\n", file=out)
    print ("const GFXglyph %sGlyph[] PROGMEM = {\n" %(f.fontName), file=out)

    print("/* offset, bit-width, bit-height, advance cursor, x offset, y offset */", file=out)

    for char in f.chars:
        # offset, bit-width, bit-height, advance cursor, x offset, y offset
        character = chr(char.encoding) if char.encoding >= 0x20 else ''

        if not char.rows:
            character = "No Bitmap Defined"

        print ("\t{ %d, %d, %d, %d, %d, %d }, /* 0x%02X %s '%s' */" %(
                char.offset, char.bits_width, char.height,
                char.advance, char.xoffs, char.yoffs,
                char.encoding, char.comment, character), file=out)

    print ("};\n\n", file=out)
    print ("const GFXfont %s PROGMEM = {" %(f.fontName), file=out)
    print ("\t(uint8_t *)%sBitmap," %(f.fontName), file=out)
    print ("\t(GFXglyph *)%sGlyph, " %(f.fontName), file=out)
    print ("\t%d, %d, %d //ASCII start, ASCII stop,y Advance" %(min_enc, max_enc, f.font_bb_h), file=out)
    print ("};\n", file=out)

this_dir = os.path.dirname(os.path.abspath(__file__))
fonts_dir = os.path.abspath(os.path.join(this_dir, "..", "clock", "my_fonts"))

def output_index(fonts):
    out = open(os.path.join(fonts_dir, "fonts_index.h"), 'w')
    print('#pragma once\n', file=out)
    fonts = sorted(fonts, key=lambda f: (f.font_family, f.font_size))

    for f in fonts:
        print(f'#include "{f.header_filename}"', file=out)
    print ("\n\n", file=out)

    print("struct FontDef\n{\n  const GFXfont* fontPtr;\n  const char* familyName;\n  int fontSize;\n};\n", file=out)
    print("const FontDef all_fonts[] PROGMEM = {", file=out)
    for f in fonts:
        print('  {&%s, "%s", %d},' % (f.fontName, f.font_family, f.font_size), file=out)
    print("};", file=out)


def main():
    filepaths = glob.glob(os.path.abspath(os.path.join(fonts_dir, "*.bdf")))
    fonts = []
    names = {}
    for filepath in filepaths:
        print("Reading ", filepath)
        f = open_bdf(filepath)
        if f is None:
            print("Unsupported font ", filepath)
            continue
        outpath = os.path.splitext(filepath)[0] + ".h"
        print("Saving ", outpath)
        output_header(f, outpath)
        f.header_filename = os.path.split(outpath)[1]
        fonts.append(f)
        assert f.fontName not in names, "same font name more than once: " + f.fontName
        names[f.fontName] = f
    print("Saving index", len(names))
    output_index(fonts)

def test():
    f = open_bdf("C:/projects/led_matrix_clock/clock/my_fonts/mac_Chicago_12.bdf")
    output_header(f, "C:/projects/led_matrix_clock/clock/my_fonts/mac_Chicago_12.h")
        

# fonts are from https://robhagemans.github.io/monobit/

if __name__ == "__main__":
    main()
    #test()