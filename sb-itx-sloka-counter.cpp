#include <fstream>
#include <iostream>
#include <map>
#include <regex>

class SlokaCounter {
public:
    void do_counting(std::istream & f) {
        std::string line;
        while (std::getline(f, line)) {
            process_line(line);
        }

        print_total_by_chapter();

        std::cout << "total syllables: " << total_syllables << '\n';
        std::cout << "total syllables (no uvaaca): " << total_syllables_no_uvaca << '\n';
    }

private:
    std::string itx_to_unicode(std::string const &s) {
        std::string u;
        auto size = s.size();
        for (decltype(size) i=0; i < size; ++i) {
            switch (static_cast<unsigned char>(s[i])) {
                case 'M': u += "ṁ"; break;
                case 'A': u += "ā"; break;
                case 'R':
                    if (i+2 < size && s[i+1] == '^' && s[i+2] == 'i') {
                        i += 2;
                        u += "ṛ";
                    } else if (i+2 < size && s[i+1] == '^' && s[i+2] == 'I') {
                        i += 2;
                        u += "ṝ";
                    }
                    break;
                case 'L':
                    if (i+2 < size && s[i+1] == '^' && s[i+2] == 'i') {
                        i += 2;
                        u += "ḷ";
                    }
                    break;
                case 'S':
                    if (i+1 < size && s[i+1] == 'h') {
                        i += 1;
                        u += "ś";
                    }
                    break;
                case 'I': u += "ī"; break;
                case 'N': u += "ṇ"; break;
                case '~':
                    if (i+1 < size && s[i+1] == 'n') {
                        i += 1;
                        u += "ñ";
                    } else if (i+1 < size && s[i+1] == 'N') {
                        i += 1;
                        u += "ṅ";
                    }
                    break;
                case 's':
                    if (i+1 < size && s[i+1] == 'h') {
                        i += 1;
                        u += "ṣ";
                    } else {
                        u += 's';
                    }
                    break;
                case 'D': u += "ḍ"; break;
                case 'T': u += "ṭ"; break;
                case 'H': u += "ḥ"; break;
                case 'U': u += "ū"; break;
                case '.':
                    if (i+1 < size && s[i+1] == 'a') {
                        i += 1;
                        u += " '";
                    }
                    break;
                case 'c':
                    if (i+1 < size && s[i+1] == 'h') {
                        i += 1;
                        u += 'c';
                    }
                    break;
                case 'C':
                    if (i+1 < size && s[i+1] == 'h') {
                        i += 1;
                        u += "ch";
                    }
                    break;
                default: u += s[i];
            }
        }
        return u;
    }

    int syllables(std::string const & s) {
        int syllables_count = 0;
        auto size = s.size();
        for (unsigned i=0; i<size; ++i) {
            switch (static_cast<unsigned char>(s[i])) {
                // a is handled below because of ai and au
                case 'A': // aa
                case 'i':  // i
                case 'I': // ii
                case 'u':  // u
                case 'U': // uu
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
                case 'R': // R, RR
                case 'L': // L, (theoretically) LL
                    if (i+1 < size && (s[i+1] == 'i' || s[i+1] == 'I')) {
                        i += 2;
                        ++syllables_count;
                    }
                    break;
                case '.':
                    // '.a' is avagraha
                    if (i+1 < size && s[i+1] == 'a') {
                        i += 1;
                    }
                    break;
                default:
                    break;
            }
        }
        return syllables_count;
    }

    bool ends_with(std::string const & subject, std::string const & with) {
        auto subject_size = subject.size();
        auto with_size = with.size();
        if (subject_size < with_size) return false;
        auto start = subject_size - with_size;
        return (subject.compare(start, with_size, with) == 0);
    }

    void print_total_by_chapter() {
        for (auto & pair: total_by_chapter) {
            std::cout << "chapter " << pair.first << ": " << pair.second << '\n';
        }
    }

    // true if this is "... uvaaca" line
    bool uvaca(std::string const & line) {
        static std::vector<std::string> uvacas = {
            "ovAcha", // for rajovaaca, brahmovaaca, etc.
            "uvAcha", // for generic singular "xxx uvaaca"
            "UchuH", // for generic plural "xxx uucuH"
        };
        return std::any_of(uvacas.begin(), uvacas.end(),
            [&](std::string const & u) { return ends_with(line, u); });
    }

    void process_line(std::string & line) {
        std::smatch match;
        static std::regex r(R"RE((\d\d)(\d\d)(\d\d\d)(\d) (.*?)(?: *#|$))RE");
        std::string & text = line;
        if (std::regex_search(line, match, r)) {
            canto = std::stoi(match.str(1));
            chapter = std::stoi(match.str(2));
            text_num = std::stoi(match.str(3));
            line_num = std::stoi(match.str(4));
            text = match.str(5);
        } else {
            if (canto == 12 && chapter == 13 && text_num == 23 && line.size() >= 1 && line[0] == ' ') {
                // skip the rest of the lines, they are not part of Bhagavatam per se
                canto = 0;
            }
        }

        if (canto == 0) return; // it means current line is not part of Bhagavatam

        auto syllables_count = syllables(text);
        total_syllables += syllables_count;

        char canto_chapter[20];
        snprintf(canto_chapter, 20, "%02d.%02d", canto, chapter);
        char canto_dot_x[20];
        snprintf(canto_dot_x, 20, "%02d.x", canto);
        total_by_chapter[canto_chapter] += syllables_count;
        total_by_chapter[canto_dot_x] += syllables_count;

        bool is_uvaca = (line_num == 0); // uvaca(text);
        if (!is_uvaca) {
            total_syllables_no_uvaca += syllables_count;
        }
        if (is_uvaca != (line_num == 0)) {
            std::cerr << "mismatch of uvaca: is_uvaca=" << is_uvaca << ", line_num=" << line_num << '\n';
            std::exit(1);
        }

        std::cout << canto << '.' << chapter << '.' << text_num
            << "(" << syllables_count << (is_uvaca ? "'" : "") << "): "
            << itx_to_unicode(text) << '\n';
    }

    int total_syllables = 0;
    int total_syllables_no_uvaca = 0;
    int canto = 0;
    int chapter = 0;
    int text_num = 0;
    int line_num = 0;
    std::map<std::string, int> total_by_chapter;
};

int main() {
    std::ifstream f("bhagpur.itx");
    if (!f) {
        std::cerr << "can't open bhagpur.itx";
        return 1;
    }

    SlokaCounter c;
    c.do_counting(f);
}
