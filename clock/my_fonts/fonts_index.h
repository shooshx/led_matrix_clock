#pragma once

#include "Ammolite_8x10.h"
#include "ARTY.h"
#include "atari-international.h"
#include "setfont-bold.h"
#include "setfont-capitals.h"
#include "mac_Chicago_12.h"
#include "clR6x12-L8.h"
#include "commadore_c128.h"
#include "Cory_12.h"
#include "mac_Cream_10.h"
#include "cursive.h"
#include "dagger.h"
#include "Diamond_12.h"
#include "mac_Espy_Sans_9.h"
#include "mac_Espy_Sans_10.h"
#include "mac_Espy_Sans_12.h"
#include "mac_Espy_Sans_Bold_9.h"
#include "mac_Espy_Sans_Bold_10.h"
#include "mac_Espy_Sans_Bold_12.h"
#include "mac_Espy_Serif_10.h"
#include "mac_Espy_Serif_12.h"
#include "Espy_Serif_Bold_10.h"
#include "4x6-L8.h"
#include "5x7-L8.h"
#include "7x14-L8.h"
#include "7x14B-L8.h"
#include "6x9-L8.h"
#include "6x10-L8.h"
#include "5x8-L8.h"
#include "9x15B-L8.h"
#include "atari-small.h"
#include "12x13ja-L8.h"
#include "6x12-L8.h"
#include "6x13-L8.h"
#include "9x18-L8.h"
#include "9x18B-L8.h"
#include "6x13B-L8.h"
#include "7x13-L8.h"
#include "7x13B-L8.h"
#include "8x13-L8.h"
#include "8x13B-L8.h"
#include "9x15-L8.h"
#include "10x20-L8.h"
#include "gem-1_8x8.h"
#include "gem-1.h"
#include "Geneva_9.h"
#include "Geneva_10.h"
#include "Geneva_12.h"
#include "Geneva_14.h"
#include "Geneva_18.h"
#include "GOTHIC.h"
#include "win_Helv_10.h"
#include "lisa_Modern_14pt_C.h"
#include "lisa_Modern_7pt.h"
#include "Helvetica_9.h"
#include "Helvetica_11.h"
#include "mac_Helvetica_12.h"
#include "mac_Helvetica_14.h"
#include "Helvetica-Bold_10.h"
#include "pc_iskra_1080.h"
#include "Los_Angeles_12.h"
#include "MicroKnight.h"
#include "Monaco_6x11.h"
#include "Monaco_6x16.h"
#include "New_York_9.h"
#include "New_York_12.h"
#include "New_York_14.h"
#include "Ohlfs_9.h"
#include "Ohlfs_10.h"
#include "Ohlfs_12.h"
#include "Ohlfs_16.h"
#include "Onyx_7x9.h"
#include "Opal_12.h"
#include "Palatino_12.h"
#include "Palatino_14.h"
#include "PalmOS-Bold.h"
#include "PalmOS-Standard.h"
#include "Peridot_7.h"
#include "poisk.h"
#include "atari-classic.h"
#include "setfont-rounded.h"
#include "Ruby_12.h"
#include "tele_saa5056-hebrew.h"
#include "setfont-serif.h"
#include "setfont-square1.h"
#include "setfont-square2.h"
#include "xerox-alto-sysfont.h"
#include "mac_System_12.h"
#include "atari-st-6x6.h"
#include "Terminal_EGA.h"
#include "Terminal_90.h"
#include "lisa_8pt_20.h"
#include "Tms_Rmn5.h"
#include "Tms_Rmn7.h"
#include "Tms_Rmn2.h"
#include "Toronto_12.h"
#include "University_10.h"
#include "University_12.h"



struct FontDef
{
  const GFXfont* fontPtr;
  const char* familyName;
  int fontSize;
};

const FontDef all_fonts[] PROGMEM = {
  {&AmmoliteRegularRNormal101007272C80ISO88591, "Ammolite", 10},
  {&ArtyRegularRNormal8807272C80ISO106461, "Arty", 8},
  {&AtarixlRegularRNormal8807272C80ATASCIIINTERNATIONAL, "Atarixl", 8},
  {&BoldRegularRNormal8807272C80AMSTRADCPMPLUS, "Bold", 8},
  {&CapitalsRegularRNormal8807272C80AMSTRADCPMPLUS, "Capitals", 8},
  {&ChicagoRegularRNormal151209090P77APPLEROMAN, "Chicago", 12},
  {&SchumacherCleanMediumRNormal121207575C60iso88598, "Clean", 12},
  {&Commodore128RegularRNormal8807272C80C64FULL, "Commodore 128", 8},
  {&CoryRegularRNormal121207272P69FONTSPECIFIC0, "Cory", 12},
  {&CreamRegularRNormal121008686P67APPLEROMAN, "Cream", 10},
  {&FineCursiveMediumINormal131307272C120ISO88591, "Cursive", 13},
  {&Dagger, "Dagger", 13},
  {&DiamondRegularRNormal121207272P87ISO88591, "Diamond", 12},
  {&EspySansRegularRNormal12909696P53APPLEROMAN, "Espy Sans", 9},
  {&EspySansRegularRNormal131009393P59APPLEROMAN, "Espy Sans", 10},
  {&EspySansRegularRNormal151209090P65APPLEROMAN, "Espy Sans", 12},
  {&EspySansBoldRegularRNormal12909696P61APPLEROMAN, "Espy Sans Bold", 9},
  {&EspySansBoldRegularRNormal131009393P66APPLEROMAN, "Espy Sans Bold", 10},
  {&EspySansBoldRegularRNormal161209696P81APPLEROMAN, "Espy Sans Bold", 12},
  {&EspySerifRegularRNormal131009393P63APPLEROMAN, "Espy Serif", 10},
  {&EspySerifRegularRNormal161209696P74APPLEROMAN, "Espy Serif", 12},
  {&EspySerifBoldRegularRNormal131009393P72APPLEROMAN, "Espy Serif Bold", 10},
  {&MiscFixedMediumRNormal6607575C40iso88598, "Fixed", 6},
  {&MiscFixedMediumRNormal7707575C50iso88598, "Fixed", 7},
  {&MiscFixedMediumRNormal141307575C70iso88598, "Fixed", 7},
  {&MiscFixedBoldRNormal141307575C70iso88598, "Fixed", 7},
  {&MiscFixedMediumRNormal9907575C60iso88598, "Fixed", 9},
  {&MiscFixedMediumRNormal101007575C60iso88598, "Fixed", 10},
  {&MiscFixedMediumRNormal8807575C50iso88598, "Fixed", 11},
  {&MiscFixedBoldRNormal151407575C90iso88598, "Fixed", 11},
  {&atarismall, "Fixed", 11},
  {&MiscFixedMediumRNormalja131207575C120iso88598, "Fixed", 12},
  {&MiscFixedMediumRSemiCondensed121107575C60iso88598, "Fixed", 12},
  {&MiscFixedMediumRSemiCondensed131207575C60iso88598, "Fixed", 12},
  {&MiscFixedMediumRNormal18120100100C90iso88598, "Fixed", 12},
  {&MiscFixedBoldRNormal18120100100C90iso88598, "Fixed", 12},
  {&MiscFixedBoldRSemiCondensed131207575C60iso88598, "Fixed", 13},
  {&MiscFixedMediumRNormal131207575C70iso88598, "Fixed", 13},
  {&MiscFixedBoldRNormal131207575C70iso88598, "Fixed", 13},
  {&MiscFixedMediumRNormal131207575C80iso88598, "Fixed", 13},
  {&MiscFixedBoldRNormal131207575C80iso88598, "Fixed", 13},
  {&MiscFixedMediumRNormal151407575C90iso88598, "Fixed", 15},
  {&MiscFixedMediumRNormal202007575C100iso88598, "Fixed", 20},
  {&G12Cga8X8RegularRNormal81005757C80FONTSPECIFIC0, "G12-Cga-8X8", 10},
  {&G12Ega8X14RegularRNormal111007979C80FONTSPECIFIC0, "G12-Ega-8X14", 10},
  {&GenevaRegularRNormal12909696P54APPLEROMAN, "Geneva", 9},
  {&GenevaRegularRNormal121008686P60APPLEROMAN, "Geneva", 10},
  {&GenevaRegularRNormal151209090P70APPLEROMAN, "Geneva", 12},
  {&GenevaRegularRNormal181409292P81APPLEROMAN, "Geneva", 14},
  {&GenevaRegularRNormal221808888P106APPLEROMAN, "Geneva", 18},
  {&GothicRegularRNormal8807272C80ISO106461, "Gothic", 8},
  {&HelvRegularRNormal71009648P67WINDOWSANSI20, "Helv", 10},
  {&Helv14CRegularRNormal171707272P92FONTSPECIFIC0, "Helv14C", 17},
  {&Helv7RpsRegularRNormal111107272P64FONTSPECIFIC0, "Helv7Rps", 11},
  {&HelveticaRegularRNormal9907272P50ISO88591, "Helvetica", 9},
  {&HelveticaRegularRNormal111107272P60ISO88591, "Helvetica", 11},
  {&HelveticaRegularRNormal131207878P65APPLEROMAN, "Helvetica", 12},
  {&HelveticaRegularRNormal141407272P75APPLEROMAN, "Helvetica", 14},
  {&HelveticaBoldRegularRNormal111007575P59ISO88591, "Helvetica-Bold", 10},
  {&Iskra1080RegularRNormal8807272C80ISO106461, "Iskra1080", 8},
  {&LosAngelesRegularRNormal141208484P67APPLEROMAN, "Los Angeles", 12},
  {&MicroKnightRegularRNormal8807272C80ISO88591, "MicroKnight", 8},
  {&MonacoRegularRNormal11908888P58APPLEROMAN, "Monaco", 9},
  {&MonacoRegularRNormal151209090P67APPLEROMAN, "Monaco", 12},
  {&NewYorkRegularRNormal12909696P51APPLEROMAN, "New York", 9},
  {&NewYorkRegularRNormal151209090P70APPLEROMAN, "New York", 12},
  {&NewYorkRegularRNormal171408787P82APPLEROMAN, "New York", 14},
  {&OhlfsRegularRNormal11907575P50ISO88591, "Ohlfs", 9},
  {&OhlfsRegularRNormal111007575P60ISO88591, "Ohlfs", 10},
  {&OhlfsRegularRNormal151207575P80ISO88591, "Ohlfs", 12},
  {&OhlfsRegularRNormal181607575P110ISO88591, "Ohlfs", 16},
  {&OnyxRegularRNormal8807272C70ISO88591, "Onyx", 8},
  {&OpalRegularRNormal121207272P92ISO88591, "Opal", 12},
  {&PalatinoRegularRNormal141208484P68APPLEROMAN, "Palatino", 12},
  {&PalatinoRegularRNormal161408282P79APPLEROMAN, "Palatino", 14},
  {&Palmos30EnRegularRNormal111107272P61FONTSPECIFIC0, "Palmos30-En", 11},
  {&Palmos30EnRegularRNormal111107272P52FONTSPECIFIC0, "Palmos30-En", 11},
  {&PeridotRegularRNormal7707272P44ISO88591, "Peridot", 7},
  {&PoiskRegularRNormal8807272C80ISO106461, "Poisk", 8},
  {&RevapalRegularRNormal8807272C80ATASCII0, "Revapal", 8},
  {&RoundedRegularRNormal8807272C80AMSTRADCPMPLUS, "Rounded", 8},
  {&RubyRegularRNormal121207272P101ISO88591, "Ruby", 12},
  {&Saa5056RegularRNormal9907272C60FONTSPECIFIC0, "Saa5056", 9},
  {&SerifRegularRNormal8807272C80AMSTRADCPMPLUS, "Serif", 8},
  {&Square1RegularRNormal8807272C80AMSTRADCPMPLUS, "Square1", 8},
  {&Square2RegularRNormal8807272C80AMSTRADCPMPLUS, "Square2", 8},
  {&SysfontRegularRNormal121207272P68FONTSPECIFIC0, "Sysfont", 12},
  {&SystemRegularRNormal141208484P76APPLEROMAN, "System", 12},
  {&System08RegularRNormal6805454C60ATARIST, "System08", 8},
  {&TerminalRegularRNormal81209672C80IBMCP437, "Terminal", 12},
  {&TerminalRegularRNormal129009696C50IBMCP850, "Terminal", 90},
  {&Tile7R20SRegularRNormal9907272P50FONTSPECIFIC0, "Tile7R20S", 9},
  {&TmsRmnRegularRMedium1380120120P72IBMUGL, "Tms Rmn", 8},
  {&TmsRmnRegularRMedium11809696P59IBMUGL, "Tms Rmn", 8},
  {&TmsRmnRegularRMedium131009696P72IBMUGL, "Tms Rmn", 10},
  {&TorontoRegularRNormal151209090P81APPLEROMAN, "Toronto", 12},
  {&UniversityRegularRNormal101007272P52FONTSPECIFIC0, "University", 10},
  {&UniversityRegularRNormal121207272P59FONTSPECIFIC0, "University", 12},
};
