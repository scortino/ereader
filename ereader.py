import ctypes as ct
from numpy.ctypeslib import ndpointer, as_array
import numpy as np
import pandas as pd


# Compile dynamic library
# os.popen("gcc -g -fPIC -Wall -Werror -Wextra -pedantic econverts_func.c -shared -o econverts_func.so")

# Load C dynamic library
libc = ct.cdll.LoadLibrary("./ereader.so")

# Get reader function from loaded library
c_read_wf1 = libc.read_wf1

# Define argument types of c_read_wf1
# c_read_wf1 takes as an argument a string
# representing the name of the .wf1 file
# to read
c_read_wf1.argtypes = [ct.c_char_p]

# Define return type of c_read_wf1
# c_read_wf1 return a struct containing
# information contained in the file 
# useful for its recostruction as a
# Pandas data frame
class EViewsInfo(ct.Structure):
    _fields_ = [("success", ct.c_bool),
                ("GlobNumVars", ct.c_int),
                ("NumVars", ct.c_int),
                ("NumObs", ct.c_int),
                ("VarNames", ct.POINTER(ct.c_char)),
                ("VarLengths", ct.POINTER(ct.c_int)),
                ("DataArray", ct.POINTER(ct.c_double))]

c_read_wf1.restype = EViewsInfo

# Get freer function from loaded library
# to free memory allocated for arrays
# after information stored are used
free = libc.free_ars

# Define argument types of free
# free gets three pointer returned by
# c_read_wf1 as argument
free.argtypes = [ct.POINTER(ct.c_char),
                 ct.POINTER(ct.c_int),
                 ct.POINTER(ct.c_double)]

# Define return type of free
free.restype = None

def read_wf1(infile: str):
    # Convert infile stirng into a byte string
    b_infile = ct.c_char_p(bytes(infile, encoding="utf8"))

    # Get EviewsInfo struct from 
    res = c_read_wf1(b_infile)

    # Check whether c_read_wf1 succeeded in extracting info from infile
    success = res.success
    if not success:
        raise Exception(f"Unable to extract information from {infile}\n")
    
    # Get global number of variables (including structural)
    globnumvars = int(res.GlobNumVars)
    
    # Get real number of variables
    numvars = int(res.NumVars)

    # Get number of observations
    numobs = int(res.NumObs)

    # Get pointer to array containing length of variable names
    varlengths_ptr = res.VarLengths

    # Get array from pointer
    varlengths_ar = as_array(varlengths_ptr, shape=(globnumvars, ))

    # Get pointer to array containing the chars that are present variable names
    varnames_ptr = res.VarNames

    # Get array from pointer
    varnames_ar = as_array(varnames_ptr, shape=(globnumvars * 32, ))

    # Decode the characters from b to utf-8 and join them into a single string
    varnames_str = ''.join(map(lambda ch: ch.decode("utf-8"), varnames_ar))

    # Initialize list for storing variable names
    varnames_list = []

    # Iterates over array containing lengths of the variable names
    i = 0
    for l in range(globnumvars):
        j = varlengths_ar[l]

        # Extract variable name from varnames_str
        varnames_list.append(varnames_str[i:i+j])

        # Update position in varnames_str
        i += j

    # Convert list into array for subsequent masking
    varnames_ar = np.array(varnames_list)


    # Get pointer to array containing observations
    data_ptr = res.DataArray

    # Get array from pointer
    data_ar = as_array(data_ptr, shape=(numobs * globnumvars, ))

    # Update entries representing missing data
    data_ar[data_ar == -100000] = np.nan

    # Reshape data_ar into numobs x globnumvars matrix
    data_ar = data_ar.reshape((numobs, globnumvars), order='F')

    # Create mask representing columns relative to structural variables
    mask = np.mean(data_ar, axis=0) != -99999
    mask = np.arange(globnumvars)[mask]

    # Keep only names of variables of interest
    varnames_ar = varnames_ar[mask]

    # Keep only columns containing data of variables of interest
    data_ar = data_ar[:, mask]

    # Create DataFrame with information extracted use varnames_ar as columns
    table = pd.DataFrame(data_ar, columns=varnames_ar)

    # Free memory dynamically allocated by C function for three arrays created in its execution 
    free(varnames_ptr, varlengths_ptr, data_ptr)

    return table
