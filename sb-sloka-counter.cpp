#include <cstdio>
#include <iostream>
#include <regex>
#include <stdexcept>
#include "rtfparser.h"

void parse_line(std::string const & line, CHP const & chp);

class SbSlokaCounter {
public:
    void write(std::string const & string, CHP const & chp) {
        if (int(chp.cur_font) != 0) return;
        parse_line(string, chp);
    }
};

bool starts_with(std::string const & s, std::string const & with) {
    return (s.compare(0, with.size(), with) == 0);
}

void error(char const * msg, int canto, int chapter, int text,
    int last_canto, int last_chapter, int last_text) {
    std::cerr << msg << ": " << canto << '.' << chapter << '.' << text
        << " (previous: " << last_canto << '.' << last_chapter << '.' << last_text << ")\n";
    std::exit(1);
}

void check_verse_range_numbers(int canto, int chapter, int text) {
    static int last_canto = 0;
    static int last_chapter = 0;
    static int last_text = 0;
    if (canto != last_canto) {
        if (canto != last_canto+1) {
            error("unexpected canto", canto, chapter, text, last_canto, last_chapter, last_text);
        }
        if (chapter != 1) {
            error("unexpected chapter", canto, chapter, text, last_canto, last_chapter, last_text);
        }
        if (text != 1) {
            error("unexpected text number", canto, chapter, text, last_canto, last_chapter, last_text);
        }
    } else if (chapter != last_chapter) {
        if (chapter != last_chapter+1) {
            error("unexpected chapter", canto, chapter, text, last_canto, last_chapter, last_text);
        }
        if (text != 1) {
            error("unexpected text number", canto, chapter, text, last_canto, last_chapter, last_text);
        }
    } else if (text != last_text+1) {
        error("unexpected text number", canto, chapter, text, last_canto, last_chapter, last_text);
    }
    last_canto = canto;
    last_chapter = chapter;
    last_text = text;
}

void parse_line(std::string const & line, CHP const & chp) {
    static std::string verse_range;
    if (verse_range.empty()) {
        // we only recognize "SB x.y.z" in the hidden text parts
        if (!chp.hidden) {
            return;
        }
        static std::regex r(R"regex(^SB ((\d+).(\d+).(\d+)))regex");
        std::smatch match;
        if (std::regex_search(line, match, r)) {
            //std::cout << "match: " << match.str(1) << '\n';
            verse_range = match.str(1);
            int canto = std::stoi(match.str(2));
            int chapter = std::stoi(match.str(3));
            int text = std::stoi(match.str(4));

            check_verse_range_numbers(canto, chapter, text);
            return;
        }
        //std::cout << "no match: " << line;
        return;
    }

    // verse range is not empty

    if (line == "SYNONYMS\n") {
            verse_range = "";
            std::cout << std::flush;
            return;
    }
    if (starts_with(line, "TEXT")) return; // skip lines like "TEXT 3" and "TEXT"
    std::cout << verse_range << ": " << line;
}

int main() {
    FILE *f = fopen("sb.rtf", "r");
    if (!f) {
        fprintf(stderr, "Can't open sb.rtf");
        return 1;
    }

    RtfParser<SbSlokaCounter> p;
    Status ec = p.RtfParse(f);
    if (ec != Status::OK) {
        fprintf(stderr, "error %d parsing RTF\n", int(ec));
    }
    fclose(f);
}
