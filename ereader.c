// Read information about EViews workfile (.wf1) into 
// a structure to be handled through Python's ctypes

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// .wf1 related data types
#include "wf1.h"

// Global flag for error message
bool flag = false;

// Return type containing relevant info of .wf1 file
typedef struct
{
    bool success;

    // Global variables (also structural)
    int GlobNumVars;

    // Variables of interest
    int NumVars;

    int NumObs;
    char *VarNames;
    int *VarLengths;
    EVIEWSDATAOBS *DataArray;
}
EVIEWSINFO;

// Extract information from infile into a EVIEWSINFO structure
EVIEWSINFO read_wf1(char *infile)
{
    // Initialize EVIEWSFILE structure that will be returned
    EVIEWSINFO outstruct;
    outstruct.success = false;

    // Remember infile's extension
    char *infile_ext = strrchr(infile, '.');

    // Ensure infile is a .wf1 file
    if (strcmp(infile_ext, ".wf1") != 0)
    {
        fprintf(stderr, "Cannot handle %s files \n", infile_ext);
        return outstruct;
    }

    // Open infile
    FILE *inptr = fopen(infile, "r");
    if (!inptr)
    {
        fprintf(stderr, "Unable to open %s in read mode\n", infile);
        return outstruct;
    }

    // Initialize word buffer
    char word[22];
    word[21] = '\0';

    // Get string at beginning of infile
    fgets(word, 22, inptr);

    // Ensure infile compatibility
    if (strcmp(word, "New MicroTSP Workfile") != 0)
    {
        fprintf(stderr, "EViews workfile not compatible\n");
        return outstruct;
    }

    // Go back to the beginning of the file
    rewind(inptr);

    // Initialize header
    EVIEWSHEADER vh;

    // Read header into vh
    fread(&vh, sizeof(EVIEWSHEADER), 1, inptr);

    // Remember length of header
    int hlen = vh.HeaderSize;

    // Get number of variables
    outstruct.GlobNumVars = vh.NumVarPlusOne - 1; 

    // Store initial number of variables in return struct
    outstruct.NumVars = outstruct.GlobNumVars;

    // Get global number of observations
    outstruct.NumObs = vh.NumObs;

    // Go to the first EVIEWVARIABLEINFO
    fseek(inptr, hlen + 26, SEEK_SET);

    // Variable info buffer
    EVIEWSVARIABLEINFO vi;

    // Data block buffer
    EVIEWSDATABLOCK db;

    // Allocate memory for matrix to collect name of variables
    outstruct.VarNames = malloc(sizeof(char) * outstruct.GlobNumVars * 32);
    if (!outstruct.VarNames)
    {
        fprintf(stderr, "Unable to allocate memory for VarNames array\n");
        return outstruct;
    }

    // Allocate memory for matrix to collect lenght of variables' name
    outstruct.VarLengths = malloc(sizeof(int) * outstruct.GlobNumVars);
    if (!outstruct.VarNames)
    {
        fprintf(stderr, "Unable to allocate memory for VarLenghts array\n");
        return outstruct;
    }

    // Allocate memory for matrix to collect DATAOBS
    outstruct.DataArray = malloc(sizeof(EVIEWSDATAOBS) *outstruct.GlobNumVars * outstruct.NumObs); 
    if (!outstruct.VarNames)
    {
        fprintf(stderr, "Unable to allocate memory for data array\n");
        return outstruct;
    }

    // Do for every variable
    for (int i = 0; i < outstruct.GlobNumVars; i++)
    {
        // Go to the datablock of variable i
        fseek(inptr, hlen + 26 + i * sizeof(EVIEWSVARIABLEINFO), SEEK_SET);

        // Read the information about the variable and store it in vi
        fread(&vi, sizeof(EVIEWSVARIABLEINFO), 1, inptr);

        // Remember length of variable name
        int l = strlen(vi.VarName);

        // Store variable length in length array
        outstruct.VarLengths[i] = l;

        // Store variable name in name array
        for (int j = 0; j < 32; j++)
        {
            outstruct.VarNames[32 * i + j] = (j < l) ? vi.VarName[j] : '\0';  
        }

        // If variable is structural or unknown
        if (vi.ObjNat != 44 || strcmp(vi.VarName, "RESID") == 0 || strncmp(vi.VarName, "SERIES", 6) == 0)
        {
            // Update the number of variable of interest
            outstruct.NumVars--;

            // Signal the presence of unknown or structural variable in matrix with sentinel value
            for (int j = 0; j < outstruct.NumObs; j++)
            {
                outstruct.DataArray[i * outstruct.NumObs + j] = -99999;
            }
            
            // Go to the next variable without reading data
            continue;
        }

        // Go to the actual data block
        fseek(inptr, vi.DataPos, SEEK_SET);

        // Read data block into db buffer
        fread(&db, sizeof(EVIEWSDATABLOCK), 1, inptr);

        // Assert that the number of observations found in the
        // data block matched the global number of observation
        if (db.NumObs != outstruct.NumObs && !flag)
        {
            fprintf(stderr, "Inconsistent number of observation found, the resulting structure may contain errors\n");

            // Update flag to avoid mutiple error messages
            flag = true;
        }

        // For each actual data point in the data block
        for (int j = 0; j < outstruct.NumObs; j++)
        {
            // Read data point and store it into data array
            fread(&outstruct.DataArray[i * outstruct.NumObs + j], sizeof(EVIEWSDATAOBS), 1, inptr);

            // If value is missing or NaN
            if ((outstruct.DataArray[i * outstruct.NumObs + j] == 1e-37) || (outstruct.DataArray[i * outstruct.NumObs + j] != outstruct.DataArray[i * outstruct.NumObs + j]))
            {
                // Signal the presence of missing or NaN data with sentinel value
                outstruct.DataArray[i * outstruct.NumObs + j] = -100000;
            }
        }
    }

    // Close infile
    fclose(inptr);

    // Everything went (hopefully) fine
    outstruct.success = true;

    return outstruct;
}

// Free the memory allocated for the three arrays created by the read_wf1()
void free_ars(char *VarNames, int *VarLengths, EVIEWSDATAOBS *DataArray)
{
    free(VarNames);
    free(VarLengths);
    free(DataArray);
}
