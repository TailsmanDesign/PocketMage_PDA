//  ooooooooooooo ooooooo  ooooo ooooooooooooo  //
//  8'   888   `8  `8888    d8'  8'   888   `8  //
//       888         Y888..8P         888       //
//       888          `8888'          888       //
//       888         .8PY888.         888       //
//       888        d8'  `888b        888       //
//      o888o      o888o  o88888o      o888o    //

#include <globals.h>
#if !OTA_APP  // POCKETMAGE_OS
static constexpr const char* TAG = "TXT_NEW";

#pragma region Font includes
// Mono
#include <Fonts/FreeMono9pt8b.h>
#include <Fonts/FreeMonoBold12pt8b.h>
#include <Fonts/FreeMonoBold18pt8b.h>
#include <Fonts/FreeMonoBold24pt8b.h>
#include <Fonts/FreeMonoBold9pt8b.h>
#include <Fonts/FreeMonoBoldOblique12pt8b.h>
#include <Fonts/FreeMonoBoldOblique18pt8b.h>
#include <Fonts/FreeMonoBoldOblique24pt8b.h>
#include <Fonts/FreeMonoBoldOblique9pt8b.h>
#include <Fonts/FreeMonoOblique9pt8b.h>

// Serif
#include <Fonts/FreeSerif9pt8b.h>
#include <Fonts/FreeSerifBold12pt8b.h>
#include <Fonts/FreeSerifBold18pt8b.h>
#include <Fonts/FreeSerifBold24pt8b.h>
#include <Fonts/FreeSerifBold9pt8b.h>
#include <Fonts/FreeSerifBoldItalic12pt8b.h>
#include <Fonts/FreeSerifBoldItalic18pt8b.h>
#include <Fonts/FreeSerifBoldItalic24pt8b.h>
#include <Fonts/FreeSerifBoldItalic9pt8b.h>
#include <Fonts/FreeSerifItalic9pt8b.h>

// Sans
#include <Fonts/FreeSans9pt8b.h>
#include <Fonts/FreeSansBold12pt8b.h>
#include <Fonts/FreeSansBold18pt8b.h>
#include <Fonts/FreeSansBold24pt8b.h>
#include <Fonts/FreeSansBold9pt8b.h>
#include <Fonts/FreeSansBoldOblique12pt8b.h>
#include <Fonts/FreeSansBoldOblique18pt8b.h>
#include <Fonts/FreeSansBoldOblique24pt8b.h>
#include <Fonts/FreeSansBoldOblique9pt8b.h>
#include <Fonts/FreeSansOblique9pt8b.h>

#include "esp32-hal-log.h"
#include "esp_log.h"

// ------------------ General ------------------
enum TXTState_NEW { TXT_, FONT, SAVE_AS, LOAD_FILE, JOURNAL_MODE, NEW_FILE };
TXTState_NEW CurrentTXTState_NEW = TXT_;

// ------------------ Fonts ------------------
#define SPECIAL_PADDING 20      // Padding for lists, code blocks, quote blocks
#define SPACEWIDTH_SYMBOL "n"   // n is roughly the width of a space
#define WORDWIDTH_BUFFER 0      // Add extra spacing to each word
#define DISPLAY_WIDTH_BUFFER 0  // Add margin for text wrap calc
#define HEADING_LINE_PADDING 8  // Padding between each line
#define NORMAL_LINE_PADDING 2

#pragma region Fonts
#include <gfxfont.h>

// --- UTF-8 Decoding System ---
inline uint16_t decodeUTF8(const char* str, uint16_t* index, uint16_t len) {
  if (*index >= len) return 0;
  uint8_t c = str[*index];
  (*index)++;
  
  if (c < 0x80) return c; // Standard ASCII
  if ((c & 0xE0) == 0xC0) { // 2-byte sequence
    if (*index >= len) return c; // Incomplete, fallback safely
    uint8_t c2 = str[*index]; (*index)++;
    return ((c & 0x1F) << 6) | (c2 & 0x3F);
  }
  if ((c & 0xF0) == 0xE0) { // 3-byte sequence
    if (*index + 1 >= len) return c;
    uint8_t c2 = str[*index]; (*index)++;
    uint8_t c3 = str[*index]; (*index)++;
    return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
  }
  return c; // Fallback
}

// Maps standard Unicode points to your custom font's 0x20-0xDF format
inline uint8_t mapUnicodeToFontIndex(uint16_t unicode) {
  if (unicode < 0x80) return unicode;
  if (unicode >= 0x00A0 && unicode <= 0x00FF) return unicode - 0x20;
  return 0x7F; // Replacement character for unsupported glyphs like 'ł' (U+0142)
}

// Fast character width lookup (Modified for mapped 8-bit glyphs)
inline uint16_t getFastCharWidth(uint8_t c, const GFXfont* font) {
  if (!font) return 6; // Fallback for default 5x7 font
  if (c == ' ') c = 'n'; 
  
  if (c >= pgm_read_byte(&font->first) && c <= pgm_read_byte(&font->last)) {
    GFXglyph *glyph = &(((GFXglyph *)pgm_read_ptr(&font->glyph))[c - pgm_read_byte(&font->first)]);
    return pgm_read_byte(&glyph->width); 
  }
  return 0; 
}

// Fast line height lookup 
inline uint16_t getFastCharHeight(const GFXfont* font) {
  if (!font) return 8;
  if ('A' >= pgm_read_byte(&font->first) && 'A' <= pgm_read_byte(&font->last)) {
    GFXglyph *glyph = &(((GFXglyph *)pgm_read_ptr(&font->glyph))['A' - pgm_read_byte(&font->first)]);
    return pgm_read_byte(&glyph->height);
  }
  return pgm_read_byte(&font->yAdvance) / 2;
}

// Font setup
enum FontFamily { serif = 0, sans = 1, mono = 2 };
uint8_t fontStyle = sans;

struct FontMap {
  const GFXfont* normal;
  const GFXfont* normal_B;
  const GFXfont* normal_I;
  const GFXfont* normal_BI;

  const GFXfont* h1;
  const GFXfont* h1_B;
  const GFXfont* h1_I;
  const GFXfont* h1_BI;

  const GFXfont* h2;
  const GFXfont* h2_B;
  const GFXfont* h2_I;
  const GFXfont* h2_BI;

  const GFXfont* h3;
  const GFXfont* h3_B;
  const GFXfont* h3_I;
  const GFXfont* h3_BI;

  const GFXfont* code;
  const GFXfont* code_B;
  const GFXfont* code_I;
  const GFXfont* code_BI;

  const GFXfont* quote;
  const GFXfont* quote_B;
  const GFXfont* quote_I;
  const GFXfont* quote_BI;

  const GFXfont* list;
  const GFXfont* list_B;
  const GFXfont* list_I;
  const GFXfont* list_BI;
};

FontMap fonts[3];

void setFontStyle(FontFamily f) {
  fontStyle = f;
}

// ------------------ Document Variables ------------------
static bool updateScreen = false;
static ulong currentLineNum = 0;
static ulong topVisibleLine = 0;

#if BOARD_HAS_PSRAM
  #define MAX_LINES 4000
#else
  #define MAX_LINES 1000
#endif

#define LINE_CAP 64       

struct Line {
  char type = ' ';          
  char text[LINE_CAP + 1];  
  uint16_t len;             
};

struct Document {
  Line lines[MAX_LINES];
  ulong lineCount;
};

Document* doc_ptr = nullptr;
#define document (*doc_ptr)

void initLine(Line& line) {
  line.type = ' ';
  line.len = 0;
  line.text[0] = '\0';
}

void initDocMemory() {
  if (doc_ptr == nullptr) {
    #if BOARD_HAS_PSRAM
      doc_ptr = (Document*) ps_malloc(sizeof(Document));
      ESP_LOGI(TAG, "Allocated document memory in PSRAM");
      if (doc_ptr == nullptr) {
        ESP_LOGW(TAG, "Fallback to normal RAM");
        doc_ptr = (Document*) malloc(sizeof(Document));
      }
    #else
      doc_ptr = (Document*) malloc(sizeof(Document));
    #endif

    if (doc_ptr != nullptr) {
      doc_ptr->lineCount = 0;
      for (int i = 0; i < MAX_LINES; i++) {
        initLine(doc_ptr->lines[i]);
      }
    } else {
      ESP_LOGE(TAG, "FATAL: Failed to allocate memory for TXT Document!");
    }
  }
}

#pragma region Editor Helpers
// Helpers
const GFXfont* pickFont(char style, bool bold, bool italic) {
  FontMap& fm = fonts[fontStyle];  

  switch (style) {
    case '1':  // H1
      if (bold && italic) return fm.h1_BI;
      if (bold) return fm.h1_B;
      if (italic) return fm.h1_I;
      return fm.h1;

    case '2':  // H2
      if (bold && italic) return fm.h2_BI;
      if (bold) return fm.h2_B;
      if (italic) return fm.h2_I;
      return fm.h2;

    case '3':  // H3
      if (bold && italic) return fm.h3_BI;
      if (bold) return fm.h3_B;
      if (italic) return fm.h3_I;
      return fm.h3;

    case '>':  // Quote
      if (bold && italic) return fm.quote_BI;
      if (bold) return fm.quote_B;
      if (italic) return fm.quote_I;
      return fm.quote;

    case 'U':  // Unordered List
    case 'u':  // Unordered List (Continuation)
    case 'O':  // Ordered List
    case 'o':  // Ordered List (Continuation)
      if (bold && italic) return fm.list_BI;
      if (bold) return fm.list_B;
      if (italic) return fm.list_I;
      return fm.list;

    case 'C':  // Code
      if (bold && italic) return fm.code_BI;
      if (bold) return fm.code_B;
      if (italic) return fm.code_I;
      return fm.code;

    default:  // Normal
      if (bold && italic) return fm.normal_BI;
      if (bold) return fm.normal_B;
      if (italic) return fm.normal_I;
      return fm.normal;
  }
}

uint16_t getLineMaxHeight(Line& line) {
  bool boldExists = false;
  bool italicExists = false;
  bool boldItalicExists = false;

  if (line.type == ' ') return 0;

  for (uint16_t i = 0; i < line.len; i++) {
    if (line.text[i] == '*') {
      if (i == 0 || line.text[i - 1] != '*') {
        uint8_t starCount = 0;
        while (i + starCount < line.len && line.text[i + starCount] == '*' && starCount < 3) {
          starCount++;
        }
        if (starCount == 1) italicExists = true;
        else if (starCount == 2) boldExists = true;
        else if (starCount == 3) boldItalicExists = true;
      }
    }
  }

  const GFXfont* font;
  if (boldItalicExists) font = pickFont(line.type, true, true);
  else if (boldExists) font = pickFont(line.type, true, false);
  else if (italicExists) font = pickFont(line.type, false, true);
  else font = pickFont(line.type, false, false);

  return getFastCharHeight(font); 
}

int getCalculatedLineHeight(Line& line) {
  if (line.type == 'H') return 8;  
  if (line.type == 'B') return 8;  

  int h = getLineMaxHeight(line);
  
  if (line.type == '1' || line.type == '2' || line.type == '3') {
    return h + HEADING_LINE_PADDING;
  }
  
  return h + NORMAL_LINE_PADDING;
}

int drawLineEink(Document& doc, ulong lineIndex, int startX, int startY, bool isSelected) {
  Line& line = doc.lines[lineIndex]; 
  
  char style = line.type;
  int cursorY = startY;

  uint16_t fgColor;
  uint16_t bgColor;

  if (style == 'B') {
    fgColor = GxEPD_BLACK;
    bgColor = GxEPD_WHITE;
  }
  else {
    fgColor = isSelected ? GxEPD_WHITE : GxEPD_BLACK;
    bgColor = isSelected ? GxEPD_BLACK : GxEPD_WHITE;
  }

  int hpx = getCalculatedLineHeight(line);

  if (isSelected) {
    display.fillRect(0, startY, display.width(), hpx, bgColor);
  }

  display.setTextColor(fgColor);

  if (style == 'H') {
    display.drawFastHLine(0, cursorY + 3, display.width(), fgColor);
    display.drawFastHLine(0, cursorY + 4, display.width(), fgColor);
    return hpx;
  }
  else if (style == 'B') {
    return 8;
  }

  if (style == '>')
    startX += SPECIAL_PADDING;
  else if (style == 'U' || style == 'O' || style == 'u' || style == 'o')
    startX += 2 * SPECIAL_PADDING;
  else if (style == 'C')
    startX += (SPECIAL_PADDING / 2);

  // ---------- Render Text with UTF-8 Support ---------- //
  bool bold = false;
  bool italic = false;
  int xpos = startX; 
  int baselineY = cursorY + getLineMaxHeight(line); 

  uint16_t i = 0;
  while (i < line.len) {
    if (xpos > display.width()) break;
    
    // Decode UTF-8 string into Unicode Code Point (Advances i automatically)
    uint16_t unicode = decodeUTF8(line.text, &i, line.len);

    if (unicode == '*') {
      // Toggle formatting
      int starCount = 1;
      while (i < line.len && line.text[i] == '*' && starCount < 3) {
        starCount++;
        i++;
      }
      switch (starCount) {
        case 1: italic = !italic; break;
        case 2: bold = !bold; break;
        case 3: bold = !bold; italic = !italic; break;
      }
      continue;
    }

    const GFXfont* font = pickFont(style, bold, italic);
    display.setFont(font);

    // Map Unicode code point to the 8-bit glyph index in the custom font
    uint8_t mappedChar = mapUnicodeToFontIndex(unicode);

    display.setCursor(xpos, baselineY); 
    display.write(mappedChar); // Write natively supports raw glyph indices

    xpos += getFastCharWidth(mappedChar, font);
  }

  // ---------- Post-Render Formatting ---------- //
  if (style == '>') {
    display.drawFastVLine(SPECIAL_PADDING / 2, startY, hpx, fgColor);
    display.drawFastVLine((SPECIAL_PADDING / 2) + 1, startY, hpx, fgColor);
  }
  else if (style == 'C') {
    display.drawFastVLine(SPECIAL_PADDING / 4, startY, hpx, fgColor);
    display.drawFastVLine(display.width() - (SPECIAL_PADDING / 4), startY, hpx, fgColor);
    display.drawFastVLine((SPECIAL_PADDING / 4) + 1, startY, hpx, fgColor);
    display.drawFastVLine(display.width() - (SPECIAL_PADDING / 4) - 1, startY, hpx, fgColor);
  }
  else if (style == '1' || style == '2' || style == '3') {
    bool isLastLineOfHeader = true;
    if (lineIndex < doc.lineCount - 1) {
       if (doc.lines[lineIndex + 1].type == style) {
           isLastLineOfHeader = false; 
       }
    }
    
    if (isLastLineOfHeader) {
      display.drawFastHLine(0, startY + hpx - 2, display.width(), fgColor);
      display.drawFastHLine(0, startY + hpx - 3, display.width(), fgColor);
    }
  }
  else if (style == 'U') {
    display.fillCircle(startX - 8, startY + (hpx / 2), 3, fgColor);
  }
  else if (style == 'O') {
    int listNum = 1;
    if (lineIndex > 0) {
      for (long k = lineIndex - 1; k >= 0; k--) {
        if (doc.lines[k].type == 'O') listNum++;
        else if (doc.lines[k].type != 'o') break; 
      }
    }

    String numStr = String(listNum) + ".";
    display.setFont(pickFont('O', false, false)); 
    
    int16_t nx, ny;
    uint16_t nw, nh;
    display.getTextBounds(numStr, 0, 0, &nx, &ny, &nw, &nh);
    
    display.setCursor(startX - nw - 8, baselineY);
    display.print(numStr);
  }

  return hpx; 
}

void insertLineArray(ulong index) {
  if (document.lineCount >= MAX_LINES) return; 
  if (index < document.lineCount) {
    memmove(&document.lines[index + 1], &document.lines[index], sizeof(Line) * (document.lineCount - index));
  }
  initLine(document.lines[index]);
  document.lineCount++;
}

void deleteLineArray(ulong index) {
  if (index >= document.lineCount) return;
  if (index < document.lineCount - 1) {
    memmove(&document.lines[index], &document.lines[index + 1], sizeof(Line) * (document.lineCount - index - 1));
  }
  document.lineCount--;
}

void deleteLinesMultiple(ulong index, int count) {
  if (count <= 0 || index >= document.lineCount) return;
  int actualCount = count;
  if (index + count > document.lineCount) {
    actualCount = document.lineCount - index;
  }
  if (index + actualCount < document.lineCount) {
    memmove(&document.lines[index], &document.lines[index + actualCount], sizeof(Line) * (document.lineCount - index - actualCount));
  }
  document.lineCount -= actualCount;
}

int getLineEinkWidth(Line& line) {
  int width = 0;
  char style = line.type;

  // 1. Account for the structural left-side padding
  if (style == '>') width += SPECIAL_PADDING;
  else if (style == 'U' || style == 'O' || style == 'u' || style == 'o') width += 2 * SPECIAL_PADDING;
  else if (style == 'C') width += (SPECIAL_PADDING / 2);

  // 2. Iterate through the UTF-8 text and measure glyphs
  bool bold = false;
  bool italic = false;
  uint16_t i = 0;

  while (i < line.len) {
    uint16_t unicode = decodeUTF8(line.text, &i, line.len);

    // Handle inline formatting toggles (stars have 0 visual width)
    if (unicode == '*') {
      int starCount = 1;
      while (i < line.len && line.text[i] == '*' && starCount < 3) {
        starCount++; 
        i++;
      }
      switch (starCount) {
        case 1: italic = !italic; break;
        case 2: bold = !bold; break;
        case 3: bold = !bold; italic = !italic; break;
      }
      continue; 
    }

    // Determine active font and map to your custom 8-bit glyph index
    const GFXfont* font = pickFont(style, bold, italic);
    uint8_t mappedChar = mapUnicodeToFontIndex(unicode);

    // Add the exact pixel width of this specific character
    width += getFastCharWidth(mappedChar, font);
  }

  // 3. Account for the structural right-side padding (Code blocks have right-side borders)
  if (style == 'C') width += (SPECIAL_PADDING / 2);

  return width;
}

int findWrapIndex(const String& content, int startIndex, char style) {
  int availableWidth = display.width() - DISPLAY_WIDTH_BUFFER;

  if (style == '>') availableWidth -= SPECIAL_PADDING;
  else if (style == 'U' || style == 'O' || style == 'u' || style == 'o') availableWidth -= 2 * SPECIAL_PADDING;
  else if (style == 'C') availableWidth -= (SPECIAL_PADDING / 2);

  bool bold = false;
  bool italic = false;
  int currentWidth = 0;
  int lastSpaceIndex = -1;

  int maxLen = min((int)(content.length() - startIndex), (int)LINE_CAP);
  const GFXfont* activeFont = pickFont(style, bold, italic);

  uint16_t i = 0;
  while (i < maxLen) {
    uint16_t charStart = i;
    uint16_t unicode = decodeUTF8(content.c_str() + startIndex, &i, maxLen);

    if (unicode == ' ') lastSpaceIndex = charStart;

    if (unicode == '*') {
      uint8_t starCount = 1;
      while (i < maxLen && content[startIndex + i] == '*' && starCount < 3) {
        starCount++; i++;
      }
      switch (starCount) {
        case 1: italic = !italic; break;
        case 2: bold = !bold; break;
        case 3: bold = !bold; italic = !italic; break;
      }
      activeFont = pickFont(style, bold, italic);
      continue; 
    }

    uint8_t mappedChar = mapUnicodeToFontIndex(unicode);
    currentWidth += getFastCharWidth(mappedChar, activeFont);

    if (currentWidth > availableWidth) {
      if (lastSpaceIndex > 0) return lastSpaceIndex;
      return charStart > 0 ? charStart : 1; 
    }
  }

  if (maxLen < (int)(content.length() - startIndex) && lastSpaceIndex > 0) {
    return lastSpaceIndex;
  }
  return maxLen; 
}

void reflowParagraph(ulong startLine, uint16_t& activeCursor) {
  char baseStyle = document.lines[startLine].type;
  char contStyle = baseStyle;
  
  if (baseStyle == 'U' || baseStyle == 'u') contStyle = 'u';
  else if (baseStyle == 'O' || baseStyle == 'o') contStyle = 'o';

  String fullText;
  fullText.reserve(512); 
  fullText = document.lines[startLine].text;
  
  ulong endLine = startLine + 1;
  while (endLine < document.lineCount && document.lines[endLine].type == contStyle) {
    if (document.lines[endLine].len > 0) {
      fullText += " ";
      fullText += document.lines[endLine].text;
    }
    endLine++;
  }
  
  int absoluteCursor = activeCursor; 
  ulong currWriteIdx = startLine;
  
  int textIndex = 0;
  int textLen = fullText.length();
  
  while (textIndex < textLen) {
    char currentStyle = (currWriteIdx == startLine) ? baseStyle : contStyle;
    
    int wrapLen = findWrapIndex(fullText, textIndex, currentStyle);
    
    String chunk = fullText.substring(textIndex, textIndex + wrapLen);
    textIndex += wrapLen;
    
    if (currWriteIdx >= endLine) {
      insertLineArray(currWriteIdx);
      endLine++;
    }
    
    Line& writeLine = document.lines[currWriteIdx];
    writeLine.type = currentStyle;
    strncpy(writeLine.text, chunk.c_str(), LINE_CAP);
    writeLine.text[LINE_CAP] = '\0';
    writeLine.len = strlen(writeLine.text);
    
    if (absoluteCursor != -1) {
      if (absoluteCursor <= writeLine.len) {
        currentLineNum = currWriteIdx;
        activeCursor = absoluteCursor;
        absoluteCursor = -1; 
      } else {
        absoluteCursor -= writeLine.len;
      }
    }
    
    if (textIndex < textLen && fullText[textIndex] == ' ') {
        textIndex++;
        if (absoluteCursor > 0) absoluteCursor--; 
    }
    
    currWriteIdx++;
  }
  
  if (currWriteIdx == startLine) {
    Line& writeLine = document.lines[currWriteIdx];
    writeLine.type = baseStyle;
    writeLine.text[0] = '\0';
    writeLine.len = 0;
    
    if (absoluteCursor != -1) {
      currentLineNum = currWriteIdx;
      activeCursor = 0;
    }
    currWriteIdx++; 
  }
  
  if (endLine > currWriteIdx) {
    deleteLinesMultiple(currWriteIdx, endLine - currWriteIdx);
  }
}

void mergeLinesUp(ulong currLine, uint16_t& cursor) {
  if (currLine == 0) return;
  ulong prevLine = currLine - 1;
  char pType = document.lines[prevLine].type;
  
  if (pType == 'B' || pType == 'H') {
    deleteLineArray(prevLine);
    currentLineNum--;
    updateScreen = true;
    return;
  }
  
  char contStyle = pType; 
  if (pType == 'U' || pType == 'u') contStyle = 'u';
  else if (pType == 'O' || pType == 'o') contStyle = 'o';

  String fullText;
  fullText.reserve(512);
  fullText = document.lines[prevLine].text;
  cursor = fullText.length(); 
  fullText += document.lines[currLine].text;
  
  char currBase = document.lines[currLine].type;
  char currContStyle = currBase;
  if (currBase == 'U' || currBase == 'u') currContStyle = 'u';
  else if (currBase == 'O' || currBase == 'o') currContStyle = 'o';

  ulong endLine = currLine + 1;
  while (endLine < document.lineCount && document.lines[endLine].type == currContStyle) {
    if (document.lines[endLine].len > 0) {
      fullText += " ";
      fullText += document.lines[endLine].text;
    }
    endLine++;
  }
  
  deleteLinesMultiple(currLine, endLine - currLine);
  
  ulong currWriteIdx = prevLine;
  int absoluteCursor = cursor;
  
  int textIndex = 0;
  int textLen = fullText.length();
  
  while (textIndex < textLen) {
    char currentStyle = (currWriteIdx == prevLine) ? pType : contStyle;
    int wrapLen = findWrapIndex(fullText, textIndex, currentStyle);
    
    String chunk = fullText.substring(textIndex, textIndex + wrapLen);
    textIndex += wrapLen;
    
    if (currWriteIdx >= document.lineCount || currWriteIdx > prevLine) {
      insertLineArray(currWriteIdx);
    }
    
    Line& writeLine = document.lines[currWriteIdx];
    writeLine.type = currentStyle;
    strncpy(writeLine.text, chunk.c_str(), LINE_CAP);
    writeLine.text[LINE_CAP] = '\0';
    writeLine.len = strlen(writeLine.text);
    
    if (absoluteCursor != -1) {
      if (absoluteCursor <= writeLine.len) {
        currentLineNum = currWriteIdx;
        cursor = absoluteCursor;
        absoluteCursor = -1;
      } else {
        absoluteCursor -= writeLine.len;
      }
    }
    
    if (textIndex < textLen && fullText[textIndex] == ' ') {
        textIndex++;
        if (absoluteCursor > 0) absoluteCursor--; 
    }
    
    currWriteIdx++;
  }
  
  if (currWriteIdx == prevLine) {
    Line& writeLine = document.lines[currWriteIdx];
    writeLine.type = pType;
    writeLine.text[0] = '\0';
    writeLine.len = 0;
    currentLineNum = currWriteIdx;
    cursor = 0;
  }
  
  updateScreen = true;
}

inline void insertChar(Line& line, uint16_t& cursor, char c) {
  if (line.len >= LINE_CAP) return;

  if (cursor < line.len) {
    memmove(&line.text[cursor + 1], &line.text[cursor], line.len - cursor + 1);
  } else {
    line.text[line.len] = c;
    line.len++;
    line.text[line.len] = '\0';
    cursor++;
    return;
  }

  line.text[cursor] = c;
  line.len++;
  cursor++;
}

inline void deleteChar(Line& line, uint16_t& cursor) {
  if (cursor == 0) return;

  uint16_t old_cursor = cursor;
  // Safely leap over UTF-8 continuation bytes to delete the entire glyph
  do { cursor--; } while (cursor > 0 && (line.text[cursor] & 0xC0) == 0x80);
  
  uint16_t bytesToDelete = old_cursor - cursor;
  memmove(&line.text[cursor], &line.text[old_cursor], line.len - old_cursor + 1);
  line.len -= bytesToDelete;
}

void cycleTextStyle(Line& line, uint16_t& cursor) {
  if (line.len == 0) return;

  int leftBound = -1;
  int rightBound = -1;
  uint8_t currentStars = 0;
  uint8_t leftStars = 0;
  uint8_t rightStars = 0;

  int i = 0;
  int activeStart = -1;
  uint8_t activeStars = 0;

  while (i < line.len) {
    if (line.text[i] == '*') {
      int startIdx = i;
      uint8_t stars = 0;
      while (i < line.len && line.text[i] == '*' && stars < 3) {
        stars++;
        i++;
      }

      if (activeStars == 0) {
        activeStart = startIdx;
        activeStars = stars;
      } else if (activeStars == stars) {
        if (cursor >= activeStart && cursor <= i) {
          leftBound = activeStart;
          rightBound = i;
          currentStars = activeStars;
          leftStars = activeStars;
          rightStars = activeStars;
          break;
        }
        activeStars = 0;
        activeStart = -1;
      } else {
        activeStart = startIdx;
        activeStars = stars;
      }
    } else {
      i++;
    }
  }

  if (leftBound == -1) {
    leftBound = cursor;
    while (leftBound > 0 && line.text[leftBound - 1] != ' ') {
      leftBound--;
    }
    rightBound = cursor;
    while (rightBound < line.len && line.text[rightBound] != ' ') {
      rightBound++;
    }
    currentStars = 0;
    leftStars = 0;
    rightStars = 0;
  }

  uint8_t nextStars = (currentStars + 1) % 4; 
  int sizeDiff = (nextStars * 2) - (leftStars + rightStars);
  if (line.len + sizeDiff > LINE_CAP) return;

  int contentStart = leftBound + leftStars;
  int contentEnd = rightBound - rightStars;

  int cursorOffset = cursor - contentStart;
  if (cursorOffset < 0) cursorOffset = 0; 
  if (cursorOffset > (contentEnd - contentStart)) cursorOffset = contentEnd - contentStart; 

  if (rightStars > 0) {
    memmove(&line.text[rightBound - rightStars], &line.text[rightBound],
            line.len - rightBound + 1); 
    line.len -= rightStars;
    rightBound -= rightStars;
  }

  if (leftStars > 0) {
    memmove(&line.text[leftBound], &line.text[leftBound + leftStars],
            line.len - leftBound - leftStars + 1);
    line.len -= leftStars;
    rightBound -= leftStars;
  }

  if (nextStars > 0) {
    memmove(&line.text[rightBound + nextStars], &line.text[rightBound], line.len - rightBound + 1);
    for (int j = 0; j < nextStars; j++) line.text[rightBound + j] = '*';
    line.len += nextStars;

    memmove(&line.text[leftBound + nextStars], &line.text[leftBound], line.len - leftBound + 1);
    for (int j = 0; j < nextStars; j++) line.text[leftBound + j] = '*';
    line.len += nextStars;
  }

  cursor = leftBound + nextStars + cursorOffset;
}

void cycleParagraphStyle(ulong& currLine, uint16_t& cursor) {
  char activeType = document.lines[currLine].type;
  ulong startLine = currLine;
  char oldContStyle = activeType;

  if (activeType == 'u' || activeType == 'U') {
    oldContStyle = 'u';
    while (startLine > 0 && document.lines[startLine].type == 'u') startLine--;
  } else if (activeType == 'o' || activeType == 'O') {
    oldContStyle = 'o';
    while (startLine > 0 && document.lines[startLine].type == 'o') startLine--;
  } else {
    oldContStyle = activeType;
    while (startLine > 0 && document.lines[startLine - 1].type == activeType) {
      startLine--;
    }
  }

  static const char styleCycle[] = {'T', '1', '2', '3', '>', 'O', 'U', 'C', 'H'};
  static const int numStyles = sizeof(styleCycle) / sizeof(styleCycle[0]);
  
  char oldBaseStyle = document.lines[startLine].type;
  int currentIndex = 0;
  for (int i = 0; i < numStyles; i++) {
    if (oldBaseStyle == styleCycle[i]) {
      currentIndex = i;
      break;
    }
  }
  char newBaseStyle = styleCycle[(currentIndex + 1) % numStyles];
  
  char newContStyle = newBaseStyle;
  if (newBaseStyle == 'U') newContStyle = 'u';
  else if (newBaseStyle == 'O') newContStyle = 'o';

  int absoluteCursor = 0;
  String fullText;
  fullText.reserve(512); 
  ulong endLine = startLine;
  
  while (endLine < document.lineCount) {
    if (endLine > startLine && document.lines[endLine].type != oldContStyle) break;
    if (endLine == currLine) {
      absoluteCursor = fullText.length() + (fullText.length() > 0 ? 1 : 0) + cursor;
    }
    if (document.lines[endLine].len > 0) {
      if (fullText.length() > 0) fullText += " ";
      fullText += document.lines[endLine].text;
    }
    endLine++;
  }

  ulong currWriteIdx = startLine;
  int textIndex = 0;
  int textLen = fullText.length();
  
  while (textIndex < textLen) {
    char currentStyle = (currWriteIdx == startLine) ? newBaseStyle : newContStyle;
    int wrapLen = findWrapIndex(fullText, textIndex, currentStyle);
    
    String chunk = fullText.substring(textIndex, textIndex + wrapLen);
    textIndex += wrapLen;
    
    if (currWriteIdx >= endLine) {
      insertLineArray(currWriteIdx);
      endLine++;
    }
    
    Line& writeLine = document.lines[currWriteIdx];
    writeLine.type = currentStyle;
    strncpy(writeLine.text, chunk.c_str(), LINE_CAP);
    writeLine.text[LINE_CAP] = '\0';
    writeLine.len = strlen(writeLine.text);
    
    if (absoluteCursor != -1) {
      if (absoluteCursor <= writeLine.len) {
        currLine = currWriteIdx;
        cursor = absoluteCursor;
        absoluteCursor = -1;
      } else {
        absoluteCursor -= writeLine.len;
      }
    }
    
    if (textIndex < textLen && fullText[textIndex] == ' ') {
        textIndex++;
        if (absoluteCursor > 0) absoluteCursor--; 
    }
    currWriteIdx++;
  }
  
  if (currWriteIdx == startLine) {
    Line& writeLine = document.lines[currWriteIdx];
    writeLine.type = newBaseStyle;
    writeLine.text[0] = '\0';
    writeLine.len = 0;
    currLine = startLine;
    cursor = 0;
    currWriteIdx++;
  }

  if (endLine > currWriteIdx) {
    deleteLinesMultiple(currWriteIdx, endLine - currWriteIdx);
  }
  
  updateScreen = true;
}

void setFontOLED(bool bold, bool italic) {
  if (bold && italic)
    u8g2.setFont(u8g2_font_luBIS18_tf);  
  else if (bold && !italic)
    u8g2.setFont(u8g2_font_luBS18_tf);  
  else if (!bold && italic)
    u8g2.setFont(u8g2_font_luIS18_tf);  
  else
    u8g2.setFont(u8g2_font_lubR18_tf);  
  return;
}

void toolBar(Line& line, bool bold, bool italic) {
  u8g2.setFont(u8g2_font_5x7_tf);

  switch (KB().getKeyboardState()) {
    case 1:
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("SHIFT")) / 2,
                   u8g2.getDisplayHeight(), "SHIFT");
      break;
    case 2:
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("FN")) / 2, u8g2.getDisplayHeight(),
                   "FN");
      break;
    case 3:
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("FN+SHIFT")) / 2,
                   u8g2.getDisplayHeight(), "FN+SHIFT");
    default:
      break;
  }

  char currentDocLineType = line.type;
  String lineTypeLabel;

  switch (currentDocLineType) {
    case ' ': lineTypeLabel = " "; break;
    case 'T': lineTypeLabel = "BODY"; break;
    case '1': lineTypeLabel = "H1"; break;
    case '2': lineTypeLabel = "H2"; break;
    case '3': lineTypeLabel = "H3"; break;
    case 'C': lineTypeLabel = "CODE"; break;
    case '>': lineTypeLabel = "QUOTE"; break;
    case 'U':
    case 'u': lineTypeLabel = "U LIST"; break;
    case 'O':
    case 'o': lineTypeLabel = "O LIST"; break;
    case 'H': lineTypeLabel = "H RULE"; break;
    case 'B': lineTypeLabel = "BLANK LINE"; break;
    default:  lineTypeLabel = ""; break;  
  }

  if (lineTypeLabel.length() > 0) {
    u8g2.drawStr(0, u8g2.getDisplayHeight(), lineTypeLabel.c_str());
  }

  if (bold == true && italic == true) {
    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth("BOLD+ITALIC"), u8g2.getDisplayHeight(),
                 "BOLD+ITALIC");
  } else if (bold == true && italic == false) {
    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth("BOLD"), u8g2.getDisplayHeight(),
                 "BOLD");
  } else if (bold == false && italic == true) {
    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth("ITALIC"), u8g2.getDisplayHeight(),
                 "ITALIC");
  } else {
    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth("NORMAL"), u8g2.getDisplayHeight(),
                 "NORMAL");
  }
}

uint8_t getFastOledCharWidth(uint16_t unicode, bool bold, bool italic, bool isTiny);

void displayScrollPreviewOLED(Document& doc, ulong activeCursorLine) {
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);

  int startX = 0; 
  int cursorY = 0;
  int specialPadding = 8; 
  
  ulong displayTop = 0;
  if (activeCursorLine > 3) {
    displayTop = activeCursorLine - 3;
  }

  u8g2.drawVLine(76, 0, u8g2.getDisplayHeight());

  for (ulong i = displayTop; i < doc.lineCount; i++) {
    if (cursorY > u8g2.getDisplayHeight()) break; 

    Line& line = doc.lines[i];
    char style = line.type;
    int padX = startX;

    int max_hpx = 2;
    switch (style) {
      case '1': max_hpx = 5; break;
      case '2': max_hpx = 4; break;
      case '3': max_hpx = 3; break;
      case 'T': max_hpx = 2; break;
      case 'C': max_hpx = 2; padX += (specialPadding / 2); break;
      case '>': max_hpx = 2; padX += specialPadding; break;
      case 'U': 
      case 'u': 
      case 'O': 
      case 'o': 
        max_hpx = 2; 
        padX += specialPadding; 
        break;
      case 'H': 
        if (cursorY > 0) {
          u8g2.drawHLine(startX, cursorY, 74);
          cursorY += 3;
        }
        continue;
      case 'B': 
        cursorY += 4; 
        continue; 
      default: 
        max_hpx = 2; 
        break;
    }

    uint16_t boxWidth = map(line.len, 0, LINE_CAP, 0, 72 - padX);
    if (boxWidth == 0 && line.len > 0) boxWidth = 2; 

    u8g2.drawBox(padX + 2, cursorY, boxWidth, max_hpx);

    if (style == '>') {
      u8g2.drawVLine(startX + (specialPadding / 2), cursorY, max_hpx+1);
    } 
    else if (style == 'C') {
      u8g2.drawVLine(startX + (specialPadding / 4) - 1, cursorY, max_hpx+1);
      u8g2.drawVLine(padX + 2 + boxWidth + 2, cursorY, max_hpx+1);
    } 
    else if (style == '1' || style == '2' || style == '3') {
      bool isLast = true;
      if (i < doc.lineCount - 1 && doc.lines[i+1].type == style) {
         isLast = false; 
      }
      if (isLast && (cursorY + max_hpx + 1) < u8g2.getDisplayHeight()) {
        u8g2.drawHLine(startX, cursorY + max_hpx + 1, 74);
      }
    } 
    else if (style == 'U') {
      u8g2.drawHLine(startX + specialPadding - 3, cursorY + (max_hpx / 2), 2);
    } 
    else if (style == 'O') {
      u8g2.drawVLine(startX + specialPadding - 3, cursorY, 2);
      u8g2.drawPixel(startX + specialPadding - 1, cursorY + 1);
    }

    if (i == activeCursorLine) {
      u8g2.drawFrame(padX, cursorY - 1, boxWidth + 4, max_hpx + 2);
      u8g2.drawTriangle(74, cursorY-3, 74, cursorY + 3, 70, cursorY); 
    }

    cursorY += max_hpx + 2; 
    if (style == '1' || style == '2' || style == '3') cursorY += 2;
  }

  Line& activeLine = doc.lines[activeCursorLine];
  String typeLabel = "";
  
  switch (activeLine.type) {
    case '1': typeLabel = "H1"; break;
    case '2': typeLabel = "H2"; break;
    case '3': typeLabel = "H3"; break;
    case '>': typeLabel = "QUOTE"; break;
    case 'C': typeLabel = "CODE"; break;
    case 'U': 
    case 'u': typeLabel = "U LIST"; break;
    case 'O': 
    case 'o': typeLabel = "O LIST"; break;
    case 'H': typeLabel = "H RULE"; break;
    case 'B': typeLabel = "BLANK"; break;
    case ' ': typeLabel = " "; break;
    default:  typeLabel = "BODY"; break;
  }

  u8g2.setFont(u8g2_font_5x7_tf);
  
  String lineStr = "L: " + String(activeCursorLine);
  u8g2.drawStr(80, 7, lineStr.c_str());
  u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth(typeLabel.c_str()), 7, typeLabel.c_str());
  u8g2.drawHLine(78, 10, u8g2.getDisplayWidth()-78);

  u8g2.setFont(u8g2_font_lubR18_tf);

  if (activeLine.len > 0) {
    int prevCursorX = 80;
    int rightEdge = u8g2.getDisplayWidth(); 
    
    uint16_t i = 0;
    while(i < activeLine.len) {
      if (prevCursorX > rightEdge) break; 
      
      uint16_t unicode = decodeUTF8(activeLine.text, &i, activeLine.len);
      if (unicode == '*') continue; 
      
      // FIX: Use our pre-cached fast lookup table instead!
      int charWidth = getFastOledCharWidth(unicode, false, false, false);
      
      u8g2.drawGlyph(prevCursorX, u8g2.getDisplayHeight(), unicode);
      prevCursorX += charWidth;
    }
  }

  u8g2.sendBuffer();
}

bool isFolderEmpty(const char* dirPath) {
  if (!global_fs) return true;
  
  File dir = global_fs->open(dirPath);
  if (!dir || !dir.isDirectory()) {
    return true; 
  }

  File file = dir.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      return false; 
    }
    file = dir.openNextFile();
  }
  
  return true; 
}

#pragma region Mrkdn File Ops
void saveMarkdownFile(const String& path) {
  if (PM_SDAUTO().getNoSD()) {
    OLED().sysMessage("SAVE FAILED - No SD!",3000);
    return;
  }

  SDActive = true;
  pocketmage::setCpuSpeed(240); 
  delay(50);

  String savePath = path;
  if (savePath == "" || savePath == "-")
    savePath = "/temp.txt";
  if (!savePath.startsWith("/"))
    savePath = "/" + savePath;

  File file = global_fs->open(savePath.c_str(), FILE_WRITE);
  if (!file) {
    OLED().sysMessage("SAVE FAILED - OPEN ERR",2000);
    ESP_LOGE("SD", "Failed to open file for writing: %s", savePath.c_str());
    SDActive = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(80);
    return;
  }

  for (ulong i = 0; i < document.lineCount; i++) {
    Line& line = document.lines[i];
    
    bool isContinuation = false;
    if (i > 0) {
      char pType = document.lines[i-1].type;
      if (line.type == 'T' && pType == 'T') isContinuation = true;
      else if (line.type == 'u' && (pType == 'U' || pType == 'u')) isContinuation = true;
      else if (line.type == 'o' && (pType == 'O' || pType == 'o')) isContinuation = true;
      else if (line.type == '1' && pType == '1') isContinuation = true;
      else if (line.type == '2' && pType == '2') isContinuation = true;
      else if (line.type == '3' && pType == '3') isContinuation = true;
      else if (line.type == '>' && pType == '>') isContinuation = true;
      else if (line.type == 'C' && pType == 'C') isContinuation = true;
    }

    if (!isContinuation) {
      if (i > 0) {
        if (document.lines[i-1].type == 'C') file.print("`");
        file.println();
      }

      switch (line.type) {
        case '1': file.print("# "); break;
        case '2': file.print("## "); break;
        case '3': file.print("### "); break;
        case '>': file.print("> "); break;
        case 'U': file.print("- "); break;
        case 'O': file.print("1. "); break;
        case 'C': file.print("`"); break;
        case 'H': file.print("---"); break;
        default: break; 
      }
    } else {
      file.print(" ");
    }

    if (line.len > 0) {
      file.print(line.text);
    }
  }

  if (document.lineCount > 0) {
    if (document.lines[document.lineCount - 1].type == 'C') file.print("`");
    file.println();
  }

  file.close();

  PM_SDAUTO().writeMetadata(savePath);
  PM_SDAUTO().setEditingFile(savePath);

  OLED().sysMessage("Saved: " + savePath,1000);

  if (SAVE_POWER) pocketmage::setCpuSpeed(80); 
  SDActive = false;
}

bool loadMarkdownFile(const String& path) {
  pocketmage::setCpuSpeed(240);

  if (path == "" || path == " " || path == "-") {
    return false;
  }

  if (PM_SDAUTO().getNoSD()) {
    OLED().sysMessage("LOAD FAILED - No SD!",5000);
    return false;
  }

  delay(50);

  File file = global_fs->open(path.c_str(), FILE_READ);
  if (!file) {
    return false;
  }

  document.lineCount = 0;
  for (ulong i = 0; i < MAX_LINES; i++) {
    initLine(document.lines[i]);
  }

  while (file.available() && document.lineCount < MAX_LINES) {
    String line = file.readStringUntil('\n');
    line.trim();
    char style = 'T';
    String content = line; 

    if (line.length() == 0) {
      style = 'B'; 
      content = "";
    } else if (line.startsWith("### ")) {
      style = '3'; 
      content = line.substring(4);  
    } else if (line.startsWith("## ")) {
      style = '2'; 
      content = line.substring(3);  
    } else if (line.startsWith("# ")) {
      style = '1'; 
      content = line.substring(2);  
    } else if (line.startsWith("> ")) {
      style = '>'; 
      content = line.substring(2);  
    } else if (line.startsWith("- ")) {
      style = 'U'; 
      content = line.substring(2); 
    } else if (line == "---") {
      style = 'H'; 
      content = "";  
    } else if ((line.startsWith("```")) || (line.startsWith("`") && line.endsWith("`")) || (line.startsWith("```") && line.endsWith("```"))) {
      if (line.startsWith("```"))
        content = line.substring(3);
      else if (line.startsWith("```") && line.endsWith("```"))
        content = line.substring(3, line.length() - 3);
      else if (line.startsWith("`") && line.endsWith("`"))
        content = line.substring(1, line.length() - 1);

      style = 'C'; 
    } else if (line.length() > 2 && isDigit(line.charAt(0)) && line.charAt(1) == '.' && line.charAt(2) == ' ') {
      style = 'O'; 
      content = line.substring(3); 
    }

    while (content.length() > 0 && document.lineCount < MAX_LINES) {
      int splitIndex = findWrapIndex(content, 0, style);

      String chunk = content.substring(0, splitIndex);
      content = content.substring(splitIndex);
      content.trim(); 

      Line& docLine = document.lines[document.lineCount];
      docLine.type = style;
      strncpy(docLine.text, chunk.c_str(), LINE_CAP);
      docLine.text[LINE_CAP] = '\0';
      docLine.len = strlen(docLine.text);
      
      document.lineCount++;
      
      if (content.length() > 0) {
        if (style == 'U') style = 'u';
        else if (style == 'O') style = 'o';
      }
    }
    
    if (content.length() == 0 && (style == 'B' || style == 'H') && document.lineCount < MAX_LINES) {
        Line& docLine = document.lines[document.lineCount];
        docLine.type = style;
        docLine.text[0] = '\0';
        docLine.len = 0;
        document.lineCount++;
    }
  }

  file.close();

  if (document.lineCount == 0) {
    document.lineCount = 1;
    initLine(document.lines[0]);
    document.lines[0].type = 'T';
    currentLineNum = 0;
  } else {
    currentLineNum = document.lineCount - 1;
  }

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(80);
  SDActive = false;

  OLED().sysMessage("FILE LOADED",500);

  return true;
}

void newMarkdownFile(const String& path) {
  if (PM_SDAUTO().getNoSD()) {
    OLED().sysMessage("CREATE FAILED - No SD!",3000);
    return;
  }

  SDActive = true;
  pocketmage::setCpuSpeed(240); 
  delay(50);

  String savePath = path;
  if (savePath == "" || savePath == "-") savePath = "/notes/temp.txt";
  if (!savePath.startsWith("/")) savePath = "/" + savePath;

  File file = global_fs->open(savePath.c_str(), FILE_WRITE);
  if (!file) {
    OLED().sysMessage("CREATE FAILED",2000);
    ESP_LOGE("SD", "Failed to create file: %s", savePath.c_str());
    SDActive = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(80);
    return;
  }
  file.close(); 

  PM_SDAUTO().writeMetadata(savePath);
  PM_SDAUTO().setEditingFile(savePath);

  OLED().sysMessage("Created File",1000);

  loadMarkdownFile(savePath);
  
  currentLineNum = 0;
  topVisibleLine = 0;
  updateScreen = true;

  if (SAVE_POWER) pocketmage::setCpuSpeed(80); 
  SDActive = false;
}

#pragma region OLED Editor
// Extanded OLED char width cache (Handles up to 255 for extended ASCII/UTF-8 map)
static uint8_t oled_char_widths[5][256] = {0};
static bool oled_widths_cached = false;

inline void initOledWidthCache() {
  if (oled_widths_cached) return;
  char temp[3] = {0, 0, 0};
  
  for (int i = 32; i < 256; i++) {
    // Pack index 'i' into proper UTF-8 format for u8g2 to measure
    if (i < 128) {
       temp[0] = i; temp[1] = 0;
    } else {
       temp[0] = 0xC0 | (i >> 6);
       temp[1] = 0x80 | (i & 0x3F);
       temp[2] = 0;
    }
    
    u8g2.setFont(u8g2_font_lubR18_tf);
    oled_char_widths[0][i] = u8g2.getUTF8Width(temp);
    
    u8g2.setFont(u8g2_font_luBS18_tf);
    oled_char_widths[1][i] = u8g2.getUTF8Width(temp);
    
    u8g2.setFont(u8g2_font_luIS18_tf);
    oled_char_widths[2][i] = u8g2.getUTF8Width(temp);
    
    u8g2.setFont(u8g2_font_luBIS18_tf);
    oled_char_widths[3][i] = u8g2.getUTF8Width(temp);
    
    u8g2.setFont(u8g2_font_5x7_tf);
    oled_char_widths[4][i] = u8g2.getUTF8Width(temp);
  }
  oled_widths_cached = true;
}

inline uint8_t getFastOledCharWidth(uint16_t unicode, bool bold, bool italic, bool isTiny) {
  if (unicode < 32 || unicode > 255) return 0; // Safely ignore control chars and unmapped blocks
  if (isTiny) return oled_char_widths[4][unicode];
  
  uint8_t fontIdx = 0;
  if (bold && italic) fontIdx = 3;
  else if (italic) fontIdx = 2;
  else if (bold) fontIdx = 1;
  
  return oled_char_widths[fontIdx][unicode];
}

// OLED Editor
void editorOledDisplay(Line& line, uint16_t cursor_pos, bool currentlyTyping) {
  u8g2.clearBuffer();
  initOledWidthCache();

  int display_w = u8g2.getDisplayWidth();
  int total_pixel_width = 0;
  int cursor_pixel_offset = 0;
  
  bool currentWordBold = false;
  bool currentWordItalic = false;

  bool bold = false;
  bool italic = false;

  // --- PASS 1: Single-sweep UTF-8 width and cursor calculation ---
  uint16_t i = 0;
  while (i < line.len) {
    uint16_t start_i = i;
    uint16_t unicode = decodeUTF8(line.text, &i, line.len);

    if (unicode == '*') {
      if (start_i == 0 || line.text[start_i - 1] != '*') {
        uint8_t starCount = 1;
        while (i < line.len && line.text[i] == '*' && starCount < 3) {
          starCount++; i++;
        }
        switch (starCount) {
          case 1: italic = !italic; break;
          case 2: bold = !bold; break;
          case 3: bold = !bold; italic = !italic; break;
        }
      }
      total_pixel_width += getFastOledCharWidth('*', false, false, true);
    } else {
      int w = getFastOledCharWidth(unicode, bold, italic, false);
      if (italic) w -= 3; 
      total_pixel_width += w;
    }

    if (start_i < cursor_pos) cursor_pixel_offset = total_pixel_width;
    if (i == cursor_pos) { currentWordBold = bold; currentWordItalic = italic; }
  }

  // --- DETERMINE SCROLL OFFSET ---
  int line_start = 0;
  if (total_pixel_width >= display_w - 5) {
    if (cursor_pos >= line.len) {
      line_start = display_w - 8 - total_pixel_width;
    } else if (cursor_pixel_offset > (display_w - 8) / 2) {
      line_start = ((display_w - 8) / 2) - cursor_pixel_offset;
      if (line_start + total_pixel_width < display_w - 8) {
        line_start = display_w - 8 - total_pixel_width;
      }
    }
  }

  // --- PASS 2: Render UTF-8 characters ---
  int xpos = line_start;
  bold = false; 
  italic = false;

  if (cursor_pos == 0) u8g2.drawVLine(xpos, 1, 22);

  i = 0;
  while (i < line.len) {
    uint16_t start_i = i;
    uint16_t unicode = decodeUTF8(line.text, &i, line.len);

    if (unicode == '*') {
      if (start_i == 0 || line.text[start_i - 1] != '*') {
        uint8_t starCount = 1;
        while (i < line.len && line.text[i] == '*' && starCount < 3) {
          starCount++; i++;
        }
        switch (starCount) {
          case 1: italic = !italic; break;
          case 2: bold = !bold; break;
          case 3: bold = !bold; italic = !italic; break;
        }
      }
      
      u8g2.setFont(u8g2_font_5x7_tf);
      int w = getFastOledCharWidth('*', false, false, true);
      if (xpos + w >= 0 && xpos <= display_w) u8g2.drawGlyph(xpos, 8, '*'); 
      xpos += w;
    } else {
      setFontOLED(bold, italic);
      int char_w = getFastOledCharWidth(unicode, bold, italic, false);

      if (xpos + char_w >= 0 && xpos <= display_w) {
        u8g2.drawGlyph(xpos, 20, unicode); 
      }
      
      xpos += char_w;
      if (italic) xpos -= 3;
    }

    if (cursor_pos == i) u8g2.drawVLine(xpos, 1, 22);
  }

  // Draw progress bar if on the last line
  if (currentLineNum == (document.lineCount - 1)) {
    int width = getLineEinkWidth(line);
    long progress = map(width,0,display.width(),0,u8g2.getDisplayWidth());

    if (progress > 0) {
      u8g2.drawHLine(0,0,progress);
      u8g2.drawHLine(0,1,progress);
      u8g2.drawVLine(0,0,2);
      u8g2.drawVLine(u8g2.getDisplayWidth()-1,0,2);
    }
  }

  if (currentlyTyping) toolBar(line, currentWordBold, currentWordItalic);
  else OLED().infoBar();

  u8g2.sendBuffer();
}

#pragma region E-Ink Editor
// E-Ink Editor
void editorEinkDisplay(Document& doc, uint16_t currentLineNum) {
  if (currentLineNum < topVisibleLine) {
    topVisibleLine = currentLineNum;
  }

  uint16_t j = 0;
  while (j <= doc.lineCount) {
    int cursorBottomY = 0;
    
    for (uint16_t i = topVisibleLine; i <= currentLineNum; i++) {
      cursorBottomY += getCalculatedLineHeight(doc.lines[i]);
    }

    if (cursorBottomY > display.height()) {
      topVisibleLine++;
    } else {
      break;
    }
    j++;
  }

  display.fillScreen(GxEPD_WHITE); 
  int currentY = 0;
  int linesDrawn = 0;

  for (uint16_t i = topVisibleLine; i < doc.lineCount; i++) {
    bool isSelected = (i == currentLineNum);
    int heightUsed = drawLineEink(doc, i, 0, currentY, isSelected); 
    
    currentY += heightUsed;
    linesDrawn++;
    if (currentY >= display.height() + 20) break;
  }

  // Draw TXT Scrollbar (Matching Terminal app style)
  int maxScroll = doc.lineCount - linesDrawn;
  if (maxScroll > 0) {
    int barWidth = 2;
    int barX = display.width() - barWidth;
    
    float visibleRatio = (float)linesDrawn / doc.lineCount;
    int handleHeight = max((int)(display.height() * visibleRatio), 15);
    
    float scrollFraction = (float)topVisibleLine / maxScroll;
    if (scrollFraction > 1.0f) scrollFraction = 1.0f; // clamp just in case
    int handleY = scrollFraction * (display.height() - handleHeight);

    // Backdrop for scrollbar
    display.fillRect(barX-1, 0, barWidth+1, display.height(), GxEPD_WHITE);
    
    display.fillRect(barX, handleY, barWidth, handleHeight, GxEPD_BLACK);

    // Ensure bar never visually touches top/bottom edges
    display.drawFastHLine(barX, display.height()-1, barWidth, GxEPD_WHITE);
    display.drawFastHLine(barX, 0, barWidth, GxEPD_WHITE);
  }
}

void editor(char inchar) {
  static uint16_t cursor_pos = 0;
  static long lastInput = millis();
  bool currentlyTyping = false;
  
  static unsigned long lastStyleCycleMillis = 0;
  static bool pendingStyleRefresh = false;

  Line& line = document.lines[currentLineNum];

  if (cursor_pos > line.len) cursor_pos = line.len;

  if (inchar != 0) {
    lastInput = millis();

    // ENTER KEY
    if (inchar == 13) {
      Line& activeLine = document.lines[currentLineNum];

      String leftHalf = String(activeLine.text).substring(0, cursor_pos);
      String rightHalf = String(activeLine.text).substring(cursor_pos);
      
      strncpy(activeLine.text, leftHalf.c_str(), LINE_CAP);
      activeLine.text[LINE_CAP] = '\0';
      activeLine.len = leftHalf.length();

      insertLineArray(currentLineNum + 1);
      document.lines[currentLineNum + 1].type = 'B';
      
      insertLineArray(currentLineNum + 2);
      Line& newLine = document.lines[currentLineNum + 2];
      
      if (activeLine.type == 'U' || activeLine.type == 'u') newLine.type = 'U'; 
      else if (activeLine.type == 'O' || activeLine.type == 'o') newLine.type = 'O'; 
      else if (activeLine.type == '>') newLine.type = '>';
      else if (activeLine.type == 'C') newLine.type = 'C';
      else newLine.type = 'T';

      strncpy(newLine.text, rightHalf.c_str(), LINE_CAP);
      newLine.text[LINE_CAP] = '\0';
      newLine.len = rightHalf.length();

      currentLineNum += 2;
      cursor_pos = 0;
      updateScreen = true;
      
      reflowParagraph(currentLineNum, cursor_pos);
    }

    else if (inchar == 17) {
      if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
      else if (KB().getKeyboardState() == FUNC) KB().setKeyboardState(FN_SHIFT);
      else KB().setKeyboardState(SHIFT);
    }

    else if (inchar == 18) {
      if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
      else if (KB().getKeyboardState() == SHIFT) KB().setKeyboardState(FN_SHIFT);
      else KB().setKeyboardState(FUNC);
    }

    // BKSP Recieved
    else if (inchar == 8) {
      if (cursor_pos > 0) {
        deleteChar(document.lines[currentLineNum], cursor_pos);
        
        bool hasContinuation = false;
        if (currentLineNum + 1 < document.lineCount) {
          char bType = document.lines[currentLineNum].type;
          char nextType = document.lines[currentLineNum + 1].type;
          if (bType == nextType) hasContinuation = true;
          else if ((bType == 'U' || bType == 'u') && nextType == 'u') hasContinuation = true;
          else if ((bType == 'O' || bType == 'o') && nextType == 'o') hasContinuation = true;
        }

        if (hasContinuation) {
          ulong preReflowLine = currentLineNum; 
          reflowParagraph(currentLineNum, cursor_pos); 
          if (currentLineNum != preReflowLine) updateScreen = true;
        }
      } else {
        mergeLinesUp(currentLineNum, cursor_pos);
      }
    }

    // LEFT
    else if (inchar == 19) {
      if (cursor_pos > 0) {
        do { cursor_pos--; } while (cursor_pos > 0 && (document.lines[currentLineNum].text[cursor_pos] & 0xC0) == 0x80);
      } else if (currentLineNum > 0) {
        currentLineNum--;
        cursor_pos = document.lines[currentLineNum].len;
        updateScreen = true;
      }
    }

    // RIGHT
    else if (inchar == 21) {
      if (cursor_pos < document.lines[currentLineNum].len) {
        do { cursor_pos++; } while (cursor_pos < document.lines[currentLineNum].len && (document.lines[currentLineNum].text[cursor_pos] & 0xC0) == 0x80);
      } else if (currentLineNum < document.lineCount - 1) {
        currentLineNum++;
        cursor_pos = 0;
        updateScreen = true;
      }
    }

    else if (inchar == 20) {}

    else if (inchar == 28) {
      cycleParagraphStyle(currentLineNum, cursor_pos);
      lastStyleCycleMillis = millis();
      pendingStyleRefresh = true;
    }

    else if (inchar == 30) {
      cycleTextStyle(document.lines[currentLineNum], cursor_pos);
    }

    else if (inchar == 29) {
      if (CurrentTXTState_NEW != JOURNAL_MODE) {
        CurrentTXTState_NEW = NEW_FILE;
        KB().setKeyboardState(NORMAL);
      }
    }

    else if (inchar == 12) {
      if (CurrentTXTState_NEW != JOURNAL_MODE) HOME_INIT();
      else JOURNAL_INIT(); 
    }

    else if (inchar == 6) {
      if (CurrentTXTState_NEW != JOURNAL_MODE) {
        String savePath = PM_SDAUTO().getEditingFile();
        if (savePath == "" || savePath == "-" || savePath == "/notes/temp.txt") {
          KB().setKeyboardState(NORMAL);
          CurrentTXTState_NEW = SAVE_AS;
        } else {
          if (!savePath.startsWith("/")) savePath = "/" + savePath;
          saveMarkdownFile(savePath);
        }
      } else {
        String savePath = getCurrentJournal();
        if (!savePath.startsWith("/")) savePath = "/" + savePath;
        saveMarkdownFile(savePath);
      }
    }

    else if (inchar == 7) {
      if (CurrentTXTState_NEW != JOURNAL_MODE) {
        CurrentTXTState_NEW = LOAD_FILE;
        KB().setKeyboardState(NORMAL);
      } else {
        String outPath = getCurrentJournal();
        if (!outPath.startsWith("/")) outPath = "/" + outPath;
        loadMarkdownFile(outPath);
      }
    }

    else if (inchar == 24) cursor_pos = 0;
    else if (inchar == 26) cursor_pos = document.lines[currentLineNum].len;
    else if (inchar == 25) {}

    // TAB / SHIFT+TAB
    else if (inchar == 9 || inchar == 14) {
      if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT || inchar == 14) {
        if (cursor_pos == 0) {
          if (currentLineNum > 0) {
            currentLineNum--;
            cursor_pos = document.lines[currentLineNum].len;
            updateScreen = true;
          }
        } else {
          while (cursor_pos > 0 && document.lines[currentLineNum].text[cursor_pos - 1] == ' ') cursor_pos--;
          while (cursor_pos > 0 && document.lines[currentLineNum].text[cursor_pos - 1] != ' ') cursor_pos--;
        }
      } else {
        if (cursor_pos >= line.len) {
          if (currentLineNum < document.lineCount - 1) {
            currentLineNum++;
            cursor_pos = 0;
            updateScreen = true;
          }
        } else {
          while (cursor_pos < line.len && document.lines[currentLineNum].text[cursor_pos] != ' ') cursor_pos++;
          while (cursor_pos < line.len && document.lines[currentLineNum].text[cursor_pos] == ' ') cursor_pos++;
        }
      }
    }

    // Normal character input
    else {
      ulong preReflowLine = currentLineNum;
      
      if (document.lines[currentLineNum].type == ' ' || document.lines[currentLineNum].type == 'B') {
        document.lines[currentLineNum].type = 'T';
      }

      insertChar(document.lines[currentLineNum], cursor_pos, inchar);
      
      reflowParagraph(currentLineNum, cursor_pos);

      if (currentLineNum != preReflowLine) {
        updateScreen = true;
      }

      if (inchar < 48 || inchar > 57) {
        if (KB().getKeyboardState() != NORMAL) KB().setKeyboardState(NORMAL);
      }
    }
  }

  if (pendingStyleRefresh && (millis() - lastStyleCycleMillis > 1000)) {
    updateScreen = true;
    pendingStyleRefresh = false;
  }

  if (millis() - lastInput > IDLE_TIME) currentlyTyping = false;
  else currentlyTyping = true;

  unsigned long currentMillis = millis();
  if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
    OLEDFPSMillis = currentMillis;
    
    if (TOUCH().getLastTouch() == -1) {
      if (!currentlyTyping) keypad.flush(); 
      lastInput = millis();
      editorOledDisplay(document.lines[currentLineNum], cursor_pos, currentlyTyping);
    } else {
      displayScrollPreviewOLED(document, currentLineNum);
    }
  }
}


#pragma region INIT
void initFonts() {
  // Mono
  fonts[mono].normal = &FreeMono9pt8b;
  fonts[mono].normal_B = &FreeMonoBold9pt8b;
  fonts[mono].normal_I = &FreeMonoOblique9pt8b;
  fonts[mono].normal_BI = &FreeMonoBoldOblique9pt8b;

  fonts[mono].h1 = &FreeMonoBold24pt8b;
  fonts[mono].h1_B = &FreeMonoBold24pt8b;  // Already bold
  fonts[mono].h1_I = &FreeMonoBoldOblique24pt8b;
  fonts[mono].h1_BI = &FreeMonoBoldOblique24pt8b;

  fonts[mono].h2 = &FreeMonoBold18pt8b;
  fonts[mono].h2_B = &FreeMonoBold18pt8b;
  fonts[mono].h2_I = &FreeMonoBoldOblique18pt8b;
  fonts[mono].h2_BI = &FreeMonoBoldOblique18pt8b;

  fonts[mono].h3 = &FreeMonoBold12pt8b;
  fonts[mono].h3_B = &FreeMonoBold12pt8b;
  fonts[mono].h3_I = &FreeMonoBoldOblique12pt8b;
  fonts[mono].h3_BI = &FreeMonoBoldOblique12pt8b;

  fonts[mono].code = &FreeMono9pt8b;
  fonts[mono].code_B = &FreeMono9pt8b;
  fonts[mono].code_I = &FreeMono9pt8b;
  fonts[mono].code_BI = &FreeMono9pt8b;

  fonts[mono].quote = &FreeMono9pt8b;
  fonts[mono].quote_B = &FreeMonoBold9pt8b;
  fonts[mono].quote_I = &FreeMonoOblique9pt8b;
  fonts[mono].quote_BI = &FreeMonoBoldOblique9pt8b;

  fonts[mono].list = &FreeMono9pt8b;
  fonts[mono].list_B = &FreeMonoBold9pt8b;
  fonts[mono].list_I = &FreeMonoOblique9pt8b;
  fonts[mono].list_BI = &FreeMonoBoldOblique9pt8b;

  // Serif
  fonts[serif].normal = &FreeSerif9pt8b;
  fonts[serif].normal_B = &FreeSerifBold9pt8b;
  fonts[serif].normal_I = &FreeSerifItalic9pt8b;
  fonts[serif].normal_BI = &FreeSerifBoldItalic9pt8b;

  fonts[serif].h1 = &FreeSerifBold24pt8b;
  fonts[serif].h1_B = &FreeSerifBold24pt8b;
  fonts[serif].h1_I = &FreeSerifBoldItalic24pt8b;
  fonts[serif].h1_BI = &FreeSerifBoldItalic24pt8b;

  fonts[serif].h2 = &FreeSerifBold18pt8b;
  fonts[serif].h2_B = &FreeSerifBold18pt8b;
  fonts[serif].h2_I = &FreeSerifBoldItalic18pt8b;
  fonts[serif].h2_BI = &FreeSerifBoldItalic18pt8b;

  fonts[serif].h3 = &FreeSerifBold12pt8b;
  fonts[serif].h3_B = &FreeSerifBold12pt8b;
  fonts[serif].h3_I = &FreeSerifBoldItalic12pt8b;
  fonts[serif].h3_BI = &FreeSerifBoldItalic12pt8b;

  fonts[serif].code = &FreeMono9pt8b;
  fonts[serif].code_B = &FreeMono9pt8b;
  fonts[serif].code_I = &FreeMono9pt8b;
  fonts[serif].code_BI = &FreeMono9pt8b;

  fonts[serif].quote = &FreeSerif9pt8b;
  fonts[serif].quote_B = &FreeSerifBold9pt8b;
  fonts[serif].quote_I = &FreeSerifItalic9pt8b;
  fonts[serif].quote_BI = &FreeSerifBoldItalic9pt8b;

  fonts[serif].list = &FreeSerif9pt8b;
  fonts[serif].list_B = &FreeSerifBold9pt8b;
  fonts[serif].list_I = &FreeSerifItalic9pt8b;
  fonts[serif].list_BI = &FreeSerifBoldItalic9pt8b;

  // Sans
  fonts[sans].normal = &FreeSans9pt8b;
  fonts[sans].normal_B = &FreeSansBold9pt8b;
  fonts[sans].normal_I = &FreeSansOblique9pt8b;
  fonts[sans].normal_BI = &FreeSansBoldOblique9pt8b;

  fonts[sans].h1 = &FreeSansBold24pt8b;
  fonts[sans].h1_B = &FreeSansBold24pt8b;
  fonts[sans].h1_I = &FreeSansBoldOblique24pt8b;
  fonts[sans].h1_BI = &FreeSansBoldOblique24pt8b;

  fonts[sans].h2 = &FreeSansBold18pt8b;
  fonts[sans].h2_B = &FreeSansBold18pt8b;
  fonts[sans].h2_I = &FreeSansBoldOblique18pt8b;
  fonts[sans].h2_BI = &FreeSansBoldOblique18pt8b;

  fonts[sans].h3 = &FreeSansBold12pt8b;
  fonts[sans].h3_B = &FreeSansBold12pt8b;
  fonts[sans].h3_I = &FreeSansBoldOblique12pt8b;
  fonts[sans].h3_BI = &FreeSansBoldOblique12pt8b;

  fonts[sans].code = &FreeMono9pt8b;
  fonts[sans].code_B = &FreeMono9pt8b;
  fonts[sans].code_I = &FreeMono9pt8b;
  fonts[sans].code_BI = &FreeMono9pt8b;

  fonts[sans].quote = &FreeSans9pt8b;
  fonts[sans].quote_B = &FreeSansBold9pt8b;
  fonts[sans].quote_I = &FreeSansOblique9pt8b;
  fonts[sans].quote_BI = &FreeSansBoldOblique9pt8b;

  fonts[sans].list = &FreeSans9pt8b;
  fonts[sans].list_B = &FreeSansBold9pt8b;
  fonts[sans].list_I = &FreeSansOblique9pt8b;
  fonts[sans].list_BI = &FreeSansBoldOblique9pt8b;
}

void TXT_INIT(String inPath) {
  initDocMemory();
  initFonts();
  setFontStyle(serif);
  bool fileLoaded = loadMarkdownFile(inPath);
  if (fileLoaded) {
    CurrentAppState = TXT;
    CurrentTXTState_NEW = TXT_;
    updateScreen = true;
  } else if (isFolderEmpty("/notes")){
    // /notes is empty, make a blank file
    newMarkdownFile("/notes/temp.txt");
    CurrentAppState = TXT;
    CurrentTXTState_NEW = TXT_;
    updateScreen = true;
  } else {
    CurrentAppState = TXT;
    CurrentTXTState_NEW = LOAD_FILE;
    updateScreen = true;
  }
  KB().setKeyboardState(NORMAL);
  
}

void TXT_INIT_JournalMode() {
  initDocMemory();
  initFonts();

  String outPath = getCurrentJournal();
  if (!outPath.startsWith("/")) outPath = "/" + outPath;
  loadMarkdownFile(outPath);

  setFontStyle(serif);

  updateScreen = true;
  CurrentAppState = TXT;
  CurrentTXTState_NEW = JOURNAL_MODE;
  KB().setKeyboardState(NORMAL);
}

#pragma region Loops
void einkHandler_TXT_NEW() {
  if (updateScreen) {
    updateScreen = false;
    display.setFullWindow();
    display.setTextColor(GxEPD_BLACK);
    editorEinkDisplay(document, currentLineNum);
    #if POCKETMAGE_HW_VERSION != 2
      EINK().forceSlowFullUpdate(true);
    #endif
    EINK().refresh();
  }
}

void processKB_TXT_NEW() {
  OLED().setPowerSave(false);
  disableTimeout = false;
  
  static String inputBuffer = "";

  // 1. Drain the hardware buffer continuously at loop speed
  char inchar = KB().updateKeypress();
  unsigned long currentMillis = millis();

  if (inchar != 0) {
    pocketmage::setCpuSpeed(240);
  }

  if (CurrentTXTState_NEW == TXT_ || CurrentTXTState_NEW == JOURNAL_MODE) {
    if (TOUCH().updateScroll(document.lineCount, currentLineNum)) {
      updateScreen = true; 
    }
  }

  switch (CurrentTXTState_NEW) {
    case TXT_:
    case JOURNAL_MODE:
      editor(inchar);
      break;

    case SAVE_AS:
      // 2. Process input only if cooldown has expired
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        if (inchar != 0) {
          KBBounceMillis = currentMillis; // Reset the debounce timer

          if (inchar == 13) {
            if (inputBuffer != "" && inputBuffer != "-") {
              if (!inputBuffer.startsWith("/notes/")) inputBuffer = "/notes/" + inputBuffer;
              if (!inputBuffer.endsWith(".txt") && !inputBuffer.endsWith(".md")) inputBuffer = inputBuffer + ".txt";
              saveMarkdownFile(inputBuffer);
              CurrentTXTState_NEW = TXT_;
            } else {
              OLED().sysMessage("Invalid Name", 2000);
            }
            inputBuffer = "";
          }
          else if (inchar == 17) {
            if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == FUNC) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(SHIFT);
          }
          else if (inchar == 18) {
            if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == SHIFT) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(FUNC);
          }
          else if (inchar == 32) {
          }
          else if (inchar == 20) {
            inputBuffer = "";
          }
          else if (inchar == 8) {
            if (inputBuffer.length() > 0) {
              inputBuffer.remove(inputBuffer.length() - 1);
            }
          }
          else if (inchar == 12) {
            CurrentTXTState_NEW = TXT_;
          }
          else {
            inputBuffer += inchar;
            if (inchar >= 48 && inchar <= 57) {} 
            else if (KB().getKeyboardState() != NORMAL) {
              KB().setKeyboardState(NORMAL);
            }
          }
        }
      }

      // 3. Update OLED at true OLED_MAX_FPS
      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        // Make sure we didn't just jump back to the text editor!
        if (CurrentTXTState_NEW == SAVE_AS) {
            OLED().oledLine(inputBuffer, inputBuffer.length(), false, "Input Filename");
        }
      }
      break;

    case NEW_FILE:
      // 2. Process input only if cooldown has expired
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        if (inchar != 0) {
          KBBounceMillis = currentMillis; // Reset the debounce timer

          if (inchar == 13) {
            if (inputBuffer != "" && inputBuffer != "-") {
              if (!inputBuffer.startsWith("/notes/")) inputBuffer = "/notes/" + inputBuffer;
              if (!inputBuffer.endsWith(".txt") && !inputBuffer.endsWith(".md")) inputBuffer = inputBuffer + ".txt";
              newMarkdownFile(inputBuffer);
              CurrentTXTState_NEW = TXT_;
              updateScreen = true;
            } else {
              OLED().sysMessage("Invalid Name", 2000);
            }
            inputBuffer = "";
          }
          else if (inchar == 17) {
            if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == FUNC) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(SHIFT);
          }
          else if (inchar == 18) {
            if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == SHIFT) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(FUNC);
          }
          else if (inchar == 32) {
          }
          else if (inchar == 20) {
            inputBuffer = "";
          }
          else if (inchar == 8) {
            if (inputBuffer.length() > 0) {
              inputBuffer.remove(inputBuffer.length() - 1);
            }
          }
          else if (inchar == 12) {
            CurrentTXTState_NEW = TXT_;
          }
          else {
            inputBuffer += inchar;
            if (inchar >= 48 && inchar <= 57) {} 
            else if (KB().getKeyboardState() != NORMAL) {
              KB().setKeyboardState(NORMAL);
            }
          }
        }
      }

      // 3. Update OLED at true OLED_MAX_FPS
      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        // Make sure we didn't just jump back to the text editor!
        if (CurrentTXTState_NEW == NEW_FILE) {
            OLED().oledLine(inputBuffer, inputBuffer.length(), false, "Name New File");
        }
      }
      break;

    case LOAD_FILE:
      String outPath = fileWizardMini(false, "/notes", inchar);
      if (outPath == "_EXIT_") {
        CurrentTXTState_NEW = TXT_;
        break;
      }
      else if (outPath != "") {
        if (outPath.endsWith(".txt") || outPath.endsWith(".md")) {
          if (!outPath.startsWith("/")) outPath = "/" + outPath;
          loadMarkdownFile(outPath);
          PM_SDAUTO().setEditingFile(outPath);
          CurrentTXTState_NEW = TXT_;
          updateScreen = true;
        } else {
          OLED().sysMessage("Incompatible Filetype!", 2000);
          CurrentTXTState_NEW = TXT_;
        }
      }
      break;
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

#endif