//  ooooooooooooo ooooooo  ooooo ooooooooooooo  //
//  8'   888   `8  `8888    d8'  8'   888   `8  //
//       888         Y888..8P         888       //
//       888          `8888'          888       //
//       888         .8PY888.         888       //
//       888        d8'  `888b        888       //
//      o888o     o888o  o88888o     o888o      //

#include <pocketmage.h>

// Font includes
// Mono
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBoldOblique12pt7b.h>
#include <Fonts/FreeMonoBoldOblique18pt7b.h>
#include <Fonts/FreeMonoBoldOblique24pt7b.h>
#include <Fonts/FreeMonoBoldOblique9pt7b.h>
#include <Fonts/FreeMonoOblique9pt7b.h>

// Serif
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/FreeSerifBold24pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerifBoldItalic12pt7b.h>
#include <Fonts/FreeSerifBoldItalic18pt7b.h>
#include <Fonts/FreeSerifBoldItalic24pt7b.h>
#include <Fonts/FreeSerifBoldItalic9pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>

// Sans
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBoldOblique12pt7b.h>
#include <Fonts/FreeSansBoldOblique18pt7b.h>
#include <Fonts/FreeSansBoldOblique24pt7b.h>
#include <Fonts/FreeSansBoldOblique9pt7b.h>
#include <Fonts/FreeSansOblique9pt7b.h>

#include "esp32-hal-log.h"
#include "esp_log.h"

// ------------------ General ------------------
enum TXTState_NEW { TXT_, FONT, SAVE_AS, LOAD_FILE, NEW_FILE };
TXTState_NEW CurrentTXTState_NEW = TXT_;

#define TYPE_INTERFACE_TIMEOUT 5000  // ms
#define SCROLL_LINE_OFFSET 3         // lines

// ------------------ Fonts ------------------
#define SPECIAL_PADDING 20      // Padding for lists, code blocks, quote blocks
#define SPACEWIDTH_SYMBOL "n"   // n is roughly the width of a space
#define WORDWIDTH_BUFFER 0      // Add extra spacing to each word
#define DISPLAY_WIDTH_BUFFER 0  // Add margin for text wrap calc
#define HEADING_LINE_PADDING 8  // Padding between each line
#define NORMAL_LINE_PADDING 2

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

const GFXfont* pickFont(char style, bool bold, bool italic) {
  FontMap& fm = fonts[fontStyle];  // currently active family

  switch (style) {
    case '1':  // H1
      if (bold && italic)
        return fm.h1_BI;
      if (bold)
        return fm.h1_B;
      if (italic)
        return fm.h1_I;
      return fm.h1;

    case '2':  // H2
      if (bold && italic)
        return fm.h2_BI;
      if (bold)
        return fm.h2_B;
      if (italic)
        return fm.h2_I;
      return fm.h2;

    case '3':  // H3
      if (bold && italic)
        return fm.h3_BI;
      if (bold)
        return fm.h3_B;
      if (italic)
        return fm.h3_I;
      return fm.h3;

    case '>':  // Quote
      if (bold && italic)
        return fm.quote_BI;
      if (bold)
        return fm.quote_B;
      if (italic)
        return fm.quote_I;
      return fm.quote;

    case '-':  // List
      if (bold && italic)
        return fm.list_BI;
      if (bold)
        return fm.list_B;
      if (italic)
        return fm.list_I;
      return fm.list;

    case 'C':  // Code
      if (bold && italic)
        return fm.code_BI;
      if (bold)
        return fm.code_B;
      if (italic)
        return fm.code_I;
      return fm.code;

    default:  // Normal
      if (bold && italic)
        return fm.normal_BI;
      if (bold)
        return fm.normal_B;
      if (italic)
        return fm.normal_I;
      return fm.normal;
  }
}

// ------------------ Document Variables ------------------
static bool updateScreen = false;
ulong indexCounter = 0;
ulong lineScroll = 0;
enum EditingModes { edit_inline = 0, edit_append = 1 };
uint8_t currentEditMode = edit_append;
String currentLine = "";

struct wordObject {
  String text;
  bool bold;
  bool italic;
};

struct LineObject {
  ulong index;
  std::vector<wordObject> words;
};

// Document Line object
struct DocLine {
  char style;                     // Markdown style: '1', '2', '3', '>', '-', etc.
  String line;                    // Raw line content
  std::vector<wordObject> words;  // Parsed words with formatting
  std::vector<LineObject> lines;  // split into line objects
  ulong orderedListNumber;

  // Parse the line into wordObjects
  void parseWords() {
    words.clear();
    int i = 0;
    while (i < line.length()) {
      if (line[i] == '*' && i + 1 < line.length() && line[i + 1] == '*') {
        // Bold **...**
        int end = line.indexOf("**", i + 2);
        if (end == -1)
          end = line.length();
        String segment = line.substring(i + 2, end);
        splitIntoWords(segment, true, false);
        i = end + 2;
      } else if (line[i] == '*') {
        // Italic *...*
        int end = line.indexOf("*", i + 1);
        if (end == -1)
          end = line.length();
        String segment = line.substring(i + 1, end);
        splitIntoWords(segment, false, true);
        i = end + 1;
      } else {
        // Normal text until next * or **
        int nextBold = line.indexOf("**", i);
        int nextItalic = line.indexOf("*", i);
        int end = line.length();
        if (nextBold >= 0)
          end = min(end, nextBold);
        if (nextItalic >= 0)
          end = min(end, nextItalic);
        String segment = line.substring(i, end);
        splitIntoWords(segment, false, false);
        i = end;
      }
    }
  }

  // Split word objects into lines
  void splitToLines() {
    uint16_t textWidth = display.width() - DISPLAY_WIDTH_BUFFER;

    if (style == '>' || style == 'C') {
      textWidth -= SPECIAL_PADDING;
    }
    else if (style == '-' || style == 'L') {
      textWidth -= 2*SPECIAL_PADDING;
    }

    lines.clear();
    LineObject currentLine;
    int lineWidth = 0;

    for (auto& w : words) {
      const GFXfont* font = pickFont(style, w.bold, w.italic);
      display.setFont(font);

      int16_t x1, y1;
      uint16_t wpx, hpx;
      display.getTextBounds(w.text.c_str(), 0, 0, &x1, &y1, &wpx, &hpx);

      int spaceWidth;
      display.getTextBounds(SPACEWIDTH_SYMBOL, 0, 0, &x1, &y1, (uint16_t*)&spaceWidth, &hpx);

      // Calculate width for this word plus space
      int addWidth =
          wpx + spaceWidth + WORDWIDTH_BUFFER;  // IDK why 12 makes the text wrap work perfectly...

      // If the word doesn't fit, start a new line
      if (lineWidth > 0 && (lineWidth + addWidth > textWidth)) {
        currentLine.index = indexCounter++;
        lines.push_back(currentLine);

        currentLine.words.clear();
        lineWidth = 0;
      }

      currentLine.words.push_back(w);
      lineWidth += addWidth;
    }

    if (!currentLine.words.empty()) {
      currentLine.index = indexCounter++;
      lines.push_back(currentLine);
    }
  }

  // Compile line objects back into text
  void compileToText() {
    String compiled = "";

    for (auto &ln : lines) {
      for (auto &w : ln.words) {
        // Determine formatting markers
        if (w.bold && w.italic)
          compiled += "***";
        else if (w.bold)
          compiled += "**";
        else if (w.italic)
          compiled += "*";

        compiled += w.text;

        if (w.bold && w.italic)
          compiled += "***";
        else if (w.bold)
          compiled += "**";
        else if (w.italic)
          compiled += "*";

        compiled += " ";
      }
    }

    compiled.trim();
    line = compiled;
  }

  int displayLine(int startX, int startY) {
    ulong offsetLineScroll = 0;
    if (lineScroll <= SCROLL_LINE_OFFSET) {
      offsetLineScroll = 0;
    } else
      offsetLineScroll = lineScroll - SCROLL_LINE_OFFSET;

    int cursorY = startY;

    // Entire block is offscreen, do not render.
    if (!lines.empty() && lines.back().index < offsetLineScroll) {
      return 0;
    }

    // ---------- Non-Text Rendered Items ---------- //

    // Horizontal Rules just print a line
    if (style == 'H') {
      display.drawFastHLine(0, cursorY + 3, display.width(), GxEPD_BLACK);
      display.drawFastHLine(0, cursorY + 4, display.width(), GxEPD_BLACK);
      return 8;
    }
    // Blank lines just take up space
    else if (style == 'B') {
      return 12;
    }


    // ---------- Add Padding If Needed ---------- //

    // Lists and Blockquotes are padded on the left
    if (style == '>')
      startX += SPECIAL_PADDING;
    else if (style == '-' || style == 'L')
      startX += 2*SPECIAL_PADDING;
    // Code blocks are padded on both sides
    else if (style == 'C')
      startX += (SPECIAL_PADDING / 2);

    
    // ---------- Render Text ---------- //

    for (auto& ln : lines) {
      if (ln.index < offsetLineScroll)
        continue;  // skip lines above scroll

      int cursorX = startX;

      // 1. Find max height for this line
      uint16_t max_hpx = 0;
      for (auto& w : ln.words) {
        const GFXfont* font = pickFont(style, w.bold, w.italic);
        display.setFont(font);
        int16_t x1, y1;
        uint16_t wpx, hpx;
        display.getTextBounds(w.text.c_str(), cursorX, cursorY, &x1, &y1, &wpx, &hpx);
        if (hpx > max_hpx)
          max_hpx = hpx;
      }

      // Add space for headings
      if (style == '1' || style == '2' || style == '3')
        max_hpx += 4;

      // 2. Draw all words at the same baseline
      for (auto& w : ln.words) {
        const GFXfont* font = pickFont(style, w.bold, w.italic);
        display.setFont(font);

        int16_t x1, y1;
        uint16_t wpx, hpx;
        display.getTextBounds(w.text.c_str(), cursorX, cursorY, &x1, &y1, &wpx, &hpx);

        // Draw word at the baseline
        display.setCursor(cursorX, cursorY + max_hpx);
        display.print(w.text);

        // Advance cursor (word width + space)
        int16_t sx1, sy1;
        uint16_t sw, sh;
        display.getTextBounds(SPACEWIDTH_SYMBOL, cursorX, cursorY, &sx1, &sy1, &sw, &sh);

        cursorX += wpx + sw;
      }

      // Move down for next line
      uint8_t padding = 0;
      if (style == '1' || style == '2' || style == '3')
        padding = HEADING_LINE_PADDING;
      else
        padding = NORMAL_LINE_PADDING;
      cursorY += max_hpx + padding;
    }

    // ---------- Post-Render Formatting ---------- //

    // Blockquotes get a vertical line on the left
    if (style == '>') {
      display.drawFastVLine(SPECIAL_PADDING / 2, startY, (cursorY - startY), GxEPD_BLACK);
      display.drawFastVLine((SPECIAL_PADDING / 2) + 1, startY, (cursorY - startY), GxEPD_BLACK);
    }

    // Code Blocks get a vertical line on each side
    else if (style == 'C') {
      display.drawFastVLine(SPECIAL_PADDING / 4, startY, (cursorY - startY), GxEPD_BLACK);
      display.drawFastVLine(display.width() - (SPECIAL_PADDING / 4), startY, (cursorY - startY),
                            GxEPD_BLACK);
      display.drawFastVLine((SPECIAL_PADDING / 4) + 1, startY, (cursorY - startY), GxEPD_BLACK);
      display.drawFastVLine(display.width() - (SPECIAL_PADDING / 4) - 1, startY, (cursorY - startY),
                            GxEPD_BLACK);
    }

    // Headings get a horizontal line below them
    else if ((style == '1' || style == '2' || style == '3')) {
      display.drawFastHLine(0, cursorY - 2, display.width(), GxEPD_BLACK);
      display.drawFastHLine(0, cursorY - 3, display.width(), GxEPD_BLACK);
    }

    // Unordered Lists get a '●'
    else if (style == '-') {
      display.fillCircle(startX - 8, startY + 8, 3, GxEPD_BLACK);
    }
    // Ordered Lists get their #
    else if (style == 'L') {
      String number = String(orderedListNumber) + ". ";
      const GFXfont* font = pickFont('T', false, false);
      display.setFont(font);
      int16_t x1, y1;
      uint16_t wpx, hpx;
      display.getTextBounds(number.c_str(), 0, 0, &x1, &y1, &wpx, &hpx);

      display.setCursor(startX - wpx - 5, startY + hpx);
      display.print(number.c_str());
    }

    return cursorY - startY;
  }

  int displayLinePreview(int startX, int startY) {
    // 74px on OLED horizontally
    u8g2.setDrawColor(1);

    int specialPadding = 8;  // px

    int cursorY = startY;

    // Entire block is offscreen, do not render.
    if (!lines.empty() && lines.back().index < lineScroll) {
      return 0;
    }

    // Horizontal Rules just print a line
    if (style == 'H' && cursorY > 0) {
      u8g2.drawHLine(startX, cursorY, 80);
      return 2;
    }
    // Blank Lines just take up space
    else if (style == 'B') {
      return 3;
    }

    // Lists and Blockquotes are padded on the left
    if (style == '>' || style == '-' || style == 'L')
      startX += specialPadding;
    else if (style == 'C')
      startX += (specialPadding / 2);

    for (auto& ln : lines) {
      if (ln.index < lineScroll)
        continue;  // skip lines above scroll

      int cursorX = startX;

      // 1. Find height for this line
      int max_hpx = 0;
      switch (style) {
        case 'T':
          max_hpx = 2;
          break;
        case '1':
          max_hpx = 5;
          break;
        case '2':
          max_hpx = 4;
          break;
        case '3':
          max_hpx = 3;
          break;
        case 'C':
          max_hpx = 2;
          break;
        case '>':
          max_hpx = 2;
          break;
        case '-':
          max_hpx = 2;
          break;
        case 'L':
          max_hpx = 2;
          break;
        case 'H':
          max_hpx = 1;
          break;
        default:
          max_hpx = 2;
          break;
      }

      // 2. Draw all words at the same baseline
      for (auto& w : ln.words) {
        const GFXfont* font = pickFont(style, w.bold, w.italic);
        display.setFont(font);

        int16_t x1, y1;
        uint16_t wpx, hpx;
        display.getTextBounds(w.text.c_str(), cursorX, cursorY, &x1, &y1, &wpx, &hpx);

        // Advance cursor (word width + space)
        int16_t sx1, sy1;
        uint16_t sw, sh;
        display.getTextBounds(SPACEWIDTH_SYMBOL, cursorX, cursorY, &sx1, &sy1, &sw, &sh);

        cursorX += wpx + sw;
      }
      uint16_t boxWidth = map(cursorX, 0, display.width(), 0, 76);

      u8g2.drawBox(startX + 2, cursorY, boxWidth, max_hpx);

      // Move down for next line
      cursorY += max_hpx + 1;
    }

    // Blockquotes get a vertical line on the left
    if (style == '>' && (cursorY - 1) > 0) {
      u8g2.drawVLine((specialPadding / 2), startY, (cursorY - startY));
    }

    // Code Blocks get a vertical line on each side
    else if (style == 'C' && (cursorY - 1) > 0) {
      u8g2.drawVLine(SPECIAL_PADDING / 4 - 3, startY, (cursorY - startY));
      u8g2.drawVLine(76 - (SPECIAL_PADDING / 4) + 3, startY, (cursorY - startY));
    }

    // Headings get a horizontal line below them
    else if ((style == '1' || style == '2' || style == '3') && (cursorY - 2) > 0) {
      u8g2.drawHLine(0, cursorY - 2, 80);
    }

    // Ordered lists get a glyph
    else if (style == 'L' && (startY+1) > 0) {
      //u8g2.drawBox(startX - 3, startY, 2, 2);
      u8g2.drawVLine(startX-3, startY, 2);
      u8g2.drawPixel(startX-1, startY+1);
    }
    // Unordered lists get a '-'
    else if (style == '-' && (startY) > 0)
      u8g2.drawHLine(startX-3, startY, 2);

    return cursorY - startY;
  }

 private:
  // Helper: split a string segment into words and push them into words vector
  void splitIntoWords(const String& segment, bool bold, bool italic) {
    int start = 0;
    while (start < segment.length()) {
      int nextSpace = segment.indexOf(' ', start);
      if (nextSpace == -1)
        nextSpace = segment.length();
      String word = segment.substring(start, nextSpace);
      if (word.length() > 0) {
        words.push_back({word, bold, italic});
      }
      start = nextSpace + 1;
    }
  }
};

ulong editingLine_index = 0;
std::vector<DocLine> docLines;

// ------------------ Rendering ------------------

// Count number of display lines
int getTotalDisplayLines() {
  int total = 0;
  for (const auto& doc : docLines) {
    total += doc.lines.size();
  }
  return total;
}

// Display the entire document
int displayDocument(int startX = 0, int startY = 0) {
  int cursorY = startY;

  for (auto& doc : docLines) {
    // Display this DocLine, offset by current cursorY
    int heightUsed = doc.displayLine(startX, cursorY);

    // If the line is off the bottom of the screen, stop drawing
    if (cursorY > display.height())
      break;

    cursorY += heightUsed;
  }

  // Return total height used
  return cursorY - startY;
}

int displayDocumentPreview(int startX = 0, int startY = 0) {
  int cursorY = startY;

  for (auto& doc : docLines) {
    // Display this DocLine, offset by current cursorY
    int heightUsed = doc.displayLinePreview(startX, cursorY);

    // If the line is off the bottom of the screen, stop drawing
    if (cursorY > u8g2.getDisplayHeight())
      break;

    cursorY += heightUsed;
  }

  // Return total height used
  return cursorY - startY;
}

bool lineHasText(const LineObject& lineObj) {
  // Check if line has any words
  if (lineObj.words.empty())
    return false;

  // Check if any word has non-empty text
  for (const auto& w : lineObj.words) {
    if (w.text.length() > 0)
      return true;
  }

  return false;
}

void toolBar(wordObject& wordObj) {
  // FN/SHIFT indicator centered
  u8g2.setFont(u8g2_font_5x7_tf);

  switch (CurrentKBState) {
    case 1:
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("SHIFT")) / 2,
                   u8g2.getDisplayHeight(), "SHIFT");
      break;
    case 2:
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth("FN")) / 2, u8g2.getDisplayHeight(),
                   "FN");
      break;
    default:
      break;
  }

  // Show line type
  char currentDocLineType = docLines[editingLine_index].style;
  String lineTypeLabel;

  switch (currentDocLineType) {
    case 'T':
      lineTypeLabel = "BODY";
      break;
    case '1':
      lineTypeLabel = "H1";
      break;
    case '2':
      lineTypeLabel = "H2";
      break;
    case '3':
      lineTypeLabel = "H3";
      break;
    case 'C':
      lineTypeLabel = "CODE";
      break;
    case '>':
      lineTypeLabel = "QUOTE";
      break;
    case '-':
      lineTypeLabel = "U LIST";
      break;
    case 'L':
      lineTypeLabel = "O LIST";
      break;
    case 'H':
      lineTypeLabel = "H RULE";
      break;
    case 'B':
      lineTypeLabel = "BLANK LINE";
      break;
    default:
      lineTypeLabel = "";
      break;  // fallback if none match
  }

  if (lineTypeLabel.length() > 0) {
    u8g2.drawStr(0, u8g2.getDisplayHeight(), lineTypeLabel.c_str());
  }

  // Show edit type
  if (currentEditMode == edit_append) {
    u8g2.drawStr(u8g2.getStrWidth(lineTypeLabel.c_str()) + 4, u8g2.getDisplayHeight(),
                 " -> APPEND");
  } else if (currentEditMode == edit_inline) {
    u8g2.drawStr(u8g2.getStrWidth(lineTypeLabel.c_str()) + 4, u8g2.getDisplayHeight(),
                 " -> INLINE");
  }

  // Bold and italic indicator
  if (wordObj.bold == true && wordObj.italic == true) {
    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth("BOLD+ITALIC"), u8g2.getDisplayHeight(),
                 "BOLD+ITALIC");
  } else if (wordObj.bold == true && wordObj.italic == false) {
    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth("BOLD"), u8g2.getDisplayHeight(),
                 "BOLD");
  } else if (wordObj.bold == false && wordObj.italic == true) {
    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth("ITALIC"), u8g2.getDisplayHeight(),
                 "ITALIC");
  } else {
    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth("NORMAL"), u8g2.getDisplayHeight(),
                 "NORMAL");
  }
}

void setFontOLED(bool bold, bool italic) {
  if (bold && italic)
    u8g2.setFont(u8g2_font_luBIS18_tf);  // bold italics
  else if (bold && !italic)
    u8g2.setFont(u8g2_font_luBS18_tf);  // bold
  else if (!bold && italic)
    u8g2.setFont(u8g2_font_luIS18_tf);  // italics
  else
    u8g2.setFont(u8g2_font_lubR18_tf);  // regular
  return;
}

LineObject* getLineObjectByIndex(ulong targetIndex) {
  for (auto& doc : docLines) {
    for (auto& line : doc.lines) {
      if (line.index == targetIndex) {
        return &line;
      }
    }
  }
  return nullptr;  // not found
}

char getStyleFromScrollLine(ulong scrollLineIndex) {
  for (auto& doc : docLines) {
    for (auto& line : doc.lines) {
      if (line.index == scrollLineIndex) {
        return doc.style;  // <-- Return the DocLine style
      }
    }
  }
  return 'T';  // fallback if not found
}

// Returns the pixel width of a LineObject on the OLED (vector of wordObjects)
int getLineWidthOLED(const LineObject& lineObj) {
  int lineWidth = 0;
  for (const auto& w : lineObj.words) {
    setFontOLED(w.bold, w.italic);

    uint16_t wpx = u8g2.getStrWidth(w.text.c_str());

    int spaceWidth = u8g2.getStrWidth(" " /*SPACEWIDTH_SYMBOL*/);

    // Add word width + space width (except after last word)
    lineWidth += wpx;
    if (&w != &lineObj.words.back()) {
      lineWidth += spaceWidth;
    }
  }
  return lineWidth;
}

void scrollPreview() {
  u8g2.clearBuffer();

  uint16_t xInit = u8g2.getDisplayWidth() / 3;

  LineObject* scrollLinePtr = getLineObjectByIndex(lineScroll);
  if (!scrollLinePtr) {
    // Pointer invalid, nothing to display
    return;
  }

  LineObject scrollLine = *getLineObjectByIndex(lineScroll);

  if (&scrollLine) {
    // Display Line

    uint16_t xpos = xInit;

    // Iterate through line and display from left to right
    for (size_t i = 0; i < scrollLine.words.size(); ++i) {
      const auto& w = scrollLine.words[i];
      setFontOLED(w.bold, w.italic);
      u8g2.drawStr(xpos, 20, w.text.c_str());

      uint16_t wpx = u8g2.getStrWidth(w.text.c_str());

      // Only add space if not the last word
      if (i < scrollLine.words.size() - 1) {
        uint8_t spaceWidth = u8g2.getStrWidth(" ");
        xpos += wpx + spaceWidth;
      } else {
        xpos += wpx;  // just the word width
      }
    }

    // Draw line number and type
    char style = getStyleFromScrollLine(lineScroll);
    String lineTypeLabel = "";

    switch (style) {
      case 'T':
        lineTypeLabel = "BODY";
        break;
      case '1':
        lineTypeLabel = "HEAD 1";
        break;
      case '2':
        lineTypeLabel = "HEAD 2";
        break;
      case '3':
        lineTypeLabel = "HEAD 3";
        break;
      case 'C':
        lineTypeLabel = "CODE BLK";
        break;
      case '>':
        lineTypeLabel = "QUOTE BLK";
        break;
      case '-':
        lineTypeLabel = "UNORD LIST";
        break;
      case 'L':
        lineTypeLabel = "ORDER LIST";
        break;
      case 'H':
        lineTypeLabel = "HORIZ RULE";
        break;
      case 'B':
        lineTypeLabel = "BLANK LINE";
        break;
      default:
        lineTypeLabel = "?";
        break;
    }

    String lineInfoStr = "L:" + String(lineScroll) + "-" + lineTypeLabel;

    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(xInit, u8g2.getDisplayHeight(), lineInfoStr.c_str());

    // Draw tooltip
    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth("Tab:Edit Inline"),
                 u8g2.getDisplayHeight(), "Tab:Edit Inline");

    // Draw Seperator
    u8g2.drawVLine(80, 0, u8g2.getDisplayHeight());

    // Draw Preview
    int totalUsed = displayDocumentPreview(0, 0);

  } else {
    return;
  }
  u8g2.sendBuffer();
}

void oledEditorDisplay(LineObject& lineObj, wordObject& currentWord, int pixelsUsed,
                       bool currentlyTyping) {
  u8g2.clearBuffer();

  // Draw line text
  if (getLineWidthOLED(lineObj) < (u8g2.getDisplayWidth() - 5)) {
    uint16_t xpos = 0;

    // Iterate through line and display from left to right
    for (size_t i = 0; i < lineObj.words.size(); ++i) {
      const auto& w = lineObj.words[i];
      setFontOLED(w.bold, w.italic);
      u8g2.drawStr(xpos, 20, w.text.c_str());

      uint16_t wpx = u8g2.getStrWidth(w.text.c_str());

      // Only add space if not the last word
      if (i < lineObj.words.size() - 1) {
        uint8_t spaceWidth = u8g2.getStrWidth(" ");
        xpos += wpx + spaceWidth;
      } else {
        xpos += wpx;  // just the word width
      }
    }

    if (lineHasText(lineObj))
      u8g2.drawVLine(xpos + 2, 1, 22);
  } else {
    // Line is too long to fit, display from right to left
    uint16_t xpos = u8g2.getDisplayWidth() - 8;

    for (size_t i = 0; i < lineObj.words.size(); ++i) {
      const auto& w = lineObj.words[lineObj.words.size() - 1 - i];
      setFontOLED(w.bold, w.italic);

      uint16_t wpx = u8g2.getStrWidth(w.text.c_str());

      // Subtract spacing *only if not the rightmost word*
      if (i == 0) {
        xpos -= wpx;  // rightmost word
      } else {
        uint8_t spaceWidth = u8g2.getStrWidth(" ");
        xpos -= (wpx + spaceWidth);
      }

      // Draw word if it's on the screen
      if ((xpos + wpx) > 0) {
        u8g2.drawStr(xpos, 20, w.text.c_str());
      }
    }

    u8g2.drawVLine(u8g2.getDisplayWidth() - 6, 1, 22);
  }

  // PROGRESS BAR
  if (lineHasText(lineObj) == true && pixelsUsed > 0) {
    if (pixelsUsed > display.width() - DISPLAY_WIDTH_BUFFER)
      pixelsUsed = display.width() - DISPLAY_WIDTH_BUFFER;
    // uint8_t progress = map(pixelsUsed, 0, display.width() - DISPLAY_WIDTH_BUFFER, 0,
    // u8g2.getDisplayWidth());
    uint8_t progress = map(min(pixelsUsed, display.width() - DISPLAY_WIDTH_BUFFER), 0,
                           display.width() - DISPLAY_WIDTH_BUFFER, 0, u8g2.getDisplayWidth() - 1);

    u8g2.drawVLine(u8g2.getDisplayWidth(), 0, 2);
    u8g2.drawVLine(0, 0, 2);

    u8g2.drawHLine(0, 0, progress);
    u8g2.drawHLine(0, 1, progress);
    // u8g2.drawHLine(0, 2, progress);

    // LINE END WARNING INDICATOR
    if (progress > (u8g2.getDisplayWidth() * 0.8)) {
      if ((millis() / 400) % 2 == 0) {  // ON for 200ms, OFF for 200ms
        u8g2.drawVLine(u8g2.getDisplayWidth() - 1, 8, 32 - 16);
        u8g2.drawLine(u8g2.getDisplayWidth() - 1, 15, u8g2.getDisplayWidth() - 4, 12);
        u8g2.drawLine(u8g2.getDisplayWidth() - 1, 15, u8g2.getDisplayWidth() - 4, 18);
      }
    }
    // New line on space animation
    if (pixelsUsed >= display.width() - DISPLAY_WIDTH_BUFFER) {
      // Sawtooth animation
      uint period = 8000;
      uint x1 = map(millis() % period, 0, period, 0, u8g2.getDisplayWidth());
      uint x2 = map((millis() + period / 4) % period, 0, period, 0, u8g2.getDisplayWidth());
      uint x3 = map((millis() + period / 2) % period, 0, period, 0, u8g2.getDisplayWidth());
      uint x4 = map((millis() + (3 * period) / 4) % period, 0, period, 0, u8g2.getDisplayWidth());

      // Draw scrolling box
      u8g2.setDrawColor(0);
      u8g2.drawBox(x1, 0, 10, 2);
      u8g2.drawBox(x2, 0, 10, 2);
      u8g2.drawBox(x3, 0, 10, 2);
      u8g2.drawBox(x4, 0, 10, 2);
      u8g2.setDrawColor(1);
    }
  }

  if (currentlyTyping) {
    // Show toolbar
    toolBar(currentWord);
  } else {
    // Show infobar
    OLED().infoBar();
  }

  u8g2.sendBuffer();
}

// ------------------ Document ------------------

// Parse and split all DocLines into rendered lines
void populateLines(std::vector<DocLine>& docLines) {
  indexCounter = 0;

  for (auto& doc : docLines) {
    doc.parseWords();
    doc.splitToLines();
  }
}

void refreshOrderedListIndexes() {
  int currentNumber = 0;
  bool prevWasList = false;

  for (auto& dl : docLines) {
    if (dl.style == 'L') {
      currentNumber = prevWasList ? currentNumber + 1 : 1;
      dl.orderedListNumber = currentNumber;
      prevWasList = true;
    } else {
      dl.orderedListNumber = -1;
      prevWasList = false;
    }
  }
}

void refreshAllLineIndexes() {
  // Refresh line indexes
  indexCounter = 0;                     // reset counter if you want indexes to start from 0
  for (auto& docLine : docLines) {      // iterate through all DocLines
    for (auto& line : docLine.lines) {  // iterate through each LineObject
      line.index = indexCounter++;
    }
  }

  // Update list indexes
  refreshOrderedListIndexes();
}

// Load File
void loadMarkdownFile(const String& path) {
  if (noSD) {
    OLED().oledWord("LOAD FAILED - No SD!");
    delay(5000);
    return;
  }

  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);

  docLines.clear();
  File file = SD_MMC.open(path.c_str(), FILE_READ);
  if (!file) {
    ESP_LOGE("SD", "File does not exist: %s", path.c_str());  // FIXME: - Come up with better error handling
                                                              //        - Should this be Error or Warning?
    OLED().oledWord("LOAD FAILED - FILE MISSING");
    delay(2000);

    // Create an empty new docLines object
    docLines.push_back({'T', "", {}});
    editingLine_index = 0;

    // Populate and update as usual so UI doesn’t crash
    populateLines(docLines);
    refreshAllLineIndexes();

    if (SAVE_POWER)
      setCpuFrequencyMhz(80);
    SDActive = false;
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    char style = 'T';
    String content = line;  // default is full line

    if (line.length() == 0) {
      style = 'B'; // Blank line
      content = "";
    } else if (line.startsWith("### ")) {
      style = '3'; // Heading 3
      content = line.substring(4);  // remove "### "
    } else if (line.startsWith("## ")) {
      style = '2'; // Heading 2
      content = line.substring(3);  // remove "## "
    } else if (line.startsWith("# ")) {
      style = '1'; // Heading 1
      content = line.substring(2);  // remove "# "
    } else if (line.startsWith("> ")) {
      style = '>'; // Quote Block
      content = line.substring(2);  // remove "> "
    } else if (line.startsWith("- ")) {
      style = '-'; // Unordered List
      content = line.substring(2); // remove "- "
    } else if (line == "---") {
      style = 'H'; // Horizontal Rule
      content = "---";  // horizontal line has no content
    } else if ((line.startsWith("```")) || (line.startsWith("`") && line.endsWith("`")) || (line.startsWith("```") && line.endsWith("```"))) {
      if (line.startsWith("```"))
        content = line.substring(3);
      else if (line.startsWith("```") && line.endsWith("```"))
        content = line.substring(3, line.length() - 3);
      else if (line.startsWith("`") && line.endsWith("`"))
        content = line.substring(1, line.length() - 1);

      style = 'C'; // Code Block
    } else if (line.length() > 2 && isDigit(line.charAt(0)) && line.charAt(1) == '.' &&
               line.charAt(2) == ' ') {
      style = 'L'; // Ordered List
      content = line.substring(3); // remove "1. ", "2. ", etc.
    }

    docLines.push_back({style, content, {}});
  }

  file.close();

  if (docLines.empty()) {
    docLines.push_back({'T', "", {}});
    editingLine_index = 0;
  } else {
    editingLine_index = docLines.size() - 1;
  }

  // Populate all the lines
  populateLines(docLines);

  // Update indexes
  refreshAllLineIndexes();

  if (SAVE_POWER)
    setCpuFrequencyMhz(80);
  SDActive = false;

  OLED().oledWord("FILE LOADED");
  delay(500);
}

void saveMarkdownFile(const String& path) {
  if (noSD) {
    OLED().oledWord("SAVE FAILED - No SD!");
    delay(3000);
    return;
  }

  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);

  // Determine save path
  String savePath = path;
  if (savePath == "" || savePath == "-")
    savePath = "/temp.txt";
  if (!savePath.startsWith("/"))
    savePath = "/" + savePath;

  File file = SD_MMC.open(savePath.c_str(), FILE_WRITE);
  if (!file) {
    OLED().oledWord("SAVE FAILED - OPEN ERR");
    delay(2000);
    ESP_LOGE("SD", "Failed to open file for writing: %s", savePath.c_str());
    SDActive = false;
    return;
  }

  // Write each DocLine as Markdown
  for (auto &dl : docLines) {
    dl.compileToText();

    String out;

    switch (dl.style) {
      case '1': out = "# " + dl.line; break;
      case '2': out = "## " + dl.line; break;
      case '3': out = "### " + dl.line; break;
      case '>': out = "> " + dl.line; break;
      case '-': out = "- " + dl.line; break;
      case 'L': out = "1. " + dl.line; break; //String(dl.orderedListNumber) + ". " + dl.line; break;
      case 'H': out = "---"; break;
      case 'C': out = "```" + dl.line + "```"; break;
      case 'B': out = ""; break;
      default:  out = dl.line; break;
    }

    file.println(out);
  }

  file.close();

  // Save metadata
  pocketmage::file::writeMetadata(savePath);
  editingFile = savePath;

  OLED().oledWord("Saved: " + savePath);
  delay(1000);

  if (SAVE_POWER)
    setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

void newMarkdownFile(const String& path) {
  if (noSD) {
    OLED().oledWord("SAVE FAILED - No SD!");
    delay(3000);
    return;
  }

  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);

  // Determine save path
  String savePath = path;
  if (savePath == "" || savePath == "-")
    savePath = "/temp.txt";
  if (!savePath.startsWith("/"))
    savePath = "/" + savePath;

  File file = SD_MMC.open(savePath.c_str(), FILE_WRITE);
  if (!file) {
    OLED().oledWord("SAVE FAILED - OPEN ERR");
    delay(2000);
    ESP_LOGE("SD", "Failed to open file for writing: %s", savePath.c_str());
    SDActive = false;
    return;
  }

  // Write nothing

  file.close();

  // Save metadata
  pocketmage::file::writeMetadata(savePath);
  editingFile = savePath;

  OLED().oledWord("Created: " + savePath);
  delay(1000);

  if (SAVE_POWER)
    setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

// Returns the pixel width of a LineObject (vector of wordObjects)
int getLineWidth(const LineObject& lineObj, char style) {
  int lineWidth = 0;
  for (const auto& w : lineObj.words) {
    const GFXfont* font = pickFont(style, w.bold, w.italic);
    display.setFont(font);

    int16_t x1, y1;
    uint16_t wpx, hpx;
    display.getTextBounds(w.text.c_str(), 0, 0, &x1, &y1, &wpx, &hpx);

    uint16_t spaceWidth;
    display.getTextBounds(SPACEWIDTH_SYMBOL, 0, 0, &x1, &y1, &spaceWidth, &hpx);

    // Add word width + space width (except after last word)
    lineWidth += (wpx + WORDWIDTH_BUFFER);
    if (&w != &lineObj.words.back()) {
      lineWidth += spaceWidth;
    }
  }
  return lineWidth;
}

void editAppend(char inchar) {
  static ulong lastTypeMillis = 0;
  ulong currentMillis = millis();

  bool moveView = false;

  // Lower baseline clock speed here?

  // Direct access to DocLine, LineObject, and wordObject
  DocLine& editingDocLine = docLines[editingLine_index];
  LineObject* lastLine;
  wordObject* lastWord;

  // Ensure we have at least one line
  if (editingDocLine.lines.empty()) {
    LineObject blankLine;
    blankLine.index = indexCounter++;
    editingDocLine.lines.push_back(blankLine);
  }
  lastLine = &editingDocLine.lines.back();

  // Ensure we have at least one word
  if (lastLine->words.empty()) {
    lastLine->words.push_back({"", false, false});
  }
  lastWord = &lastLine->words.back();

  if (inchar != 0) {
    // Increase clock speed here for faster processing?
    setCpuFrequencyMhz(240);
  }

  // HANDLE INPUTS
  // No char recieved
  if (inchar == 0) {
  }
  // Return home
  else if (inchar == 12) {
    HOME_INIT();
  }
  // TAB Recieved
  else if (inchar == 9) {
    // If scrolling, edit inline
    if (lastTouch != -1) {
    }

  }
  // SHIFT Recieved
  else if (inchar == 17) {
    if (CurrentKBState == SHIFT)
      CurrentKBState = NORMAL;
    else
      CurrentKBState = SHIFT;
  }
  // FN Recieved
  else if (inchar == 18) {
    if (CurrentKBState == FUNC)
      CurrentKBState = NORMAL;
    else
      CurrentKBState = FUNC;
  }
  // Space Recieved
  else if (inchar == 32) {
    if (getLineWidth(*lastLine, editingDocLine.style) > display.width() - DISPLAY_WIDTH_BUFFER) {
      // Word does not fit -> wrap to new line
      // Remove the word from the old line
      wordObject movedWord = std::move(*lastWord);
      lastLine->words.pop_back();

      // Create new line, move the word into it
      LineObject newLine;
      newLine.words.push_back(std::move(movedWord));
      editingDocLine.lines.push_back(std::move(newLine));

      // Update lastLine and lastWord
      lastLine = &editingDocLine.lines.back();
      lastWord = &lastLine->words.back();

      // Update line indexes
      refreshAllLineIndexes();

      // Mark screen for update
      updateScreen = true;
      moveView = true;
    }

    // Start a new empty word for the next input
    wordObject newWord;
    newWord.text = "";
    newWord.bold = false;
    newWord.italic = false;
    lastLine->words.push_back(std::move(newWord));
    lastWord = &lastLine->words.back();
  }
  // ENTER Received
  else if (inchar == 13) {
    // Horizontal Rule
    if (editingDocLine.style == 'H') {
      editingDocLine.line = "---";
      editingDocLine.words.clear();
      editingDocLine.parseWords();
      editingDocLine.splitToLines();
    }
    // Blank Line
    bool currentLineEmpty = true;
    for (auto& ln : editingDocLine.lines) {
      if (lineHasText(ln)) {
        currentLineEmpty = false;
        break;
      }
    }
    if (currentLineEmpty) {
      editingDocLine.style = 'B';
    }

    // Retain style on next line for certain styles
    char nextLineStyle = editingDocLine.style;
    if (nextLineStyle == 'C' || nextLineStyle == '>' || nextLineStyle == '-' || nextLineStyle == 'L') {
        // keep same style
    } else {
        nextLineStyle = 'T'; // fallback to body text
    }

    // Wrap current word if it doesn't fit
    if (getLineWidth(*lastLine, editingDocLine.style) > display.width() - DISPLAY_WIDTH_BUFFER) {
      wordObject movedWord = std::move(*lastWord);
      lastLine->words.pop_back();

      LineObject newLine;
      newLine.words.push_back(std::move(movedWord));
      editingDocLine.lines.push_back(std::move(newLine));

      lastLine = &editingDocLine.lines.back();
      lastWord = &lastLine->words.back();
    }

    // Finish current DocLine and create a new one
    DocLine newDocLine;
    newDocLine.style = nextLineStyle;

    // Add one line and one empty word
    LineObject newLine;
    newLine.index = indexCounter++;
    newLine.words.push_back({"", false, false});
    newDocLine.lines.push_back(std::move(newLine));

    // Insert new DocLine immediately after the current one
    editingLine_index++;
    docLines.insert(docLines.begin() + editingLine_index, std::move(newDocLine));

    // Update lastLine/lastWord to point to new line
    lastLine = &docLines[editingLine_index].lines.back();
    lastWord = &lastLine->words.back();

    // Refresh all line indexes
    refreshAllLineIndexes();

    // Mark screen for update
    updateScreen = true;
    moveView = true;
  }
  // ESC / CLEAR Recieved
  else if (inchar == 20) {
  }
  // LEFT
  else if (inchar == 19) {
  }
  // RIGHT
  else if (inchar == 21) {
  }
  // SHFT + LEFT (Text type select)
  else if (inchar == 28) {
    // Define the cycle order
    static const char styleCycle[] = {'T', '1', '2', '3', '>', 'L', '-', 'C', 'H'};
    static const int numStyles = sizeof(styleCycle) / sizeof(styleCycle[0]);

    // Find current style index
    int currentIndex = 0;
    for (int i = 0; i < numStyles; i++) {
      if (editingDocLine.style == styleCycle[i]) {
        currentIndex = i;
        break;
      }
    }

    // Move to next style in cycle
    currentIndex = (currentIndex + 1) % numStyles;
    editingDocLine.style = styleCycle[currentIndex];
  }
  // SHFT + RIGHT (Word type select)
  else if (inchar == 30) {
    if (lastWord->bold == false && lastWord->italic == false) {
      // If regular text switch to bold
      lastWord->bold = true;
      lastWord->italic = false;
    } else if (lastWord->bold == true && lastWord->italic == false) {
      // If bold switch to italic
      lastWord->bold = false;
      lastWord->italic = true;
    } else if (lastWord->bold == false && lastWord->italic == true) {
      // If italic switch to bold+italic
      lastWord->bold = true;
      lastWord->italic = true;
    } else if (lastWord->bold == true && lastWord->italic == true) {
      // If bold+italic switch to regular text
      lastWord->bold = false;
      lastWord->italic = false;
    }
  }
  // BKSP Received
  else if (inchar == 8) {
    if (lastWord->text.length() > 0) {
      // Remove the last character of the current word
      lastWord->text.remove(lastWord->text.length() - 1);
    } else {
      // Current word is empty, move to previous word or line
      LineObject* linePtr = lastLine;
      wordObject* wordPtr = lastWord;

      if (linePtr->words.size() > 1) {
        // Pop the empty word and go to the previous word in the same line
        linePtr->words.pop_back();
        wordPtr = &linePtr->words.back();
      } else {
        // First word in the line
        DocLine& docLineRef = docLines[editingLine_index];

        if (docLineRef.lines.size() > 1) {
          // Move to previous LineObject in the same DocLine
          docLineRef.lines.pop_back();
          linePtr = &docLineRef.lines.back();
          wordPtr = &linePtr->words.back();
        } else if (editingLine_index > 0) {
          // Move to previous DocLine
          editingLine_index--;
          DocLine& prevDocLine = docLines[editingLine_index];
          linePtr = &prevDocLine.lines.back();
          wordPtr = &linePtr->words.back();
        } else {
          // At very start of document, nothing to do
          return;
        }
      }

      // Update lastLine / lastWord references
      lastLine = linePtr;
      lastWord = wordPtr;
    }
  }
  // SAVE Recieved
  else if (inchar == 6) {
    String savePath = editingFile;
    if (savePath == "" || savePath == "-" || savePath == "/temp.txt") {
      CurrentKBState = NORMAL;
      CurrentTXTState_NEW = SAVE_AS;
      return;
    }
    if (!savePath.startsWith("/")) savePath = "/" + savePath;
    
    saveMarkdownFile(editingFile);
  }
  // NEW FILE Recieved
  else if (inchar == 30) {
    CurrentKBState = NORMAL;
    CurrentTXTState_NEW = SAVE_AS;
  }
  // FILE Recieved
  else if (inchar == 7) {
    CurrentKBState = NORMAL;
    CurrentTXTState_NEW = LOAD_FILE;
  }
  // Font Switcher
  else if (inchar == 14) {
    CurrentKBState = FUNC;
    updateScreen = true;
    CurrentTXTState_NEW = FONT;
  } else {
    // Add char to current word
    lastWord->text += inchar;

    if (inchar >= 48 && inchar <= 57) {
    }  // Only leave FN on if typing numbers
    else if (CurrentKBState != NORMAL) {
      CurrentKBState = NORMAL;
    }
  }

  if (inchar != 0) {
    // Typing is happening
    lastTypeMillis = millis();
  }

  currentMillis = millis();
  // Make sure oled only updates at 60fps
  if (currentMillis - OLEDFPSMillis >= (1000 / 60)) {
    OLEDFPSMillis = currentMillis;
    // Show line on OLED when not actively scrolling
    if (lastTouch == -1) {
      bool currentlyTyping = (millis() - lastTypeMillis < TYPE_INTERFACE_TIMEOUT);

      // Flush KB IC if not in use
      if (!currentlyTyping)
        keypad.flush();

      int lineWidth = getLineWidth(*lastLine, editingDocLine.style);

      oledEditorDisplay(*lastLine, *lastWord, lineWidth, currentlyTyping);
    } else {
      // Scrolling display function here
      scrollPreview();
    }
  }

  // Center scroll on typed line if a line update has been registered
  if (moveView) {
    // Update scroll to currently edited line
    if (editingDocLine.lines.empty())
      lineScroll = 0;
    else
      lineScroll = editingDocLine.lines.back().index;
  }

  if (SAVE_POWER)
    setCpuFrequencyMhz(POWER_SAVE_FREQ);
}

// INIT
void initFonts() {
  // Mono
  fonts[mono].normal = &FreeMono9pt7b;
  fonts[mono].normal_B = &FreeMonoBold9pt7b;
  fonts[mono].normal_I = &FreeMonoOblique9pt7b;
  fonts[mono].normal_BI = &FreeMonoBoldOblique9pt7b;

  fonts[mono].h1 = &FreeMonoBold24pt7b;
  fonts[mono].h1_B = &FreeMonoBold24pt7b;  // Already bold
  fonts[mono].h1_I = &FreeMonoBoldOblique24pt7b;
  fonts[mono].h1_BI = &FreeMonoBoldOblique24pt7b;

  fonts[mono].h2 = &FreeMonoBold18pt7b;
  fonts[mono].h2_B = &FreeMonoBold18pt7b;
  fonts[mono].h2_I = &FreeMonoBoldOblique18pt7b;
  fonts[mono].h2_BI = &FreeMonoBoldOblique18pt7b;

  fonts[mono].h3 = &FreeMonoBold12pt7b;
  fonts[mono].h3_B = &FreeMonoBold12pt7b;
  fonts[mono].h3_I = &FreeMonoBoldOblique12pt7b;
  fonts[mono].h3_BI = &FreeMonoBoldOblique12pt7b;

  fonts[mono].code = &FreeMono9pt7b;
  fonts[mono].code_B = &FreeMono9pt7b;
  fonts[mono].code_I = &FreeMono9pt7b;
  fonts[mono].code_BI = &FreeMono9pt7b;

  fonts[mono].quote = &FreeMono9pt7b;
  fonts[mono].quote_B = &FreeMonoBold9pt7b;
  fonts[mono].quote_I = &FreeMonoOblique9pt7b;
  fonts[mono].quote_BI = &FreeMonoBoldOblique9pt7b;

  fonts[mono].list = &FreeMono9pt7b;
  fonts[mono].list_B = &FreeMonoBold9pt7b;
  fonts[mono].list_I = &FreeMonoOblique9pt7b;
  fonts[mono].list_BI = &FreeMonoBoldOblique9pt7b;

  // Serif
  fonts[serif].normal = &FreeSerif9pt7b;
  fonts[serif].normal_B = &FreeSerifBold9pt7b;
  fonts[serif].normal_I = &FreeSerifItalic9pt7b;
  fonts[serif].normal_BI = &FreeSerifBoldItalic9pt7b;

  fonts[serif].h1 = &FreeSerifBold24pt7b;
  fonts[serif].h1_B = &FreeSerifBold24pt7b;
  fonts[serif].h1_I = &FreeSerifBoldItalic24pt7b;
  fonts[serif].h1_BI = &FreeSerifBoldItalic24pt7b;

  fonts[serif].h2 = &FreeSerifBold18pt7b;
  fonts[serif].h2_B = &FreeSerifBold18pt7b;
  fonts[serif].h2_I = &FreeSerifBoldItalic18pt7b;
  fonts[serif].h2_BI = &FreeSerifBoldItalic18pt7b;

  fonts[serif].h3 = &FreeSerifBold12pt7b;
  fonts[serif].h3_B = &FreeSerifBold12pt7b;
  fonts[serif].h3_I = &FreeSerifBoldItalic12pt7b;
  fonts[serif].h3_BI = &FreeSerifBoldItalic12pt7b;

  fonts[serif].code = &FreeMono9pt7b;
  fonts[serif].code_B = &FreeMono9pt7b;
  fonts[serif].code_I = &FreeMono9pt7b;
  fonts[serif].code_BI = &FreeMono9pt7b;

  fonts[serif].quote = &FreeSerif9pt7b;
  fonts[serif].quote_B = &FreeSerifBold9pt7b;
  fonts[serif].quote_I = &FreeSerifItalic9pt7b;
  fonts[serif].quote_BI = &FreeSerifBoldItalic9pt7b;

  fonts[serif].list = &FreeSerif9pt7b;
  fonts[serif].list_B = &FreeSerifBold9pt7b;
  fonts[serif].list_I = &FreeSerifItalic9pt7b;
  fonts[serif].list_BI = &FreeSerifBoldItalic9pt7b;

  // Sans
  fonts[sans].normal = &FreeSans9pt7b;
  fonts[sans].normal_B = &FreeSansBold9pt7b;
  fonts[sans].normal_I = &FreeSansOblique9pt7b;
  fonts[sans].normal_BI = &FreeSansBoldOblique9pt7b;

  fonts[sans].h1 = &FreeSansBold24pt7b;
  fonts[sans].h1_B = &FreeSansBold24pt7b;
  fonts[sans].h1_I = &FreeSansBoldOblique24pt7b;
  fonts[sans].h1_BI = &FreeSansBoldOblique24pt7b;

  fonts[sans].h2 = &FreeSansBold18pt7b;
  fonts[sans].h2_B = &FreeSansBold18pt7b;
  fonts[sans].h2_I = &FreeSansBoldOblique18pt7b;
  fonts[sans].h2_BI = &FreeSansBoldOblique18pt7b;

  fonts[sans].h3 = &FreeSansBold12pt7b;
  fonts[sans].h3_B = &FreeSansBold12pt7b;
  fonts[sans].h3_I = &FreeSansBoldOblique12pt7b;
  fonts[sans].h3_BI = &FreeSansBoldOblique12pt7b;

  fonts[sans].code = &FreeMono9pt7b;
  fonts[sans].code_B = &FreeMono9pt7b;
  fonts[sans].code_I = &FreeMono9pt7b;
  fonts[sans].code_BI = &FreeMono9pt7b;

  fonts[sans].quote = &FreeSans9pt7b;
  fonts[sans].quote_B = &FreeSansBold9pt7b;
  fonts[sans].quote_I = &FreeSansOblique9pt7b;
  fonts[sans].quote_BI = &FreeSansBoldOblique9pt7b;

  fonts[sans].list = &FreeSans9pt7b;
  fonts[sans].list_B = &FreeSansBold9pt7b;
  fonts[sans].list_I = &FreeSansOblique9pt7b;
  fonts[sans].list_BI = &FreeSansBoldOblique9pt7b;
}

void TXT_INIT() {
  initFonts();

  if (editingFile != "") {
    loadMarkdownFile(editingFile);
  }

  setFontStyle(serif);

  lineScroll = 0;
  updateScreen = true;
  CurrentAppState = TXT;
}

void einkHandler_TXT_NEW() {
  if (updateScreen) {
    updateScreen = false;
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    displayDocument();
    EINK().refresh();
    refreshAllLineIndexes();
  }
}

void processKB_TXT_NEW() {
  if (OLEDPowerSave) {
    u8g2.setPowerSave(0);
    OLEDPowerSave = false;
  }

  disableTimeout = false;
  String outPath = "";
  char inchar;

  unsigned long currentMillis = millis();

  switch (CurrentTXTState_NEW) {
    case TXT_:
      inchar = KB().updateKeypress();
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        // update scroll
        if (TOUCH().updateScroll(getTotalDisplayLines(), lineScroll)) {
          updateScreen = true;
        }
        switch (currentEditMode) {
          case edit_append:
            editAppend(inchar);
            break;
          case edit_inline:

            break;
        }
      }
      break;
    case SAVE_AS:
      inchar = KB().updateKeypress();
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        //CR Recieved
        else if (inchar == 13) {                          
          if (currentLine != "" && currentLine != "-") {
            if (!currentLine.startsWith("/notes/")) currentLine = "/notes/" + currentLine;
            if (!currentLine.endsWith(".txt")) currentLine = currentLine + ".txt";
            saveMarkdownFile(currentLine);
            CurrentTXTState_NEW = TXT_;
          } else {
            OLED().oledWord("Invalid Name");
            delay(2000);
          }
          
          currentLine = "";
        }                                      
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
        }
        //Space Recieved
        else if (inchar == 32) {                                  
          // Spaces not allowed in filenames
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentLine = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        // Home recieved
        else if (inchar == 12) {
          CurrentTXTState_NEW = TXT_;
        }
        else {
          currentLine += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL) {
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at OLED_MAX_FPS
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentLine, false, "Input Filename");
        }
      }
      break;
    case NEW_FILE:
      inchar = KB().updateKeypress();
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        //CR Recieved
        else if (inchar == 13) {                          
          if (currentLine != "" && currentLine != "-") {
            if (!currentLine.startsWith("/notes/")) currentLine = "/notes/" + currentLine;
            if (!currentLine.endsWith(".txt")) currentLine = currentLine + ".txt";
            newMarkdownFile(currentLine);
            CurrentTXTState_NEW = TXT_;
          } else {
            OLED().oledWord("Invalid Name");
            delay(2000);
          }
          
          currentLine = "";
        }                                      
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
        }
        //Space Recieved
        else if (inchar == 32) {                                  
          // Spaces not allowed in filenames
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentLine = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        // Home recieved
        else if (inchar == 12) {
          CurrentTXTState_NEW = TXT_;
        }
        else {
          currentLine += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL) {
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at OLED_MAX_FPS
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentLine, false, "Input Name for New File");
        }
      }
      break;
    case LOAD_FILE:
      outPath = fileWizardMini(false, "/notes");
      if (outPath == "_EXIT_") {
        // Return to TXT
        CurrentTXTState_NEW = TXT_;
        //newState = true;
        break;
      }
      else if (outPath != "") {
        // Ensure file is a .txt or .md
        if (outPath.endsWith(".txt") || outPath.endsWith(".md")) {
          if (!outPath.startsWith("/")) outPath = "/" + outPath;
          loadMarkdownFile(outPath);
          editingFile = outPath;
          CurrentTXTState_NEW = TXT_;
          updateScreen = true;
        } else {
          OLED().oledWord("Incompatible Filetype!");
          delay(2000);
          CurrentTXTState_NEW = TXT_;
        }
      }
      break;
  }
}
