// - Take file from command line
// - Read line from file: (addr op val)
// - Perform op
// - If read, output

#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdint.h>
using namespace std;

#define BLOCK_SIZE 8
#define OFFSET_MASK 7
#define LINE_MASK 63
#define LINE_CT 512
#define RAM_BYTES 0x10000

typedef struct {
	unsigned long tag;
	uint8_t value[BLOCK_SIZE];
	uint8_t dirty;
} CacheLine;

typedef struct {
	unsigned long tag;
	unsigned long line;
	unsigned long offset;
} Address;

ifstream infile;
ofstream outfile;
Address parsedAddr;
int curOp;
int curVal;
unsigned long curAddr;
CacheLine CACHE[LINE_CT];
uint8_t RAM[RAM_BYTES];

// Output CACHE for debugging
void dump ()
{
	static int count = 0;
	count ++;
	ofstream dumpCache("dump-cache.txt", ofstream::app);
	dumpCache << "===========" << dec << count << "===========" << endl;
	for (int i = 0; i < LINE_CT; i++) {
		dumpCache << setw(3) << dec << i << ' ' << hex << setw(3) << i << ' ';
		for (int j = BLOCK_SIZE-1; j >= 0; j--)
			dumpCache << setw(2) << setfill('0') << (int) CACHE[i].value[j];
		dumpCache << ' ' << (int) CACHE[i].dirty << ' ' << (int) CACHE[i].tag << endl;
	}
	dumpCache << endl;
	dumpCache.close();
}

void dumpRam ()
{
	static int count = 0;
	count ++;
	ofstream dumpRam("dump-ram.txt", ofstream::app);
	dumpRam << "===========" << dec << count << "===========" << endl;
	for (int i = 0; i < RAM_BYTES; i++) {
		// print address
		for (int j = BLOCK_SIZE-1; j >= 0; j--)
			if (RAM[i+j]) {
				dumpRam << setw(8) << dec << i << ' ' << hex << setw(4) << i << ' ';
				for (int k = BLOCK_SIZE-1; k >= 0; k--) 
					dumpRam << setw(2) << setfill('0') << (int) RAM[i+k];
				dumpRam << endl;
				break;
			}
		i += 7;
	}
	dumpRam << endl;
	dumpRam.close();
}

// Address is 16 bit
// 8 addressable bytes per block takes 3 bits
// 512/8 lines in cache = 2^9-2^3 = 2^6 takes 6 bits
Address parseAddr (unsigned long address)
{
	Address output;
	output.offset = address & 7; 		 // mask 3 bits
	output.line   = (address >> 3) & 63; // shift 3 bits, mask 6 bits
	output.tag    = address >> 9; 		 // shift 9 bits
	return output;
}

int writebackAndLoadIfNecessary ()
{
	int hit = 1;
	if (CACHE[parsedAddr.line].tag != parsedAddr.tag) {
		hit = 0;
		// writeback entire block if dirty
		if (CACHE[parsedAddr.line].dirty) {
			unsigned long startOfPrevBlock = (CACHE[parsedAddr.line].tag << 9) | (parsedAddr.line << 3);
			for (int i = 0; i < BLOCK_SIZE; i++)
				RAM[startOfPrevBlock + i] = CACHE[parsedAddr.line].value[i];
			// clear dirty bit
			CACHE[parsedAddr.line].dirty = 0;
		}
		// read from RAM to CACHE
		unsigned long startOfCurrBlock = curAddr & ~7; // umask 3 bits
		for (int i = BLOCK_SIZE-1; i >= 0; i--) {
			CACHE[parsedAddr.line].value[i] = RAM[startOfCurrBlock + i];
		}
		// set tag in CACHE
		CACHE[parsedAddr.line].tag = parsedAddr.tag;
	}
	return hit;
}

// Set value in CACHE
void write ()
{
	writebackAndLoadIfNecessary();
	// Set value in CACHE
	CACHE[parsedAddr.line].tag = parsedAddr.tag;
	CACHE[parsedAddr.line].value[parsedAddr.offset] = curVal;
	CACHE[parsedAddr.line].dirty = 1;
	// DEBUG
	#ifdef DEBUG
	cout << hex << (int) (curAddr & ~7) << '\t';
	for (int i=BLOCK_SIZE-1; i >=0 ; i--)
		cout << hex << setw(2) << setfill('0') << (int) CACHE[parsedAddr.line].value[i];
	cout << endl;
	#endif
}

// Load from RAM to CACHE; writeback if necessary
void read ()
{
	#ifdef DEBUG
	dump();
	dumpRam();
	#endif
	outfile << hex << uppercase << setw(4) << setfill('0') << (int) curAddr << ' ';
	// save dirtybit before eviction (to output later)
	int wasDirty = CACHE[parsedAddr.line].dirty;
	// writeback if necessary
	int hit = writebackAndLoadIfNecessary();
	// output block contents
	unsigned long startOfBlock = curAddr & ~7; // umask 3 bits
	for (int i = BLOCK_SIZE-1; i >= 0; i--) {
		outfile << setw(2) << setfill('0') << uppercase << (int) CACHE[parsedAddr.line].value[i];
	}
	// finish output: hit, dirty bit 
	outfile << ' ' << (int) hit << ' ' << (int) wasDirty << endl;
}

int main (int argc, char* args[])
{
	unsigned long addressInt;
	infile.open(args[1]);
	outfile.open("dm-out.txt");
	while (!infile.eof())
	{
		// Read line
		infile >> hex >> curAddr;
		parsedAddr = parseAddr(curAddr);
		infile >> hex >> curOp;
		infile >> hex >> curVal;
		if (infile.eof()) break;
		// Perform op
		if (curOp)
			write();
		else
			read();
		/* DEBUG */
		#ifdef DEBUG
		// cout << hex << parsedAddr.tag << ' ' << parsedAddr.line << endl;
		#endif
	}
	outfile.close();
	infile.close();
	return 0;
}