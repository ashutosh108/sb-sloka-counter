#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define fTrue 1
#define fFalse 0

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

typedef enum { rdsNorm, rdsSkip } RDS;              // Rtf Destination State
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

// What types of properties are there?
typedef enum {ipropBold, ipropItalic, ipropUnderline, ipropLeftInd,
              ipropRightInd, ipropFirstInd, ipropCols, ipropPgnX,
              ipropPgnY, ipropXaPage, ipropYaPage, ipropXaLeft,
              ipropXaRight, ipropYaTop, ipropYaBottom, ipropPgnStart,
              ipropSbk, ipropPgnFormat, ipropFacingp, ipropLandscape,
              ipropJust, ipropPard, ipropPlain, ipropSectd,
              ipropMax } IPROP;

typedef enum {actnSpec, actnByte, actnWord} ACTN;
typedef enum {propChp, propPap, propSep, propDop} PROPTYPE;

typedef struct propmod
{
    ACTN actn;              // size of value
    PROPTYPE prop;          // structure containing value
    int  offset;            // offset of value from base of structure
} PROP;

typedef enum {ipfnBin, ipfnHex, ipfnSkipDest } IPFN;
typedef enum {idestPict, idestSkip } IDEST;
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

// RTF parser declarations
int ecRtfParse(FILE *fp);
int ecPushRtfState(void);
int ecPopRtfState(void);
int ecParseRtfKeyword(FILE *fp);
int ecParseChar(int c);
int ecTranslateKeyword(char *szKeyword, int param, bool fParam);
int ecPrintChar(int ch);
int ecEndGroupAction(RDS rds);
int ecApplyPropChange(IPROP iprop, int val);
int ecChangeDest(IDEST idest);
int ecParseSpecialKeyword(IPFN ipfn);
int ecParseSpecialProperty(IPROP iprop, int val);
int ecParseHexByte(void);

// RTF parser error codes
#define ecOK 0                      // Everything's fine!
#define ecStackUnderflow    1       // Unmatched '}'
#define ecStackOverflow     2       // Too many '{' – memory exhausted
#define ecUnmatchedBrace    3       // RTF ended during an open group.
#define ecInvalidHex        4       // invalid hex character found in data
#define ecBadTable          5       // RTF table (sym or prop) not valid
#define ecAssertion         6       // Assertion failure
#define ecEndOfFile         7       // End of file reached while reading RTF
#define ecInvalidKeyword    8       // Invalid keyword
#define ecInvalidParam      9       // Invalid parameter

int cGroup;
bool fSkipDestIfUnk;
long cbBin;
long lParam;
RDS rds;
RIS ris;

CHP chp;
PAP pap;
SEP sep;
DOP dop;
SAVE *psave;
FILE *fpIn;

// %%Function: main
//
// Main loop. Initialize and parse RTF.
int main()
{
    FILE *fp;
    int ec;

    fp = fpIn = fopen("test.rtf", "r");
    if (!fp)
    {
        printf ("Can't open test file!\n");
        return 1;
    }
    if ((ec = ecRtfParse(fp)) != ecOK)
        printf("error %d parsing rtf\n", ec);
    else
        printf("Parsed RTF file OK\n");
    fclose(fp);
    return 0;
}

// %%Function: ecRtfParse
//
// Step 1:
// Isolate RTF keywords and send them to ecParseRtfKeyword;
// Push and pop state at the start and end of RTF groups;
// Send text to ecParseChar for further processing.

int ecRtfParse(FILE *fp)
{
    int ch;
    int ec;
    int cNibble = 2;
    int b = 0;
    while ((ch = getc(fp)) != EOF)
    {
        if (cGroup < 0)
            return ecStackUnderflow;
        if (ris == risBin)                      // if we’re parsing binary data, handle it directly
        {
            if ((ec = ecParseChar(ch)) != ecOK)
                return ec;
        }
        else
        {
            switch (ch)
            {
            case '{':
                if ((ec = ecPushRtfState()) != ecOK)
                    return ec;
                break;
            case '}':
                if ((ec = ecPopRtfState()) != ecOK)
                    return ec;
                break;
            case '\\':
                if ((ec = ecParseRtfKeyword(fp)) != ecOK)
                    return ec;
                break;
            case 0x0d:
            case 0x0a:          // cr and lf are noise characters...
                break;
            default:
                if (ris == risNorm)
                {
                    if ((ec = ecParseChar(ch)) != ecOK)
                        return ec;
                }
                else
                {               // parsing hex data
                    if (ris != risHex)
                        return ecAssertion;
                    b = b << 4;
                    if (isdigit(ch))
                        b += static_cast<char>(ch) - '0';
                    else
                    {
                        if (islower(ch))
                        {
                            if (ch < 'a' || ch > 'f')
                                return ecInvalidHex;
                            b += static_cast<char>(ch) - 'a' + 10;
                        }
                        else
                        {
                            if (ch < 'A' || ch > 'F')
                                return ecInvalidHex;
                            b += static_cast<char>(ch) - 'A' + 10;
                        }
                    }
                    cNibble--;
                    if (!cNibble)
                    {
                        if ((ec = ecParseChar(b)) != ecOK)
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
        return ecStackUnderflow;
    if (cGroup > 0)
        return ecUnmatchedBrace;
    return ecOK;
}

// %%Function: ecPushRtfState
//
// Save relevant info on a linked list of SAVE structures.

int ecPushRtfState(void)
{
    SAVE *psaveNew = static_cast<SAVE *>(malloc(sizeof(SAVE)));
    if (!psaveNew)
        return ecStackOverflow;

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
    return ecOK;
}

// %%Function: ecPopRtfState
//
// If we're ending a destination (that is, the destination is changing),
// call ecEndGroupAction.
// Always restore relevant info from the top of the SAVE list.

int ecPopRtfState(void)
{
    SAVE *psaveOld;
    int ec;

    if (!psave)
        return ecStackUnderflow;

    if (rds != psave->rds)
    {
        if ((ec = ecEndGroupAction(rds)) != ecOK)
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
    return ecOK;
}

// %%Function: ecParseRtfKeyword
//
// Step 2:
// get a control word (and its associated value) and
// call ecTranslateKeyword to dispatch the control.

int ecParseRtfKeyword(FILE *fp)
{
    int ch;
    char fParam = fFalse;
    char fNeg = fFalse;
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
        return ecEndOfFile;
    if (!isalpha(ch))           // a control symbol; no delimiter.
    {
        szKeyword[0] = static_cast<char>(ch);
        szKeyword[1] = '\0';
        return ecTranslateKeyword(szKeyword, 0, fParam);
    }
    for (pch = szKeyword; pch < pKeywordMax && isalpha(ch); ch = getc(fp))
        *pch++ = static_cast<char>(ch);
    if (pch >= pKeywordMax)
        return ecInvalidKeyword;	// Keyword too long
    *pch = '\0';
    if (ch == '-')
    {
        fNeg  = fTrue;
        if ((ch = getc(fp)) == EOF)
            return ecEndOfFile;
    }
    if (isdigit(ch))
    {
        fParam = fTrue;         // a digit after the control means we have a parameter
        for (pch = szParameter; pch < pParamMax && isdigit(ch); ch = getc(fp))
            *pch++ = static_cast<char>(ch);
        if (pch >= pParamMax)
            return ecInvalidParam;	// Parameter too long
        *pch = '\0';
        param = atoi(szParameter);
        if (fNeg)
            param = -param;
        lParam = param;
    }
    if (ch != ' ')
        ungetc(ch, fp);
    return ecTranslateKeyword(szKeyword, param, fParam);
}

// %%Function: ecParseChar
//
// Route the character to the appropriate destination stream.

int ecParseChar(int ch)
{
    if (ris == risBin && --cbBin <= 0)
        ris = risNorm;
    switch (rds)
    {
    case rdsSkip:
        // Toss this character.
        return ecOK;
    case rdsNorm:
        // Output a character. Properties are valid at this point.
        return ecPrintChar(ch);
    default:
    // handle other destinations....
        return ecOK;
    }
}
//
// %%Function: ecPrintChar
//
// Send a character to the output file.

int ecPrintChar(int ch)
{
    // unfortunately, we do not do a whole lot here as far as layout goes...
    putchar(ch);
    return ecOK;
}

// RTF parser tables
// Property descriptions
PROP rgprop [ipropMax] = {
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

// Keyword descriptions
SYM rgsymRtf[] = {
//  keyword     dflt    fPassDflt   kwd         idx
    "b",        1,      fFalse,     kwdProp,    ipropBold,
    "u",        1,      fFalse,     kwdProp,    ipropUnderline,
    "i",        1,      fFalse,     kwdProp,    ipropItalic,
    "li",       0,      fFalse,     kwdProp,    ipropLeftInd,
    "ri",       0,      fFalse,     kwdProp,    ipropRightInd,
    "fi",       0,      fFalse,     kwdProp,    ipropFirstInd,
    "cols",     1,      fFalse,     kwdProp,    ipropCols,
    "sbknone",  sbkNon, fTrue,      kwdProp,    ipropSbk,
    "sbkcol",   sbkCol, fTrue,      kwdProp,    ipropSbk,
    "sbkeven",  sbkEvn, fTrue,      kwdProp,    ipropSbk,
    "sbkodd",   sbkOdd, fTrue,      kwdProp,    ipropSbk,
    "sbkpage",  sbkPg,  fTrue,      kwdProp,    ipropSbk,
    "pgnx",     0,      fFalse,     kwdProp,    ipropPgnX,
    "pgny",     0,      fFalse,     kwdProp,    ipropPgnY,
    "pgndec",   pgDec,  fTrue,      kwdProp,    ipropPgnFormat,
    "pgnucrm",  pgURom, fTrue,      kwdProp,    ipropPgnFormat,
    "pgnlcrm",  pgLRom, fTrue,      kwdProp,    ipropPgnFormat,
    "pgnucltr", pgULtr, fTrue,      kwdProp,    ipropPgnFormat,
    "pgnlcltr", pgLLtr, fTrue,      kwdProp,    ipropPgnFormat,
    "qc",       justC,  fTrue,      kwdProp,    ipropJust,
    "ql",       justL,  fTrue,      kwdProp,    ipropJust,
    "qr",       justR,  fTrue,      kwdProp,    ipropJust,
    "qj",       justF,  fTrue,      kwdProp,    ipropJust,
    "paperw",   12240,  fFalse,     kwdProp,    ipropXaPage,
    "paperh",   15480,  fFalse,     kwdProp,    ipropYaPage,
    "margl",    1800,   fFalse,     kwdProp,    ipropXaLeft,
    "margr",    1800,   fFalse,     kwdProp,    ipropXaRight,
    "margt",    1440,   fFalse,     kwdProp,    ipropYaTop,
    "margb",    1440,   fFalse,     kwdProp,    ipropYaBottom,
    "pgnstart", 1,      fTrue,      kwdProp,    ipropPgnStart,
    "facingp",  1,      fTrue,      kwdProp,    ipropFacingp,
    "landscape",1,      fTrue,      kwdProp,    ipropLandscape,
    "par",      0,      fFalse,     kwdChar,    0x0a,
    "\0x0a",    0,      fFalse,     kwdChar,    0x0a,
    "\0x0d",    0,      fFalse,     kwdChar,    0x0a,
    "tab",      0,      fFalse,     kwdChar,    0x09,
    "ldblquote",0,      fFalse,     kwdChar,    0x201c,
    "rdblquote",0,      fFalse,     kwdChar,    0x201d,
    "bin",      0,      fFalse,     kwdSpec,    ipfnBin,
    "*",        0,      fFalse,     kwdSpec,    ipfnSkipDest,
    "'",        0,      fFalse,     kwdSpec,    ipfnHex,
    "author",   0,      fFalse,     kwdDest,    idestSkip,
    "buptim",   0,      fFalse,     kwdDest,    idestSkip,
    "colortbl", 0,      fFalse,     kwdDest,    idestSkip,
    "comment",  0,      fFalse,     kwdDest,    idestSkip,
    "creatim",  0,      fFalse,     kwdDest,    idestSkip,
    "doccomm",  0,      fFalse,     kwdDest,    idestSkip,
    "fonttbl",  0,      fFalse,     kwdDest,    idestSkip,
    "footer",   0,      fFalse,     kwdDest,    idestSkip,
    "footerf",  0,      fFalse,     kwdDest,    idestSkip,
    "footerl",  0,      fFalse,     kwdDest,    idestSkip,
    "footerr",  0,      fFalse,     kwdDest,    idestSkip,
    "footnote", 0,      fFalse,     kwdDest,    idestSkip,
    "ftncn",    0,      fFalse,     kwdDest,    idestSkip,
    "ftnsep",   0,      fFalse,     kwdDest,    idestSkip,
    "ftnsepc",  0,      fFalse,     kwdDest,    idestSkip,
    "header",   0,      fFalse,     kwdDest,    idestSkip,
    "headerf",  0,      fFalse,     kwdDest,    idestSkip,
    "headerl",  0,      fFalse,     kwdDest,    idestSkip,
    "headerr",  0,      fFalse,     kwdDest,    idestSkip,
    "info",     0,      fFalse,     kwdDest,    idestSkip,
    "keywords", 0,      fFalse,     kwdDest,    idestSkip,
    "operator", 0,      fFalse,     kwdDest,    idestSkip,
    "pict",     0,      fFalse,     kwdDest,    idestSkip,
    "printim",  0,      fFalse,     kwdDest,    idestSkip,
    "private1", 0,      fFalse,     kwdDest,    idestSkip,
    "revtim",   0,      fFalse,     kwdDest,    idestSkip,
    "rxe",      0,      fFalse,     kwdDest,    idestSkip,
    "stylesheet",0,     fFalse,     kwdDest,    idestSkip,
    "subject",  0,      fFalse,     kwdDest,    idestSkip,
    "tc",       0,      fFalse,     kwdDest,    idestSkip,
    "title",    0,      fFalse,     kwdDest,    idestSkip,
    "txe",      0,      fFalse,     kwdDest,    idestSkip,
    "xe",       0,      fFalse,     kwdDest,    idestSkip,
    "{",        0,      fFalse,     kwdChar,    '{',
    "}",        0,      fFalse,     kwdChar,    '}',
    "\\",       0,      fFalse,     kwdChar,    '\\'
    };
std::size_t isymMax = sizeof(rgsymRtf) / sizeof(SYM);

// %%Function: ecApplyPropChange
// Set the property identified by _iprop_ to the value _val_.

int ecApplyPropChange(IPROP iprop, int val)
{
    unsigned char *pb = nullptr;
    if (rds == rdsSkip)                 // If we're skipping text,
        return ecOK;                    // Do not do anything.

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
            return ecBadTable;
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
        return ecParseSpecialProperty(iprop, val);
        break;
    default:
        return ecBadTable;
    }
    return ecOK;
}

// %%Function: ecParseSpecialProperty
// Set a property that requires code to evaluate.

int ecParseSpecialProperty(IPROP iprop, int/* val*/)
{
    switch (iprop)
    {
    case ipropPard:
        memset(&pap, 0, sizeof(pap));
        return ecOK;
    case ipropPlain:
        memset(&chp, 0, sizeof(chp));
        return ecOK;
    case ipropSectd:
        memset(&sep, 0, sizeof(sep));
        return ecOK;
    default:
        return ecBadTable;
    }
}

// %%Function: ecTranslateKeyword
// Step 3.
// Search rgsymRtf for szKeyword and evaluate it appropriately.
// Inputs:
// szKeyword:   The RTF control to evaluate.
// param:       The parameter of the RTF control.
// fParam:      fTrue if the control had a parameter; (that is, if param is valid)
//              fFalse if it did not.

int ecTranslateKeyword(char *szKeyword, int param, bool fParam)
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
        fSkipDestIfUnk = fFalse;
        return ecOK;
    }

    // found it!  Use kwd and idx to determine what to do with it.
    fSkipDestIfUnk = fFalse;
    switch (rgsymRtf[isym].kwd)
    {
    case kwdProp:
        if (rgsymRtf[isym].fPassDflt || !fParam)
            param = rgsymRtf[isym].dflt;
        return ecApplyPropChange(static_cast<IPROP>(rgsymRtf[isym].idx), param);
    case kwdChar:
        return ecParseChar(rgsymRtf[isym].idx);
    case kwdDest:
        return ecChangeDest(static_cast<IDEST>(rgsymRtf[isym].idx));
    case kwdSpec:
        return ecParseSpecialKeyword(static_cast<IPFN>(rgsymRtf[isym].idx));
    default:
        return ecBadTable;
    }
}

// %%Function: ecChangeDest
// Change to the destination specified by idest.
// There's usually more to do here than this...

int ecChangeDest(IDEST/* idest*/)
{
    if (rds == rdsSkip)             // if we're skipping text,
        return ecOK;                // Do not do anything

    rds = rdsSkip;              // when in doubt, skip it...
    return ecOK;
}

// %%Function: ecEndGroupAction
// The destination specified by rds is coming to a close.
// If there’s any cleanup that needs to be done, do it now.

int ecEndGroupAction(RDS/* rds*/)
{
    return ecOK;
}

// %%Function: ecParseSpecialKeyword
// Evaluate an RTF control that needs special processing.

int ecParseSpecialKeyword(IPFN ipfn)
{
    if (rds == rdsSkip && ipfn != ipfnBin)  // if we're skipping, and it is not
        return ecOK;                        // the \bin keyword, ignore it.
    switch (ipfn)
    {
    case ipfnBin:
        ris = risBin;
        cbBin = lParam;
        break;
    case ipfnSkipDest:
        fSkipDestIfUnk = fTrue;
        break;
    case ipfnHex:
        ris = risHex;
        break;
    default:
        return ecBadTable;
    }
    return ecOK;
}
