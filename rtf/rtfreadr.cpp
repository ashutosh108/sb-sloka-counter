#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

enum class Status {
// RTF parser error codes
    OK              = 0,      // Everything's fine!
    StackUnderflow  = 1,      // Unmatched '}'
    StackOverflow   = 2,      // Too many '{' – memory exhausted
    UnmatchedBrace  = 3,      // RTF ended during an open group.
    InvalidHex      = 4,      // invalid hex character found in data
    BadTable        = 5,      // RTF table (sym or prop) not valid
    Assertion       = 6,      // Assertion failure
    EndOfFile       = 7,      // End of file reached while reading RTF
    InvalidKeyword  = 8,      // Invalid keyword
    InvalidParam    = 9       // Invalid parameter
};

class RtfParser {
public:
    // %%Function: RtfParse
    //
    // Step 1:
    // Isolate RTF keywords and send them to ParseRtfKeyword;
    // Push and pop state at the start and end of RTF groups;
    // Send text to ParseChar for further processing.
    Status RtfParse(FILE *fp);

private:
    typedef enum { rdsNorm, rdsSkip } RDS;              // Rtf Destination State
    // What types of properties are there?
    typedef enum {ipropBold, ipropItalic, ipropUnderline, ipropLeftInd,
                  ipropRightInd, ipropFirstInd, ipropCols, ipropPgnX,
                  ipropPgnY, ipropXaPage, ipropYaPage, ipropXaLeft,
                  ipropXaRight, ipropYaTop, ipropYaBottom, ipropPgnStart,
                  ipropSbk, ipropPgnFormat, ipropFacingp, ipropLandscape,
                  ipropJust, ipropPard, ipropPlain, ipropSectd,
                  ipropMax } IPROP;

    typedef enum {ipfnBin, ipfnHex, ipfnSkipDest } IPFN;
    typedef enum {idestPict, idestSkip } IDEST;

    typedef struct char_prop
    {
        char fBold;
        char fUnderline;
        char fItalic;
    } CHP;                  // Character Properties

    typedef enum {justL, justR, justC, justF } JUST;
    typedef struct para_prop
    {
        int xaLeft;                 // left indent in twips
        int xaRight;                // right indent in twips
        int xaFirst;                // first line indent in twips
        JUST just;                  // justification
    } PAP;                  // Paragraph Properties

    typedef enum {sbkNon, sbkCol, sbkEvn, sbkOdd, sbkPg} SBK;
    typedef enum {pgDec, pgURom, pgLRom, pgULtr, pgLLtr} PGN;
    typedef struct sect_prop
    {
        int cCols;                  // number of columns
        SBK sbk;                    // section break type
        int xaPgn;                  // x position of page number in twips
        int yaPgn;                  // y position of page number in twips
        PGN pgnFormat;              // how the page number is formatted
    } SEP;                  // Section Properties
    typedef struct doc_prop
    {
        int xaPage;                 // page width in twips
        int yaPage;                 // page height in twips
        int xaLeft;                 // left margin in twips
        int yaTop;                  // top margin in twips
        int xaRight;                // right margin in twips
        int yaBottom;               // bottom margin in twips
        int pgnStart;               // starting page number in twips
        char fFacingp;              // facing pages enabled?
        char fLandscape;            // landscape or portrait?
    } DOP;                  // Document Properties

    typedef enum { risNorm, risBin, risHex } RIS;       // Rtf Internal State

    typedef struct save             // property save structure
    {
        struct save *pNext;         // next save
        CHP chp;
        PAP pap;
        SEP sep;
        DOP dop;
        RDS rds;
        RIS ris;
    } SAVE;

    typedef enum {actnSpec, actnByte, actnWord} ACTN;
    typedef enum {propChp, propPap, propSep, propDop} PROPTYPE;

    typedef struct propmod
    {
        ACTN actn;              // size of value
        PROPTYPE prop;          // structure containing value
        int  offset;            // offset of value from base of structure
    } PROP;

    typedef enum {kwdChar, kwdDest, kwdProp, kwdSpec} KWD;
    typedef struct symbol
    {
        const char *szKeyword;  // RTF keyword
        int  dflt;              // default value to use
        bool fPassDflt;         // true to use default value from this table
        KWD  kwd;               // base action to take
        int  idx;               // index into property table if kwd == kwdProp
                                // index into destination table if kwd == kwdDest
                                // character to print if kwd == kwdChar
    } SYM;

    int cGroup=0;
    bool fSkipDestIfUnk=false;
    long cbBin=0;
    long lParam=0;
    RDS rds{};
    RIS ris{};

    CHP chp{};
    PAP pap{};
    SEP sep{};
    DOP dop{};
    SAVE *psave{};

    // RTF parser tables
    // Property descriptions
    static const RtfParser::PROP rgprop [RtfParser::ipropMax];

    // Keyword descriptions
    static const RtfParser::SYM rgsymRtf[];
    static std::size_t isymMax;

    Status PushRtfState(void);
    Status PopRtfState(void);
    Status ParseRtfKeyword(FILE *fp);
    Status ParseChar(int c);
    Status TranslateKeyword(char *szKeyword, int param, bool fParam);
    Status PrintChar(int ch);
    Status EndGroupAction(RDS rds);
    Status ApplyPropChange(IPROP iprop, int val);
    Status ChangeDest(IDEST idest);
    Status ParseSpecialKeyword(IPFN ipfn);
    Status ParseSpecialProperty(IPROP iprop, int val);
    Status ParseHexByte(void);
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
    RtfParser p;
    if ((ec = p.RtfParse(fp)) != Status::OK)
        printf("error %d parsing rtf\n", static_cast<int>(ec));
    else
        printf("Parsed RTF file OK\n");
    fclose(fp);
    return 0;
}

Status RtfParser::RtfParse(FILE *fp)
{
    int ch;
    Status ec;
    int cNibble = 2;
    int b = 0;
    while ((ch = getc(fp)) != EOF)
    {
        if (cGroup < 0)
            return Status::StackUnderflow;
        if (ris == risBin)                      // if we’re parsing binary data, handle it directly
        {
            if ((ec = ParseChar(ch)) != Status::OK)
                return ec;
        }
        else
        {
            switch (ch)
            {
            case '{':
                if ((ec = PushRtfState()) != Status::OK)
                    return ec;
                break;
            case '}':
                if ((ec = PopRtfState()) != Status::OK)
                    return ec;
                break;
            case '\\':
                if ((ec = ParseRtfKeyword(fp)) != Status::OK)
                    return ec;
                break;
            case 0x0d:
            case 0x0a:          // cr and lf are noise characters...
                break;
            default:
                if (ris == risNorm)
                {
                    if ((ec = ParseChar(ch)) != Status::OK)
                        return ec;
                }
                else
                {               // parsing hex data
                    if (ris != risHex)
                        return Status::Assertion;
                    b = b << 4;
                    if (isdigit(ch))
                        b += static_cast<char>(ch) - '0';
                    else
                    {
                        if (islower(ch))
                        {
                            if (ch < 'a' || ch > 'f')
                                return Status::InvalidHex;
                            b += static_cast<char>(ch) - 'a' + 10;
                        }
                        else
                        {
                            if (ch < 'A' || ch > 'F')
                                return Status::InvalidHex;
                            b += static_cast<char>(ch) - 'A' + 10;
                        }
                    }
                    cNibble--;
                    if (!cNibble)
                    {
                        if ((ec = ParseChar(b)) != Status::OK)
                            return ec;
                        cNibble = 2;
                        b = 0;
                        ris = risNorm;
                    }
                }                   // end else (ris != risNorm)
                break;
            }       // switch
        }           // else (ris != risBin)
    }               // while
    if (cGroup < 0)
        return Status::StackUnderflow;
    if (cGroup > 0)
        return Status::UnmatchedBrace;
    return Status::OK;
}

// %%Function: PushRtfState
//
// Save relevant info on a linked list of SAVE structures.

Status RtfParser::PushRtfState(void)
{
    SAVE *psaveNew = static_cast<SAVE *>(malloc(sizeof(SAVE)));
    if (!psaveNew)
        return Status::StackOverflow;

    psaveNew -> pNext = psave;
    psaveNew -> chp = chp;
    psaveNew -> pap = pap;
    psaveNew -> sep = sep;
    psaveNew -> dop = dop;
    psaveNew -> rds = rds;
    psaveNew -> ris = ris;
    ris = risNorm;
    psave = psaveNew;
    cGroup++;
    return Status::OK;
}

// %%Function: PopRtfState
//
// If we're ending a destination (that is, the destination is changing),
// call EndGroupAction.
// Always restore relevant info from the top of the SAVE list.

Status RtfParser::PopRtfState(void)
{
    SAVE *psaveOld;
    Status ec;

    if (!psave)
        return Status::StackUnderflow;

    if (rds != psave->rds)
    {
        if ((ec = EndGroupAction(rds)) != Status::OK)
            return ec;
    }
    chp = psave->chp;
    pap = psave->pap;
    sep = psave->sep;
    dop = psave->dop;
    rds = psave->rds;
    ris = psave->ris;

    psaveOld = psave;
    psave = psave->pNext;
    cGroup--;
    free(psaveOld);
    return Status::OK;
}

// %%Function: ParseRtfKeyword
//
// Step 2:
// get a control word (and its associated value) and
// call TranslateKeyword to dispatch the control.

Status RtfParser::ParseRtfKeyword(FILE *fp)
{
    int ch;
    char fParam = false;
    char fNeg = false;
    int param = 0;
    char *pch;
    char szKeyword[30];
    char *pKeywordMax = &szKeyword[30];
    char szParameter[20];
    char *pParamMax = &szParameter[20];

    lParam = 0;
    szKeyword[0] = '\0';
    szParameter[0] = '\0';
    if ((ch = getc(fp)) == EOF)
        return Status::EndOfFile;
    if (!isalpha(ch))           // a control symbol; no delimiter.
    {
        szKeyword[0] = static_cast<char>(ch);
        szKeyword[1] = '\0';
        return TranslateKeyword(szKeyword, 0, fParam);
    }
    for (pch = szKeyword; pch < pKeywordMax && isalpha(ch); ch = getc(fp))
        *pch++ = static_cast<char>(ch);
    if (pch >= pKeywordMax)
        return Status::InvalidKeyword;	// Keyword too long
    *pch = '\0';
    if (ch == '-')
    {
        fNeg  = true;
        if ((ch = getc(fp)) == EOF)
            return Status::EndOfFile;
    }
    if (isdigit(ch))
    {
        fParam = true;         // a digit after the control means we have a parameter
        for (pch = szParameter; pch < pParamMax && isdigit(ch); ch = getc(fp))
            *pch++ = static_cast<char>(ch);
        if (pch >= pParamMax)
            return Status::InvalidParam;	// Parameter too long
        *pch = '\0';
        param = atoi(szParameter);
        if (fNeg)
            param = -param;
        lParam = param;
    }
    if (ch != ' ')
        ungetc(ch, fp);
    return TranslateKeyword(szKeyword, param, fParam);
}

// %%Function: ParseChar
//
// Route the character to the appropriate destination stream.

Status RtfParser::ParseChar(int ch)
{
    if (ris == risBin && --cbBin <= 0)
        ris = risNorm;
    switch (rds)
    {
    case rdsSkip:
        // Toss this character.
        return Status::OK;
    case rdsNorm:
        // Output a character. Properties are valid at this point.
        return PrintChar(ch);
    default:
    // handle other destinations....
        return Status::OK;
    }
}

//
// %%Function: PrintChar
//
// Send a character to the output file.

Status RtfParser::PrintChar(int ch)
{
    // unfortunately, we do not do a whole lot here as far as layout goes...
    putchar(ch);
    return Status::OK;
}

const RtfParser::PROP RtfParser::rgprop [RtfParser::ipropMax] = {
    actnByte,   propChp,    offsetof(CHP, fBold),       // ipropBold
    actnByte,   propChp,    offsetof(CHP, fItalic),     // ipropItalic
    actnByte,   propChp,    offsetof(CHP, fUnderline),  // ipropUnderline
    actnWord,   propPap,    offsetof(PAP, xaLeft),      // ipropLeftInd
    actnWord,   propPap,    offsetof(PAP, xaRight),     // ipropRightInd
    actnWord,   propPap,    offsetof(PAP, xaFirst),     // ipropFirstInd
    actnWord,   propSep,    offsetof(SEP, cCols),       // ipropCols
    actnWord,   propSep,    offsetof(SEP, xaPgn),       // ipropPgnX
    actnWord,   propSep,    offsetof(SEP, yaPgn),       // ipropPgnY
    actnWord,   propDop,    offsetof(DOP, xaPage),      // ipropXaPage
    actnWord,   propDop,    offsetof(DOP, yaPage),      // ipropYaPage
    actnWord,   propDop,    offsetof(DOP, xaLeft),      // ipropXaLeft
    actnWord,   propDop,    offsetof(DOP, xaRight),     // ipropXaRight
    actnWord,   propDop,    offsetof(DOP, yaTop),       // ipropYaTop
    actnWord,   propDop,    offsetof(DOP, yaBottom),    // ipropYaBottom
    actnWord,   propDop,    offsetof(DOP, pgnStart),    // ipropPgnStart
    actnByte,   propSep,    offsetof(SEP, sbk),         // ipropSbk
    actnByte,   propSep,    offsetof(SEP, pgnFormat),   // ipropPgnFormat
    actnByte,   propDop,    offsetof(DOP, fFacingp),    // ipropFacingp
    actnByte,   propDop,    offsetof(DOP, fLandscape),  // ipropLandscape
    actnByte,   propPap,    offsetof(PAP, just),        // ipropJust
    actnSpec,   propPap,    0,                          // ipropPard
    actnSpec,   propChp,    0,                          // ipropPlain
    actnSpec,   propSep,    0,                          // ipropSectd
};

const RtfParser::SYM RtfParser::rgsymRtf[] = {
//  keyword     dflt    fPassDflt  kwd         idx
    "b",        1,      false,     kwdProp,    ipropBold,
    "u",        1,      false,     kwdProp,    ipropUnderline,
    "i",        1,      false,     kwdProp,    ipropItalic,
    "li",       0,      false,     kwdProp,    ipropLeftInd,
    "ri",       0,      false,     kwdProp,    ipropRightInd,
    "fi",       0,      false,     kwdProp,    ipropFirstInd,
    "cols",     1,      false,     kwdProp,    ipropCols,
    "sbknone",  sbkNon, true,      kwdProp,    ipropSbk,
    "sbkcol",   sbkCol, true,      kwdProp,    ipropSbk,
    "sbkeven",  sbkEvn, true,      kwdProp,    ipropSbk,
    "sbkodd",   sbkOdd, true,      kwdProp,    ipropSbk,
    "sbkpage",  sbkPg,  true,      kwdProp,    ipropSbk,
    "pgnx",     0,      false,     kwdProp,    ipropPgnX,
    "pgny",     0,      false,     kwdProp,    ipropPgnY,
    "pgndec",   pgDec,  true,      kwdProp,    ipropPgnFormat,
    "pgnucrm",  pgURom, true,      kwdProp,    ipropPgnFormat,
    "pgnlcrm",  pgLRom, true,      kwdProp,    ipropPgnFormat,
    "pgnucltr", pgULtr, true,      kwdProp,    ipropPgnFormat,
    "pgnlcltr", pgLLtr, true,      kwdProp,    ipropPgnFormat,
    "qc",       justC,  true,      kwdProp,    ipropJust,
    "ql",       justL,  true,      kwdProp,    ipropJust,
    "qr",       justR,  true,      kwdProp,    ipropJust,
    "qj",       justF,  true,      kwdProp,    ipropJust,
    "paperw",   12240,  false,     kwdProp,    ipropXaPage,
    "paperh",   15480,  false,     kwdProp,    ipropYaPage,
    "margl",    1800,   false,     kwdProp,    ipropXaLeft,
    "margr",    1800,   false,     kwdProp,    ipropXaRight,
    "margt",    1440,   false,     kwdProp,    ipropYaTop,
    "margb",    1440,   false,     kwdProp,    ipropYaBottom,
    "pgnstart", 1,      true,      kwdProp,    ipropPgnStart,
    "facingp",  1,      true,      kwdProp,    ipropFacingp,
    "landscape",1,      true,      kwdProp,    ipropLandscape,
    "par",      0,      false,     kwdChar,    0x0a,
    "\0x0a",    0,      false,     kwdChar,    0x0a,
    "\0x0d",    0,      false,     kwdChar,    0x0a,
    "tab",      0,      false,     kwdChar,    0x09,
    "ldblquote",0,      false,     kwdChar,    0x201c,
    "rdblquote",0,      false,     kwdChar,    0x201d,
    "bin",      0,      false,     kwdSpec,    ipfnBin,
    "*",        0,      false,     kwdSpec,    ipfnSkipDest,
    "'",        0,      false,     kwdSpec,    ipfnHex,
    "author",   0,      false,     kwdDest,    idestSkip,
    "buptim",   0,      false,     kwdDest,    idestSkip,
    "colortbl", 0,      false,     kwdDest,    idestSkip,
    "comment",  0,      false,     kwdDest,    idestSkip,
    "creatim",  0,      false,     kwdDest,    idestSkip,
    "doccomm",  0,      false,     kwdDest,    idestSkip,
    "fonttbl",  0,      false,     kwdDest,    idestSkip,
    "footer",   0,      false,     kwdDest,    idestSkip,
    "footerf",  0,      false,     kwdDest,    idestSkip,
    "footerl",  0,      false,     kwdDest,    idestSkip,
    "footerr",  0,      false,     kwdDest,    idestSkip,
    "footnote", 0,      false,     kwdDest,    idestSkip,
    "ftncn",    0,      false,     kwdDest,    idestSkip,
    "ftnsep",   0,      false,     kwdDest,    idestSkip,
    "ftnsepc",  0,      false,     kwdDest,    idestSkip,
    "header",   0,      false,     kwdDest,    idestSkip,
    "headerf",  0,      false,     kwdDest,    idestSkip,
    "headerl",  0,      false,     kwdDest,    idestSkip,
    "headerr",  0,      false,     kwdDest,    idestSkip,
    "info",     0,      false,     kwdDest,    idestSkip,
    "keywords", 0,      false,     kwdDest,    idestSkip,
    "operator", 0,      false,     kwdDest,    idestSkip,
    "pict",     0,      false,     kwdDest,    idestSkip,
    "printim",  0,      false,     kwdDest,    idestSkip,
    "private1", 0,      false,     kwdDest,    idestSkip,
    "revtim",   0,      false,     kwdDest,    idestSkip,
    "rxe",      0,      false,     kwdDest,    idestSkip,
    "stylesheet",0,     false,     kwdDest,    idestSkip,
    "subject",  0,      false,     kwdDest,    idestSkip,
    "tc",       0,      false,     kwdDest,    idestSkip,
    "title",    0,      false,     kwdDest,    idestSkip,
    "txe",      0,      false,     kwdDest,    idestSkip,
    "xe",       0,      false,     kwdDest,    idestSkip,
    "{",        0,      false,     kwdChar,    '{',
    "}",        0,      false,     kwdChar,    '}',
    "\\",       0,      false,     kwdChar,    '\\'
};

std::size_t RtfParser::isymMax = sizeof(rgsymRtf) / sizeof(RtfParser::SYM);

// %%Function: ApplyPropChange
// Set the property identified by _iprop_ to the value _val_.

Status RtfParser::ApplyPropChange(IPROP iprop, int val)
{
    unsigned char *pb = nullptr;
    if (rds == rdsSkip)                 // If we're skipping text,
        return Status::OK;              // Do not do anything.

    switch (rgprop[iprop].prop)
    {
    case propDop:
        pb = reinterpret_cast<unsigned char *>(&dop);
        break;
    case propSep:
        pb = reinterpret_cast<unsigned char *>(&sep);
        break;
    case propPap:
        pb = reinterpret_cast<unsigned char *>(&pap);
        break;
    case propChp:
        pb = reinterpret_cast<unsigned char *>(&chp);
        break;
    default:
        if (rgprop[iprop].actn != actnSpec)
            return Status::BadTable;
        break;
    }
    switch (rgprop[iprop].actn)
    {
    case actnByte:
        pb[rgprop[iprop].offset] = static_cast<unsigned char>(val);
        break;
    case actnWord:
        assert(pb != nullptr);
        (*reinterpret_cast<int *>(pb+rgprop[iprop].offset)) = val;
        break;
    case actnSpec:
        return ParseSpecialProperty(iprop, val);
        break;
    default:
        return Status::BadTable;
    }
    return Status::OK;
}

// %%Function: ParseSpecialProperty
// Set a property that requires code to evaluate.

Status RtfParser::ParseSpecialProperty(IPROP iprop, int/* val*/)
{
    switch (iprop)
    {
    case ipropPard:
        memset(&pap, 0, sizeof(pap));
        return Status::OK;
    case ipropPlain:
        memset(&chp, 0, sizeof(chp));
        return Status::OK;
    case ipropSectd:
        memset(&sep, 0, sizeof(sep));
        return Status::OK;
    default:
        return Status::BadTable;
    }
}

// %%Function: TranslateKeyword
// Step 3.
// Search rgsymRtf for szKeyword and evaluate it appropriately.
// Inputs:
// szKeyword:   The RTF control to evaluate.
// param:       The parameter of the RTF control.
// fParam:      true if the control had a parameter; (that is, if param is valid)
//              false if it did not.

Status RtfParser::TranslateKeyword(char *szKeyword, int param, bool fParam)
{
    std::size_t isym;

    // search for szKeyword in rgsymRtf
    for (isym = 0; isym < isymMax; isym++)
        if (strcmp(szKeyword, rgsymRtf[isym].szKeyword) == 0)
            break;
    if (isym == isymMax)            // control word not found
    {
        if (fSkipDestIfUnk)         // if this is a new destination
            rds = rdsSkip;          // skip the destination
                                    // else just discard it
        fSkipDestIfUnk = false;
        return Status::OK;
    }

    // found it!  Use kwd and idx to determine what to do with it.
    fSkipDestIfUnk = false;
    switch (rgsymRtf[isym].kwd)
    {
    case kwdProp:
        if (rgsymRtf[isym].fPassDflt || !fParam)
            param = rgsymRtf[isym].dflt;
        return ApplyPropChange(static_cast<IPROP>(rgsymRtf[isym].idx), param);
    case kwdChar:
        return ParseChar(rgsymRtf[isym].idx);
    case kwdDest:
        return ChangeDest(static_cast<IDEST>(rgsymRtf[isym].idx));
    case kwdSpec:
        return ParseSpecialKeyword(static_cast<IPFN>(rgsymRtf[isym].idx));
    default:
        return Status::BadTable;
    }
}

// %%Function: ChangeDest
// Change to the destination specified by idest.
// There's usually more to do here than this...

Status RtfParser::ChangeDest(IDEST/* idest*/)
{
    if (rds == rdsSkip)             // if we're skipping text,
        return Status::OK;                // Do not do anything

    rds = rdsSkip;              // when in doubt, skip it...
    return Status::OK;
}

// %%Function: EndGroupAction
// The destination specified by rds is coming to a close.
// If there’s any cleanup that needs to be done, do it now.

Status RtfParser::EndGroupAction(RDS/* rds*/)
{
    return Status::OK;
}

// %%Function: ParseSpecialKeyword
// Evaluate an RTF control that needs special processing.

Status RtfParser::ParseSpecialKeyword(IPFN ipfn)
{
    if (rds == rdsSkip && ipfn != ipfnBin)  // if we're skipping, and it is not
        return Status::OK;                          // the \bin keyword, ignore it.
    switch (ipfn)
    {
    case ipfnBin:
        ris = risBin;
        cbBin = lParam;
        break;
    case ipfnSkipDest:
        fSkipDestIfUnk = true;
        break;
    case ipfnHex:
        ris = risHex;
        break;
    default:
        return Status::BadTable;
    }
    return Status::OK;
}
