#include <cstdio>
#include <iostream>
#include <regex>
#include <stdexcept>
#include "rtfparser.h"

class VerseRange {
public:
    VerseRange() = default;
    void start_text_range(std::string const & text_first, std::string const & text_last="") {
        text_first_ = text_first;
        text_last_ = !text_last.empty() ? text_last : text_first;
        check_numbers();
    }

    void start_chapter(std::string const & canto, std::string const & chapter) {
        canto_ = canto;
        chapter_ = chapter;
    }

    void clear() {
        text_first_ = ""; text_last_ = "";
    }
    bool empty() {
        return text_first_.empty();
    }
    void error(char const * msg) {
        std::cerr << msg << ": " << canto_ << '.' << chapter_ << '.' << text_first_ << '-' << text_last_
            << " (previous: " << prev_canto << '.' << prev_chapter << '.' << prev_text << ")\n";
        std::exit(1);
    }

    int verse_num(std::string const & s) {
        return atoi(s.c_str());
    }

    void check_numbers() {
        if (canto_ != prev_canto) {
            if (verse_num(canto_) != verse_num(prev_canto)+1) {
                error("unexpected canto");
            }
            if (chapter_ != "1") {
                error("unexpected chapter");
            }
            if (text_first_ != "1") {
                error("unexpected text number(1)");
            }
        } else if (chapter_ != prev_chapter) {
            if (verse_num(chapter_) != verse_num(prev_chapter)+1) {
                error("unexpected chapter");
            }
            if (text_first_ != "1") {
                error("unexpected text number(2)");
            }
        } else if (verse_num(text_first_) != verse_num(prev_text)+1) {
            if (canto_ == "4" && chapter_ == "29" && text_first_ == "1a" && prev_text == "85") {
                // it's OK, no error, just weird numbering in 4.29.85 => 4.29.1a-2a
            } else if (canto_ == "4" && chapter_ == "29" && text_first_ == "1b" && prev_text == "2a") {
                // it's OK, no error, just weird numbering in 4.29.1a-2a => 4.29.1b
            } else {
                error("unexpected text number(3)");
            }
        }
        if (verse_num(text_last_) < verse_num(text_first_)) {
            error("unexpected text range");
        }
        prev_canto = canto_;
        prev_chapter = chapter_;
        prev_text = text_last_;
    }

private:
    std::string canto_, chapter_, text_first_, text_last_;
    std::string prev_canto = "";
    std::string prev_chapter = "";
    std::string prev_text = "";

    friend std::ostream & operator << (std::ostream & stream, VerseRange & r);
};

std::ostream & operator << (std::ostream & stream, VerseRange & r) {
    stream << r.canto_ << '.' << r.chapter_ << '.' << r.text_first_;
    if (r.text_last_ != r.text_first_) {
        stream << '-' << r.text_last_;
    }
    return stream;
}

int total_syllables=0;
int total_syllables_no_uvaca=0;

class SbSlokaCounter {
public:
    void write(std::string const & string, CHP const & chp) {
        if (int(chp.cur_font) != 0) return;
        parse_line(string, chp);
    }

private:
    VerseRange verse_range;

    bool check_for_verse_start(std::string const & line) {
        static std::regex r(R"re(^TEXTS? (\d+[ab]?)(?:[-\x96]{1,2}(\d+[ab]?))?\n?$)re");
        std::smatch match;
        if (std::regex_search(line, match, r)) {
            verse_range.start_text_range(match.str(1), match.str(2));
            return true;
        }
        return false;
    }

    void show_matches(std::smatch const & m) {
        for (std::size_t n=0; n < m.size(); ++n) {
            std::cout << " m[" << n << "]='" << m.str(n) << "'\n";
        }
        std::cout << "suffix='" << m.suffix().str() << "'\n";
    }

    bool check_for_chapter_start(std::string const & line) {
        static std::regex r(R"re(^SB (\d+).(\d+):)re");
        std::smatch match;

        if (std::regex_search(line, match, r)) {
            //show_matches(match);
            verse_range.start_chapter(match.str(1), match.str(2));
            return true;
        }
        //std::cout << "no match: " << line;
        return false;
    }

    bool check_verse_end(std::string const & line) {
        return (line == "SYNONYMS\n");
    }

    std::string balaram_font_to_unicode(std::string const & s) {
        std::string u;
        for (auto c: s) {
            switch (static_cast<unsigned char>(c)) {
                case 0x92: u += "'"; break;
                case 0x97: u += "—"; break;
                case 0xe0: u += "ṁ"; break;
                case 0xe4: u += "ā"; break;
                case 0xe5: u += "ṛ"; break;
                case 0xe7: u += "ś"; break;
                case 0xe8: u += "ṝ"; break;
                case 0xe9: u += "ī"; break;
                case 0xeb: u += "ṇ"; break;
                case 0xec: u += "ṅ"; break;
                case 0xef: u += "ñ"; break;
                case 0xf1: u += "ṣ"; break;
                case 0xf2: u += "ḍ"; break;
                case 0xf6: u += "ṭ"; break;
                case 0xf9: u += "ḥ"; break;
                case 0xfb: u += "ḻ"; break;
                case 0xfc: u += "ū"; break;
                case 0xff: u += "ḷ"; break;
                default: u += c;
            }
        }
        return u;
    }

    bool ends_with(std::string const & subject, std::string const & with) {
        auto subject_size = subject.size();
        auto with_size = with.size();
        if (subject_size < with_size) return false;
        auto start = subject_size - with_size;
        return (subject.compare(start, with_size, with) == 0);
    }

    // true if this is "... uvaaca" line
    bool uvaca(std::string const & line) {
        static std::vector<std::string> uvacas = {
            "ov\xe4" "ca", // for rajovaaca, brahmovaaca, etc.
            " uv\xe4" "ca", // for generic singular "xxx uvaaca"
            " \xfc" "cu\xf9", // for generic plural "xxx uucuH"
        };
        return std::any_of(uvacas.begin(), uvacas.end(),
            [&](std::string const & u) { return ends_with(line, u); });
        //return ends_with(s, uvacas[0]) || ends_with(s, uvacas[1]);
    }

    int syllables(std::string const & s) {
        int syllables_count = 0;
        auto size = s.size();
        for (unsigned i=0; i<size; ++i) {
            switch (static_cast<unsigned char>(s[i])) {
                // a is handled below because of ai and au
                case 0xe4: // aa
                case 'i':  // i
                case 0xe9: // ii
                case 'u':  // u
                case 0xfc: // uu
                case 0xe5: // R
                case 0xe8: // RR
                case 0xff: // L
                // missing in source encoding: LL
                case 'e':  // e
                case 'o':  // o
                    ++syllables_count;
                    break;
                case 'a':  // a
                    if (i+1 < size && (s[i+1] == 'i' || s[i+1] == 'u')) {
                        ++i;
                    }
                    ++syllables_count;
                    break;
                default:
                    break;
            }
        }
        return syllables_count;
    }

    void parse_verse_line(std::string const & line) {
        if (check_verse_end(line)) {
            verse_range.clear();
            std::cout << std::flush;
            return;
        }

        if (line == "TEXT\n") return;

        std::string our_line = line;
        // trim tailing newline for unification
        auto size = our_line.size();
        if (size >= 1 && our_line[size-1] == '\n') {
            our_line.resize(size-1);
        }

        auto syllables_count = syllables(our_line);
        total_syllables += syllables_count;

        bool is_uvaca = uvaca(our_line);
        if (!is_uvaca) {
            total_syllables_no_uvaca += syllables_count;
        }

        std::cout
            << verse_range << '(' << syllables_count << (is_uvaca ? "'" : "")
            << "): " << balaram_font_to_unicode(our_line) << '\n';
    }

    void parse_line(std::string const & line, CHP const & /*chp*/) {
        if (verse_range.empty()) {
            if (check_for_verse_start(line)) return;
            if (check_for_chapter_start(line)) return;
            return;
        }

        parse_verse_line(line);
    }

};

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

    std::cout << "total syllables: " << total_syllables << '\n';
    std::cout << "total syllables (no uvaaca): " << total_syllables_no_uvaca << '\n';
    fclose(f);
}
