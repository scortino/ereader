// EViews workfile related data types

#include <stdint.h>

// Alias for primitive data type
typedef uint8_t  BYTE;

// Header of .wf1 file
typedef struct
{
    BYTE Unknown0[80];

    // Size of the header
    long HeaderSize;

    BYTE Unknown1[26];

    // Number of variables + 1
    int NumVarPlusOne;

    // Data of last modification or zero
    int LastMod;

    BYTE Unknown2[2];

    // Data frequency (e.g. 1 yearly, 4 quarterly)
    short DataFreq;

    BYTE Unknown3[2];

    // Starting observation
    int StartObs;

    // Starting subperiod
    long StartSubp;

    // Total number of observations
    int NumObs;

    BYTE Unknown4[2];
} __attribute__((__packed__))
EVIEWSHEADER;

// Block containing information about each variable in the file
typedef struct
{
    BYTE Unknown0[6];

    // Size of data record
    int RecSize;

    // Size of data block
    int BlockSize;

    // Stream position of data
    long DataPos;

    // Name of the variable, padded to the right with NULL
    char VarName[32];

    // Pointer to history information or zero if no history
    long PtrToHist;

    // Possibly nature of the object
    short ObjNat;

    BYTE Unknown1[6];
} __attribute__((__packed__))
EVIEWSVARIABLEINFO;

// Modification history, optional
typedef struct
{
    BYTE Unknown0[2];

    // Length of revision string
    int RevLen;

    // Possibly another length
    int UnknownLength;

    // Stream position of revision string
    long RevPos;
} __attribute__((__packed__))
EVIEWSHISTORY;

// Block containing information about observations of specific variable
typedef struct
{
    // Number of observations
    int NumObs;

    // Starting observation
    int StartObs;

    // Usually NULL
    BYTE Unknown0[8];

    // Ending observation
    int EndObs;

    // Usually NULL
    BYTE Unknown1[2];
} __attribute__((__packed__))
EVIEWSDATABLOCK;

// Actual structure containing observation
typedef double EVIEWSDATAOBS;
