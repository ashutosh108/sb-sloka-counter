#include <iostream>
#include <string>

#include "rtfparser.h"

class CoutOutputter {
public:
    void write(std::string const & string, CHP const & /*chp*/) {
        std::cout << string;
    }
};

// %%Function: main
//
// Main loop. Initialize and parse RTF.
int main()
{
    FILE *fp;

    fp = fopen("test.rtf", "r");
    if (!fp)
    {
        printf ("Can't open test file!\n");
        return 1;
    }

    Status ec;
    RtfParser<CoutOutputter> p;
    if ((ec = p.RtfParse(fp)) != Status::OK)
        printf("error %d parsing rtf\n", static_cast<int>(ec));
    else
        printf("Parsed RTF file OK\n");
    fclose(fp);
    return 0;
}
