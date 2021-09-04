/*
 * File posted by ZrX-oMs on #vice-dev.
 *
 * Might be a decent starting point for TAP support. Although the extension
 * is ".cpp", the code is plain C.
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

//#define __AUDIO__

FILE *fhin, *fhout;

char *infile, *outfile;

#ifdef __AUDIO__
unsigned int audiosize;
unsigned char audio[30000000];
#endif

unsigned char prg[0x900000];

unsigned char TAPheader[] = {0x43, 0x36, 0x34, 0x2D, 0x54, 0x41, 0x50, 0x45, 0x2D, 0x52, 0x41, 0x57, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char CBMheader[] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x54, 0x45, 0x53, 0x54, 0x46, 0x49, 0x4C, 0x45, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
unsigned char TTheader[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x45, 0x53, 0x54, 0x46, 0x49, 0x4C, 0x45, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
unsigned char sp = 0x30, mp = 0x42, lp = 0x56;
unsigned char tsb = 0x1A, tlb = 0x28;
unsigned char pilot[0x6A00];
unsigned char byte[21];
unsigned char bit[3];

void ropen()
{
    fhin=fopen(infile,"rb");
    if (!fhin)
    {
        printf("\nERROR: Couldn't open file %s!\n", infile);
        exit(-1);
    }
}

void rclose()
{
    fclose(fhin);
}

void wopen()
{
    fhout=fopen(outfile,"wb");
    if (!fhout)
    {
        printf("\nERROR: Couldn't open file %s!\n", outfile);
        exit(-1);
    }
}

void wclose()
{
    fclose(fhout);
}

//--------------------------------------------------------------------------------------------------
// Render silence
//--------------------------------------------------------------------------------------------------
#ifdef __AUDIO__
void WAVpause(unsigned int length)
{
    unsigned int count;

    if (length != 0)
    {
        for (count = 0;  count < length; count++)
        {
            audio[audiosize++] = 0;
        }
    }
}
#endif
//--------------------------------------------------------------------------------------------------
// Render sinewave from TAP CBM Kernal load pulse value
//--------------------------------------------------------------------------------------------------
#ifdef __AUDIO__
void WAVgenerate(unsigned int bit, unsigned int length)
{
    unsigned int count, tcount, pstep1, pstep2;
    float fpulse1, fpulse2;

    fpulse1 = (float) 360 / (48000 / (985248 / (8 * (bit * 2 - 0x15))));
    pstep1 = (unsigned int) fpulse1;
    fpulse2 = 360 / (48000 / (985248 / (8 * 0x15)));
    pstep2 = (unsigned int) fpulse2;

    if (length != 0)
    {
        do
        {
            for (count = 0; count < 180; count += pstep1)
            {
                audio[audiosize++] = (unsigned char) (sin(count * 0.017453292) * 75);
            }
            tcount = count;
            for (count = tcount; count < 360; count += pstep2)
            {
                audio[audiosize++] = (unsigned char) (sin(count * 0.017453292) * 75);
            }
            length--;
        } while (length != 0);
    }
}
#endif
//--------------------------------------------------------------------------------------------------
// Convert byte to CBM Kernal load TAP values
//--------------------------------------------------------------------------------------------------

void pulse(unsigned char data)
{
    unsigned int pos, bitcount;
    unsigned char check;
    bitcount = 0;
    pos = 0;
    check = 1;
    bit[0] = lp;                                // Start bitpair
    bit[1] = mp;
    do {
        byte[pos] = bit[0];
        byte[pos + 1] = bit[1];

#ifdef __AUDIO__
        WAVgenerate(bit[0], 1);
        WAVgenerate(bit[1], 1);
#endif

        if (((data >> bitcount) & 1) == 0)      // 0 bitpair
        {
            bit[0] = sp;
            bit[1] = mp;
        }
        else                                    // 1 bitpair
        {
            bit[0] = mp;
            bit[1] = sp;
        }
        check = check ^ ((data >> bitcount) & 1);
        bitcount++;
        pos = pos + 2;
    } while (pos < 18);
    if (check == 0)                             // Parity 0 bitpair
    {
        bit[0] = sp;
        bit[1] = mp;
    }
    else                                        // Parity 1 bitpair
    {
        bit[0] = mp;
        bit[1] = sp;
    }
    byte[pos] = bit[0];
    byte[pos + 1] = bit[1];

#ifdef __AUDIO__
    WAVgenerate(bit[0], 1);
    WAVgenerate(bit[1], 1);
#endif

}

//--------------------------------------------------------------------------------------------------
// Render sinewave from TAP Turbotape pulse value
//--------------------------------------------------------------------------------------------------
#ifdef __AUDIO__
void WAVturbogen(unsigned int bit, unsigned int length)
{
    unsigned int count, tcount, pstep1, pstep2;
    float fpulse1, fpulse2;

    fpulse1 = (float) 360 / (48000 / (985248 / (8 * (bit * 2 - 0x0D))));
    pstep1 = (unsigned int) fpulse1;
    fpulse2 = 360 / (48000 / (985248 / (8 * 0x0D)));
    pstep2 = (unsigned int) fpulse2;

    if (length != 0)
    {
        do
        {
            for (count = 0; count < 180; count += pstep1)
            {
                audio[audiosize++] = (unsigned char) (sin(count * 0.017453292) * 75);
            }
            tcount = count;
            for (count = tcount; count < 360; count += pstep2)
            {
                audio[audiosize++] = (unsigned char) (sin(count * 0.017453292) * 75);
            }
            length--;
        } while (length != 0);
    }
}
#endif
//--------------------------------------------------------------------------------------------------
// Convert byte to Turbotape TAP values
//--------------------------------------------------------------------------------------------------

void turbo(unsigned char data)
{
    unsigned int pos, bitcount;
    bitcount = 7;
    pos = 0;
    do {
        if (((data >> bitcount) & 1) == 0)      // 0 bit
        {
            bit[0] = tsb;
        }
        else                                    // 1 bit
        {
            bit[0] = tlb;
        }
        byte[pos++] = bit[0];
        bitcount--;

#ifdef __AUDIO__
        WAVturbogen(bit[0], 1);
#endif

    } while (pos < 8);
}

//--------------------------------------------------------------------------------------------------
// Here we go again...
//--------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    unsigned int binsize, offset, TAPsize, count;
    unsigned char check;

#ifdef __AUDIO__
    audiosize = 0;
#endif

    infile = "turbo3.prg";                      // Load file to be encoded as CBM Kernal load
    ropen();
    binsize = fread(prg, 1, 0x900000, fhin);
    rclose();

    outfile = argv[2];                          // Create file for TAP output
    wopen();

    memset(pilot, 0x30, 0x6A00);                // Generate and write TAP header
    TAPsize = 8 + 0x9F35 + 0x4F + 0x4E + (binsize + 8) * 40;
    TAPheader[0x10] = TAPsize;
    TAPheader[0x11] = TAPsize >> 8;
    TAPheader[0x12] = TAPsize >> 16;
    TAPheader[0x13] = TAPsize >> 24;
    fwrite(TAPheader, 1, 20, fhout);
//    for (count=0;count<20;count++)
//    {
//        generate(TAPheader[count], 1);
//    }

    CBMheader[1] = prg[0];                      // Place load address and size into CBM Kernal load header
    CBMheader[2] = prg[1];
    CBMheader[3] = binsize - 2 + (prg[0] | (prg[1] << 8));
    CBMheader[4] = (binsize - 2 + (prg[0] | (prg[1] << 8))) >> 8;
    offset = 0;
    check = 0;
    fwrite(pilot, 1, 27136, fhout);             // Write CBM Kernal load header pilot to TAP

#ifdef __AUDIO__
    WAVgenerate(0x30, 27136);
#endif

    for (count = 0; count < 9; count++)         // Write CBM Kernal load first header sync countdown to TAP
    {
        pulse(0x89 - count);
        fwrite(byte, 1, 20, fhout);
    }
    do {                                        // Write CBM Kernal load header to TAP
        pulse(CBMheader[offset]);
        check = check ^ CBMheader[offset];
        offset++;
        fwrite(byte, 1, 20, fhout);
    } while (offset < sizeof(CBMheader));
    pulse(check);                               // Write CBM Kernal load header checksum to TAP
    fwrite(byte, 1, 20, fhout);
    bit[0] = lp;                                // Write CBM Kernal load EOF flag to TAP
    bit[1] = sp;
    fwrite(bit, 1, 2, fhout);

#ifdef __AUDIO__
    WAVgenerate(bit[0], 1);
    WAVgenerate(bit[1], 1);
#endif

    offset = 0;
    check = 0;
    fwrite(pilot, 1, 79, fhout);                // Write CBM Kernal load header pilot to TAP

#ifdef __AUDIO__
    WAVgenerate(0x30, 79);
#endif

    for (count = 0; count < 9; count++)         // Write CBM Kernal load repeat header sync countdown to TAP
    {
        pulse(0x09 - count);
        fwrite(byte, 1, 20, fhout);
    }
    do {                                        // Write CBM Kernal load header to TAP
        pulse(CBMheader[offset]);
        check = check ^ CBMheader[offset];
        offset++;
        fwrite(byte, 1, 20, fhout);
    } while (offset < sizeof(CBMheader));
    pulse(check);                               // Write CBM Kernal load header checksum to TAP
    fwrite(byte, 1, 20, fhout);
    bit[0] = lp;                                // Write CBM Kernal load EOF flag to TAP
    bit[1] = sp;
    fwrite(bit, 1, 2, fhout);

#ifdef __AUDIO__
    WAVgenerate(bit[0], 1);
    WAVgenerate(bit[1], 1);
#endif

    fwrite(pilot, 1, 78, fhout);                // Write CBM Kernal load header trailer to TAP

#ifdef __AUDIO__
    WAVgenerate(0x30, 78);
#endif

    byte[0] = 0x00;                             // Write pause to TAP
    byte[1] = 0x00;
    byte[2] = 0xE2;
    byte[3] = 0x04;
    fwrite(byte, 1, 4, fhout);

#ifdef __AUDIO__
    WAVpause(13230);
#endif

    offset = 2;
    check = 0;
    fwrite(pilot, 1, 5376, fhout);              // Write CBM Kernal load data pilot to TAP

#ifdef __AUDIO__
    WAVgenerate(0x30, 5376);
#endif

    for (count = 0; count < 9; count++)         // Write CBM Kernal load first data sync countdown to TAP
    {
        pulse(0x89 - count);
        fwrite(byte, 1, 20, fhout);
    }
    do {                                        // Write CBM Kernal load data to TAP
        pulse(prg[offset]);
        check = check ^ prg[offset];
        offset++;
        fwrite(byte, 1, 20, fhout);
    } while (offset < binsize);
    pulse(check);                               // Write CBM Kernal load data checksum to TAP
    fwrite(byte, 1, 20, fhout);
    bit[0] = lp;                                // Write CBM Kernal load EOF flag to TAP
    bit[1] = sp;
    fwrite(bit, 1, 2, fhout);

#ifdef __AUDIO__
    WAVgenerate(bit[0], 1);
    WAVgenerate(bit[1], 1);
#endif

    offset = 2;
    check = 0;
    fwrite(pilot, 1, 79, fhout);                // Write CBM Kernal load data pilot to TAP

#ifdef __AUDIO__
    WAVgenerate(0x30, 79);
#endif

    for (count = 0; count < 9; count++)         // Write CBM Kernal load repeat data sync countdown to TAP
    {
        pulse(0x09 - count);
        fwrite(byte, 1, 20, fhout);
    }
    do {                                        // Write CBM Kernal load data to TAP
        pulse(prg[offset]);
        check = check ^ prg[offset];
        offset++;
        fwrite(byte, 1, 20, fhout);
    } while (offset < binsize);
    pulse(check);                               // Write CBM Kernal load data checksum to TAP
    fwrite(byte, 1, 20, fhout);
    bit[0] = lp;                                // Write CBM Kernal load EOF flag to TAP
    bit[1] = sp;
    fwrite(bit, 1, 2, fhout);

#ifdef __AUDIO__
    WAVgenerate(bit[0], 1);
    WAVgenerate(bit[1], 1);
#endif

    fwrite(pilot, 1, 78, fhout);                // Write CBM Kernal load data trailer to TAP

#ifdef __AUDIO__
    WAVgenerate(0x30, 78);
#endif

//    byte[0] = 0x00;
//    byte[1] = 0x20;
//    byte[2] = 0x2B;
//    byte[3] = 0x4B;
    byte[0] = 0x00;                             // Write pause to TAP
    byte[1] = 0x00;
    byte[2] = 0xE2;
    byte[3] = 0x04;
    fwrite(byte, 1, 4, fhout);

#ifdef __AUDIO__
    WAVpause(13230);
#endif

//    wclose();

//    outfile = argv[2];
//    wopen();

//--------------------------------------------------------------------------------------------------

    infile = argv[1];                           // Load file to be encoded as Turbotape
    ropen();
    binsize = fread(prg, 1, 0x900000, fhin);
    rclose();

    for (count = 0; count < 1271; count++)      // Generate Turbotape header pilot
    {
        pilot[count*8] = 0x1A;
        pilot[count*8+1] = 0x1A;
        pilot[count*8+2] = 0x1A;
        pilot[count*8+3] = 0x1A;
        pilot[count*8+4] = 0x1A;
        pilot[count*8+5] = 0x1A;
        pilot[count*8+6] = 0x28;
        pilot[count*8+7] = 0x1A;
    }
//    TAPsize = 0x45FC + binsize * 8;
//    TAPheader[0x10] = TAPsize;
//    TAPheader[0x11] = TAPsize >> 8;
//    TAPheader[0x12] = TAPsize >> 16;
//    TAPheader[0x13] = TAPsize >> 24;
//    fwrite(TAPheader, 1, 20, fhout);

    TTheader[1] = prg[0];                       // Place load address and size into Turbotape header
    TTheader[2] = prg[1];
    TTheader[3] = binsize - 2 + (prg[0] | (prg[1] << 8));
    TTheader[4] = (binsize - 2 + (prg[0] | (prg[1] << 8))) >> 8;
    offset = 0;
    fwrite(pilot, 1, 10168, fhout);             // Write Turbotape header pilot to TAP

#ifdef __AUDIO__
    for (count = 0; count < 1271; count++)
    {
        turbo(0x02);
    }
#endif

    for (count = 0; count < 9; count++)         // Write Turbotape header sync countdown to TAP
    {
        turbo(0x09 - count);
        fwrite(byte, 1, 8, fhout);
    }
    do {                                        // Write Turbotape header to TAP
        turbo(TTheader[offset]);
        offset++;
        fwrite(byte, 1, 8, fhout);
    } while (offset < sizeof(TTheader));
    fwrite(pilot + 4, 1, 1360, fhout);          // Write Turbotape header trailer to TAP

#ifdef __AUDIO__
    for (count = 0; count < 170; count++)
    {
        turbo(0x20);
    }
#endif

    offset = 2;
    check = 0;
    fwrite(pilot, 1, 4024, fhout);              // Write Turbotape data pilot to TAP

#ifdef __AUDIO__
    for (count = 0; count < 503; count++)
    {
        turbo(0x02);
    }
#endif

    for (count = 0; count < 10; count++)        // Write Turbotape data sync countdown to TAP
    {
        turbo(0x09 - count);
        fwrite(byte, 1, 8, fhout);
    }
    do {                                        // Write Turbotape data to TAP
        turbo(prg[offset]);
        check = check ^ prg[offset];
        offset++;
        fwrite(byte, 1, 8, fhout);
    } while (offset < binsize);
    turbo(check);                               // Write Turbotape data checksum to TAP
    fwrite(byte, 1, 8, fhout);

    memset(pilot, 0x1A, 0x6A00);                // Write Turbotape data trailer to TAP
    fwrite(pilot, 1, 2040, fhout);

#ifdef __AUDIO__
    for (count = 0; count < 255; count++)
    {
        turbo(0x00);
    }
#endif

    byte[0] = 0x00;                             // Write pause to TAP
    byte[1] = 0x20;
    byte[2] = 0x2B;
    byte[3] = 0x4B;
    fwrite(byte, 1, 4, fhout);

#ifdef __AUDIO__
    WAVpause(13230);

    outfile = argv[3];                          // Write WAV generated from the encoded TAP
    wopen();
    fwrite(audio, 1, audiosize, fhout);
    wclose();
#endif

    return 0;
}
