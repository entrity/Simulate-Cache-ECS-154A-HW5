/*
Simulator for a Virtual Memory system:

page table has 16 lines = 2^4
pages are 4KB = 2^12
	...so 12 bits (LSB) of address are the byte offset for the page

physical address space of 1 GB = 2^30
virtual address space of 4 GB = 2^32

*/

#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;

#define PAGE_TABLE_LINES 16
#define RAM_FRAME_CT 4

ifstream input;
ofstream output;

class PageTableEntry {
public:
	PageTableEntry () {}
	unsigned long address;
	int frameNumber;
	int present;
};

PageTableEntry pageTable[PAGE_TABLE_LINES];

class RamEntry {
public:
	RamEntry () {}
	unsigned long pageAddress;
	unsigned short use;
};

RamEntry RAM[RAM_FRAME_CT];

int cursor = 0; // ram cursor for clock algorithm

// Find pageTable index of a particular address
int find (unsigned long address)
{
	unsigned long addressPrefix = address & ~0xfff; // mask 12 bits. This is what the pageTable holds
	for (int i = 0; i < PAGE_TABLE_LINES; i++)
		if (pageTable[i].address == addressPrefix)
			return i;
	return -1;
}

// 
void accessPageTable (unsigned long address)
{
	unsigned long frameNumber;
	int pageNumber = find(address);
	// check if page is in pageTable
	if (pageTable[pageNumber].present)
	{
		// set use bit for given frame
		frameNumber = pageTable[pageNumber].frameNumber;
		RAM[frameNumber].use = 1;
	}
	else
	{
		// find frame to replace, using clock algorithm
		unsigned long oldPageNumber;
		while (1)
		{
			frameNumber = cursor % RAM_FRAME_CT;
			cursor ++;
			RAM[frameNumber].use = !RAM[frameNumber].use;
			if (RAM[frameNumber].use) // if use bit was 0 before toggle
			{
				// clear old value in pageTable
				oldPageNumber = find(RAM[frameNumber].pageAddress);
				pageTable[oldPageNumber].present = 0;
				#ifdef DEBUG
				output << "\tframe number : " << frameNumber << endl;
				output << "\tnpage number : " << pageNumber << endl;
				output << "\topage number : " << oldPageNumber << endl;
				#endif
				// update RAM
				RAM[frameNumber].pageAddress = (address & ~0xfff);
				// update frameNumber and present for new loaded entry in pageTable
				pageTable[pageNumber].frameNumber = frameNumber;
				pageTable[pageNumber].present = 1;
				break;
			}
			// increment cursor
		}
	}
	// debug
	#ifdef DEBUG
	output << hex << pageNumber << endl;
	#endif
}

// Output RAM frames
void dumpRAM ()
{
	for (int i = 0; i < RAM_FRAME_CT; i++)
		if (RAM[i].pageAddress)
		{
			if (i) output << ' ';
			output << hex << RAM[i].pageAddress;
		}
	output << endl;
}

// Output for debugging
void dumpPageTable ()
{
	for (int i = 0; i < PAGE_TABLE_LINES; i++)
	{
		output << hex << pageTable[i].address << endl;
	}
}

// Read input file into pageTable
void init ()
{
	unsigned long address;
	for (int i = 0; i < PAGE_TABLE_LINES; i++)
	{
		input >> hex >> address;
		pageTable[i].address = address;
	}
}

int main (int argc, char* args[])
{
	// validate args
	if (argc < 2)
	{
		cout << "Usage: " << args[0] << " <inputfile>" << endl;
		return 1;
	}
	// init
	input.open(args[1]);
	output.open("vm-out.txt");
	init();
	#ifdef DEBUG
	dump();
	#endif
	// process input (after first 16 lines)
	unsigned long address;
	while(!input.eof())
	{
		input >> hex >> address;
		if (input.eof()) break;
		// interact with page table, loading page if necessary
		accessPageTable(address);
		// output RAM
		dumpRAM();
	}
	// cleanup
	output.close();
	input.close();
	return 0;
}