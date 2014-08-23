#pragma once

// Inlined Math Functions
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define ABS(a) (((a) < 0) ?(-(a)):(a))
#define INTERPOLATE(a, min, max) (((max)-(min))*(a)+(min))

// A Better Way To Use Flooring And Rounding
static const f32 negOneHalf = -0.5f;
inline i32 fastFloor(f64 x) {
    i32 i;
    __asm {
        fld x;
        fadd st, st(0);
        fadd negOneHalf;
        fistp i;
        sar i, 1;
    };
    return (i);
}
inline i32 fastFloor(f32 x) {
    i32 i;
    __asm {
        fld x;
        fadd st, st(0);
        fadd negOneHalf;
        fistp i;
        sar i, 1;
    };
    return (i);
}
inline i32 fastCeil(f64 x) {
    i32 i;
    __asm {
        fld x;
        fadd st, st(0);
        fsubr negOneHalf;
        fistp i;
        sar i, 1;
    };
    return (-i);
}
inline i32 fastCeil(f32 x) {
    i32 i;
    __asm {
        fld x;
        fadd st, st(0);
        fsubr negOneHalf;
        fistp i;
        sar i, 1;
    };
    return (-i);
}

// Endianness Change
inline ui32 changeEndian(ui32 val) {
    // TODO: Use Assembly
    return (val << 24) | ((val << 8) & 0x00ff0000) | ((val >> 8) & 0x0000ff00) | ((val >> 24) & 0x000000ff);
}

#define RADIX_MAX_MASK_BITS 8
#define RADIX_MAX_BITS 31

// Radix Sort That Makes Sorted Index List
template<typename TData, typename TIndex, i32 bits>
void radixSort(TIndex* indices, TData* data, i32 n, i32(*converter)(TData*), i32 maxBits) {
    // We Don't Want This Many Bins
    if (bits < 1 || bits > RADIX_MAX_MASK_BITS) return;

    // To Prevent Sorting Negative Values Or Nothing
    if (maxBits < 1 || maxBits > RADIX_MAX_BITS || n < 1) return;

    // The Number Of Bins
    const i32 bins = 1 << bits;

    // Create The Bit Mask
    const i32 mask = bins - 1;

    // Create The Indices And Converted Values
    TIndex* inds = indices;
    ui32* converted = new ui32[n];
    register i32 i = 0;
    for (; i < n; i++) {
        inds[i] = i;
        converted[i] = converter(data + i) + 0x80000000;
    }

    // Buffer Arrays For Sorted Elements
    TIndex* indsBuf = new TIndex[n];
    ui32* convertedBuf = new ui32[n];

    // For The Count Sorting
    i32* binCounts = new i32[bins];
    register i32 bin;
    bool isUnsorted;

    // Sort All Of The Bits
    while (maxBits > 0) {
        maxBits -= bits;

        // Clear Counts
        memset(binCounts, 0, bins * sizeof(i32));

        // Count Number Of Elements In Each Bin
        isUnsorted = false;
        for (i = n - 1; i > 0; i--) {
            // Check For Unsorted
            if (converted[i] < converted[i - 1]) {
                isUnsorted = true;
                while (i > 0) {
                    binCounts[converted[i] & mask]++;
                    i--;
                }
                break;
            }
            binCounts[converted[i] & mask]++;
        }

        // Temporal Coherence Is Established
        if (!isUnsorted) break;

        binCounts[converted[0] & mask]++;


        // Make Counts As End-Value Indices
        for (i = 1; i < bins; i++) {
            binCounts[i] += binCounts[i - 1];
        }

        for (i = n - 1; i >= 0; i--) {
            bin = converted[i] & mask;

            // Set The Data
            binCounts[bin]--;
            indsBuf[binCounts[bin]] = inds[i];
            convertedBuf[binCounts[bin]] = converted[i] >> bits;
        }

        // Swap Pointers
        TIndex* sbuf1 = indsBuf; indsBuf = inds; inds = sbuf1;
        ui32* sbuf2 = convertedBuf; convertedBuf = converted; converted = sbuf2;
    }

    // Delete Temporary Data Buffers
    delete converted;
    delete convertedBuf;

    // Make Sure Initial Entry Is Modified
    if (inds != indices) {
        memcpy_s(indices, n * sizeof(TIndex), inds, n * sizeof(TIndex));
        delete inds;
    } else {
        delete indsBuf;
    }
}
// Radix Sort That Sorts In Place
template<typename TData, i32 bits>
void radixSort(TData* data, i32 n, i32(*converter)(TData*), i32 maxBits) {
    // We Don't Want This Many Bins
    if (bits < 1 || bits > RADIX_MAX_MASK_BITS) return;

    // To Prevent Sorting Negative Values Or Nothing
    if (maxBits < 1 || maxBits > RADIX_MAX_BITS || n < 1) return;

    // The Number Of Bins
    const i32 bins = 1 << bits;

    // Create The Bit Mask
    const i32 mask = bins - 1;

    // Create The Indices And Converted Values
    TData* dBuf1 = data;
    ui32* converted = new ui32[n];
    register i32 i = 0;
    for (; i < n; i++) {
        converted[i] = converter(data + i) + 0x80000000;
    }

    // Buffer Arrays For Sorted Elements
    TData* dBuf2 = new TData[n];
    ui32* convertedBuf = new ui32[n];

    // For The Count Sorting
    i32* binCounts = new i32[bins];
    register i32 bin;
    bool isUnsorted;

    // Sort All Of The Bits
    while (maxBits > 0) {
        maxBits -= bits;

        // Clear Counts
        memset(binCounts, 0, bins * sizeof(i32));

        // Count Number Of Elements In Each Bin
        isUnsorted = false;
        for (i = n - 1; i > 0; i--) {
            // Check For Unsorted
            if (converted[i] < converted[i - 1]) {
                isUnsorted = true;
                while (i > 0) {
                    binCounts[converted[i] & mask]++;
                    i--;
                }
                break;
            }
            binCounts[converted[i] & mask]++;
        }

        // Temporal Coherence Is Established
        if (!isUnsorted) break;

        binCounts[converted[0] & mask]++;


        // Make Counts As End-Value Indices
        for (i = 1; i < bins; i++) {
            binCounts[i] += binCounts[i - 1];
        }

        for (i = n - 1; i >= 0; i--) {
            bin = converted[i] & mask;

            // Set The Data
            binCounts[bin]--;
            dBuf2[binCounts[bin]] = dBuf1[i];
            convertedBuf[binCounts[bin]] = converted[i] >> bits;
        }

        // Swap Pointers
        TData* sbuf1 = dBuf2; dBuf2 = dBuf1; dBuf1 = sbuf1;
        ui32* sbuf2 = convertedBuf; convertedBuf = converted; converted = sbuf2;
    }

    // Delete Temporary Data Buffers
    delete converted;
    delete convertedBuf;

    // Make Sure Initial Entry Is Modified
    if (dBuf1 != data) {
        memcpy_s(data, n * sizeof(TData), dBuf1, n * sizeof(TData));
        delete dBuf1;
    } else {
        delete dBuf2;
    }
}

// Ranges Of Different Types
#define RANGE_STRUCT(NAME, TYPE) struct NAME##Range { TYPE min; TYPE max; }
RANGE_STRUCT(I8, i8);
RANGE_STRUCT(I16, i16);
RANGE_STRUCT(I32, i32);
RANGE_STRUCT(I64, i64);
RANGE_STRUCT(UI8, ui8);
RANGE_STRUCT(UI16, ui16);
RANGE_STRUCT(UI32, ui32);
RANGE_STRUCT(UI64, ui64);
RANGE_STRUCT(F32, f32);
RANGE_STRUCT(F64, f64);


// ************************ String Utilities ************************
inline cString convertWToMBString(cwString ws) {
    i32 l = wcslen(ws);
    cString mbs = new char[l + 1];
    size_t numConverted;
    wcstombs_s(&numConverted, mbs, l + 1, ws, l);
    mbs[l] = 0;
    return mbs;
}


// ************************ Interpolation Utilities ************************
template <typename T>
T trilinearInterpolation_4(int x, int y, int z, T data[9][9][9])
{
    /*Vxyz = 	V000 (1 - x) (1 - y) (1 - z) +
    V100 x (1 - y) (1 - z) +
    V010 (1 - x) y (1 - z) +
    V001 (1 - x) (1 - y) z +
    V101 x (1 - y) z +
    V011 (1 - x) y z +
    V110 x y (1 - z) +
    V111 x y z */


    T px = ((T)(x % 4)) / 4.0;
    T py = ((T)(y % 4)) / 4.0;
    T pz = ((T)(z % 4)) / 4.0;
    T x /= 4;
    T y /= 4;
    T z /= 4;


    return  (data[y][z][x])*(1 - px)*(1 - py)*(1 - pz) +
        (data[y][z][x + 1])*px*(1 - py)*(1 - pz) +
        (data[y + 1][z][x])*(1 - px)*py*(1 - pz) +
        (data[y][z + 1][x])*(1 - px)*(1 - py)*pz +
        (data[y][z + 1][x + 1])*px*(1 - py)*pz +
        (data[y + 1][z + 1][x])*(1 - px)*py*pz +
        (data[y + 1][z][x + 1])*px*py*(1 - pz) +
        (data[y + 1][z + 1][x + 1])*px*py*pz;
}

template <typename T>
T trilinearInterpolation_4_8_4(int x, int y, int z, T data[9][5][5])
{
    /*Vxyz = 	V000 (1 - x) (1 - y) (1 - z) +
    V100 x (1 - y) (1 - z) +
    V010 (1 - x) y (1 - z) +
    V001 (1 - x) (1 - y) z +
    V101 x (1 - y) z +
    V011 (1 - x) y z +
    V110 x y (1 - z) +
    V111 x y z */

    T px = ((T)(x % 8)) / 8.0;
    T py = ((T)(y % 4)) / 4.0;
    T pz = ((T)(z % 8)) / 8.0;
    x /= 8;
    y /= 4;
    z /= 8;


    return  (data[y][z][x])*(1 - px)*(1 - py)*(1 - pz) +
        (data[y][z][x + 1])*px*(1 - py)*(1 - pz) +
        (data[y + 1][z][x])*(1 - px)*py*(1 - pz) +
        (data[y][z + 1][x])*(1 - px)*(1 - py)*pz +
        (data[y][z + 1][x + 1])*px*(1 - py)*pz +
        (data[y + 1][z + 1][x])*(1 - px)*py*pz +
        (data[y + 1][z][x + 1])*px*py*(1 - pz) +
        (data[y + 1][z + 1][x + 1])*px*py*pz;
}

template <typename T>
T lerp(T a, T b, float f) {
    return a * (1.0f - f) + b * f;
}

inline i32 getPositionSeed(int x, int y, int z) {
    return ((x & 0x7FF) << 10) | ((y & 0x3FF)) | ((z & 0x7FF) << 21);
}

inline i32 getPositionSeed(int x, int z) {
    return ((x & 0xFFFF) << 16) | (z & 0xFFFF);
}

//******************************* Buffer Utilities ***********************************
//Big endian buffer utils
namespace BufferUtils {

    inline ui32 extractInt(ui8* a, ui32 i) {
        return (((ui32)a[i]) << 24) | (((ui32)a[i + 1]) << 16) | (((ui32)a[i + 2]) << 8) | (((ui32)a[i + 3]));
    }
    inline ui32 extractInt(ui8* a) {
        return (((ui32)a[0]) << 24) | (((ui32)a[1]) << 16) | (((ui32)a[2]) << 8) | (((ui32)a[3]));
    }

    inline ui16 extractShort(ui8* a, ui32 i) {
        return (((ui32)a[i]) << 8) | (((ui32)a[i + 1]));
    }
    inline ui16 extractShort(ui8* a) {
        return (((ui32)a[0]) << 8) | (((ui32)a[1]));
    }

    inline f32 extractFloat(ui8* a, ui32 i) {
        ui32 n = extractInt(a, i);
        return *((f32*)(&n));
    }
    inline f32 extractFloat(ui8* a) {
        ui32 n = extractInt(a);
        return *((f32*)(&n));
    }

    inline bool extractBool(ui8* a, ui32 i) {
        return (a[i] != 0);
    }

    inline i32 setInt(ui8* a, ui32 i, ui32 data) {
        a[i] = (ui8)(data >> 24);
        a[i + 1] = (ui8)((data & 0x00FF0000) >> 16);
        a[i + 2] = (ui8)((data & 0x0000FF00) >> 8);
        a[i + 3] = (ui8)(data & 0x000000FF);
        return sizeof(ui32);
    }
    inline i32 setInt(ui8* a, ui32 data) {
        a[0] = (ui8)(data >> 24);
        a[1] = (ui8)((data & 0x00FF0000) >> 16);
        a[2] = (ui8)((data & 0x0000FF00) >> 8);
        a[3] = (ui8)(data & 0x000000FF);
        return sizeof(ui32);
    }

    inline i32 setShort(ui8* a, ui32 i, ui32 data) {
        a[i] = (ui8)((data & 0xFF00) >> 8);
        a[i + 1] = (ui8)(data & 0x00FF);
        return sizeof(ui32);
    }
    inline i32 setShort(ui8* a, ui32 data) {
        a[0] = (ui8)((data & 0xFF00) >> 8);
        a[1] = (ui8)(data & 0x00FF);
        return sizeof(ui32);
    }

    inline i32 setFloat(ui8* a, ui32 i, f32 data) {
        setInt(a, i, *((ui32*)(&data)));
        return sizeof(f32);
    }
    inline i32 setFloat(ui8* a, f32 data) {
        setInt(a, *((ui32*)(&data)));
        return sizeof(f32);
    }

    inline i32 setBool(ui8* a, ui32 i, bool data) {
        a[i] = (ui8)data;
        return sizeof(ui8);
    }

}