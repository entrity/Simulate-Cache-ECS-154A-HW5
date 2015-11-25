// Author: Sean Davis

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <iomanip>
using namespace std;

void createOps(int RAMSize, int lineSize, ofstream &outf, int numOps, 
  unsigned char *memory, int numLines)
{
  int lineNum, address, bytePos, numWindows, value, addresses[256][2], base;
  
  for(int i = 0; i < RAMSize; i++)
    memory[i] = 0;
  
  base = rand() % numLines;
  for(int i = 0; i < numLines; i++)
  {
    addresses[i][0] = i * lineSize + lineSize * numLines 
      * (rand() % (RAMSize / (numLines * lineSize) - 2));
    
    do
    {
        addresses[i][1] = i * lineSize + lineSize * numLines 
      * (rand() % (RAMSize / (numLines * lineSize) - 2));
    } while(addresses[i][0] == addresses[i][1]);
  }  // for each cache line create two random addresses that will mod to it
  
  numWindows = numOps / (numLines * 4) + 1; // ensures reuse of addresses 

  if(numWindows > numLines)
    numWindows = numLines;
  
  for(int i = 0; i < numOps; i++)
  {
    lineNum = (base + (rand() % numWindows)) % numLines;
    bytePos = rand() % (lineSize);
    address = addresses[lineNum][rand() % 2] + bytePos;
    outf << hex << right << setfill('0') <<  setw(4) << address;
   // if(memory[address] == 0 || (rand() % 4) == 0) // never been used or random write
    if(rand() % 4 < 3)  // 3/4 are writes
    {
      value = (rand() % 0xff + 1);  // guaranteed not zero.
      outf << " FF " << setw(2) <<  value << endl;
      memory[address] = value;
    } // if write
    else  // read
      outf <<  " 00 " << setw(2) << (unsigned) memory[address] << endl;
  }  // for each op
} // createDirect()


int main(int argc, char** argv)
{
  int seed, RAMSize, lineSize, numOps, numLines;
  char filename[80]; 
  unsigned char memory[0x10000];
  cout << "This program creates a test file for direct\n"
    << "cache projects using random reuse of two addresses for each cache line\n"
    << "to mimic locality.  It expects valid inputs from the user.\n"
    << "Values stored in memory will be non-zero.\n"  //  No reads before writes.\n"
    << "Read data is an accurate value of that address in memory.\n";
    
  
  cout << "Random generator seed (1-1000) >> ";
  cin >> seed;
  srand(seed);
  cout << "Size of RAM (in bytes, max 65536 = 64K) >> ";
  cin >> RAMSize;
  memset(memory,RAMSize, 0);
  cout << "Cache line size (in bytes) >> ";
  cin >> lineSize;
  cout << "Number of lines in cache >> ";
  cin >> numLines;
  cout << "Number of operations (min 20) >> ";
  cin >> numOps;
  sprintf(filename, "dmtest-%d-%d-%d-%d.txt",lineSize, numLines, numOps, seed);
  cout << "Filename: " << filename << endl;
  ofstream outf(filename);
  outf << hex << uppercase <<  setfill('0');
  createOps(RAMSize, lineSize, outf, numOps, memory, numLines);
  return 0;
} // main()

