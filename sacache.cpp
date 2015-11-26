/*
Simulation of a 4-way set-associative cache.

Simulated RAM has 16-bit addresses and is byte addressable. Though addresses
are byte addressable, blocks are always read from and written to RAM with
addresses that are multiples of eight, e.g., 8, 56, and 416. Cache block size
is 8 bytes; total capacity is 512 bytes

This program takes as input a filename from the command line. The file
specified by the filename must be an ASCII file with each line in the
following 4-byte format:

	Bytes 	Function
	1-2 	16 bit address
	3 		Read/Write
	4 		8 bits of Data

The read function will be designated by all 0's and the write will be
designated by all 1's. Upon a read operation the data segment of the 4-byte
format will be ignored. However when a write occurs the data will be written
into the specified byte. For ease of creation the input file will be in hex.

The set-associative cache produces as output a file named sa-out.txt. Each line
of the output file corresponds to the results of each read operation from the
input file. The information on each line will be address specified in the
input line, the 8 bytes of data displayed with the lowest addressed byte on
the right, a HIT output indicating whether the requested item was found in the
cache (0 for miss, 1 for hit), and the dirty-bit. Note that the dirty bit will
display as 1 if the original dirty bit was set, even if there is a read miss
that will cause the dirty bit to be cleared. These four pieces of information
are separated by a space.

An important thing to notice is that when a line gets evicted from the cache,
and at some later point is brought back into the cache by a subsequent read,
the read must return the correct value (not just zero), as if it was stored in
RAM when it was evicted from the cache. A specific example of this is line 9
in dm-test-output.txt. You can implement this however you like, but a
perfectly acceptable way to do it is to have an array of length 2^16 chars to
act as RAM that cache lines get evicted to. Initialize the contents of your
cache and memory to all 0's.

*/

/*
Address format:
- 64 cache lines = 2^
- 4 ways
- 16 sets = 64 / 4 = 2^4
| tag    | set    | offset |
| 9 bits | 4 bits | 3 bits |
| 15-7   | 6-3    | 2-0    |
*/
#define DEBUG
#include <iostream>
#include <fstream>
#include <iomanip>
#include <bitset>
using namespace std;

#define BLOCK_BYTES 8
#define CACHE_BYTES 512
#define CACHE_LINES CACHE_BYTES / BLOCK_BYTES
#define CACHE_WAYS  4
#define RAM_BYTES   0x10000 // 2^16 b/c of a 16-bit address for byte-addressable RAM   
#define RAM_BLOCKS  RAM_BYTES / 8
#define ADDRESS_OFFSET_BITS 3
#define ADDRESS_SET_BITS 4 // number of address bits used to designate cache line
#define MASK_4_BITS 15
#define MASK_3_BITS 7


typedef unsigned char byte;

byte RAM[RAM_BYTES];

ofstream output;

class Address
{
public:
	Address (int tag, int set) {
		this->tag = tag;
		this->set = set;
		this->offset = 0;
		this->value = (tag << (ADDRESS_OFFSET_BITS + ADDRESS_SET_BITS)) | (set << ADDRESS_OFFSET_BITS);
	}
	Address (unsigned short value) {
		this->value = value;
		tag = (value >> ADDRESS_OFFSET_BITS + ADDRESS_SET_BITS);
		set = (value >> ADDRESS_OFFSET_BITS) & MASK_4_BITS;
		offset = value & MASK_3_BITS;
	}
	unsigned short getTag ()
	{
		return tag;
	}
	unsigned short getSet ()
	{
		return set;
	}
	unsigned short getOffset ()
	{
		return offset;
	}
	// Return address for start of RAM block (i.e. clear 3 LSB)
	unsigned short ramBlock ()
	{
		return value & ~MASK_3_BITS;
	}
	unsigned short get ()
	{
		return value;
	}
private:
	unsigned short value;
	unsigned short tag;
	unsigned short set;
	unsigned short offset;
};

namespace CacheOps
{
	void dump();

	class CacheLine {
	public:
		byte read (int offset) {
			return value[offset];
		}

		void output(int index, ostream &out)
		{
			out << uppercase << hex << ' ' << setfill(' ') << setw(3) << (int) index << " tag " << setfill('0') << setw(3) << (int) tag << ' ';
			for (int i = BLOCK_BYTES-1; i >= 0; i--)
				out << setw(2) << uppercase << hex << (int) value[i];
			out << ' ' << setw(1) << dirty
			<< ", LRU counter " << dec << counter << hex
			<< endl;
		}

		byte value[BLOCK_BYTES];
		bool dirty;
		unsigned int counter; // used for LRU
		byte tag; // actually using 4 bits (LSB)
	};

	CacheLine CACHE[CACHE_LINES];

	// File output is: Address Block Hit Dirty(was)
	CacheLine * getLine(Address &address, bool isRead)
	{
		// init
		CacheLine * p_line = NULL;
		CacheLine * p_set = &CACHE[address.getSet()];
		CacheLine * p_lru  = p_set;
		int cacheHit;
		int wasDirty;
		// look for sought line in set
		for (int i = 0; i < CACHE_WAYS; i++)
		{
			// increment counter for line
			p_set[i].counter ++;
			// seek line with matching tag (hit)
			if (p_set[i].tag == address.getTag()) {
				p_line = &p_set[i];
			}
			// seek LRU line (in case of miss) : line whose counter is greatest
			if (p_set[i].counter > p_lru->counter){
				p_lru = &p_set[i];
			}
		}
		// output for READ
		if (isRead) output << uppercase << hex << setw(4) << (int) address.get() << ' ';
		cacheHit = p_line ? 1 : 0;
		if (!cacheHit) p_line = p_lru;
		wasDirty = p_line->dirty ? 1 : 0;
		p_line->counter = 0;
		#ifdef DEBUG
		cout << "\t" << (cacheHit ? "Hit" : "Miss");
		#endif
		cout << endl;
		// handle cache miss
		if (!cacheHit)
		{
			// write-back if dirty
			if (wasDirty) {
				Address writebackAddress(p_line->tag, address.getSet());
				for (int i = 0; i < BLOCK_BYTES; i++)
					RAM[i + writebackAddress.get()] = p_line->value[i];
				p_line->dirty = false;
			}
			// update tag
			p_line->tag = address.getTag();
			// update value
			for (int i = 0; i < BLOCK_BYTES; i++)
				p_line->value[i] = RAM[address.ramBlock() + i];
		}
		// output for READ
		if (isRead)
		{	
			// output value
			for (int i = BLOCK_BYTES-1; i >= 0; i--)
				output << setfill('0') << setw(2) << hex << (int) p_line->value[i];
			// output hit and dirty
			output << ' ' << setw(1) << (int) cacheHit << ' ' << setw(1) << (int) wasDirty;
			#ifdef DEBUG
			// output line number
			// output << ' ' << " - set]" << (int) address.getSet();
			#endif
			output << endl;
		}
		return p_line;
	}

	// Read from RAM to Cache
	// Evict if necessary
	byte read (Address &address)
	{
		return getLine(address, true)->read(address.getOffset());
	}

	// Change the value of a byte in a cache line, but do not writethrough new value to RAM
	void write (Address &address, byte value)
	{
		CacheLine * p_line = getLine(address, false);
		p_line->value[address.getOffset()] = value;
		p_line->tag = address.getTag();
		p_line->dirty = true;
	}

	void dump ()
	{
		for (int j = 0; j < CACHE_LINES; j++)
				CACHE[j].output(j, cout);
	}

	void dump (int lines)
	{
		for (int j = 0; j < lines; j++)
				CACHE[j].output(j, cout);
	}

}

int main (int argc, char* args[]) {
	if (argc < 2)
	{
		cout << "Usage:\t" << args[0] << " <filename>" << endl;
		return 1;
	}

	unsigned short addressInt;
	unsigned short opcode;
	unsigned short value;
	int count = 0;

	// open input file
	ifstream input(args[1]);
	if (input)
	{
		output.open("sa-out.txt");
		if (output)
		{
			while (!input.eof())
			{
				input >> std::hex >> addressInt;
				input >> std::hex >> opcode;
				input >> std::hex >> value;
				count ++;
				Address address(addressInt);
				#ifdef DEBUG
				cout << hex << uppercase << addressInt
					<< " " << setfill('0') << setw(2) << opcode << " " << setw(2) << value
					<< "\t"
					<< " tag " << (int) address.getTag()
					<< " set " << (int) address.getSet()
					<< " offset " << (int) address.getOffset()
					;
				#endif
				if (opcode) // write
					CacheOps::write(address, value);
				else // read
					CacheOps::read(address);
				CacheOps::dump(4);
				cin.get();
			}
			output.close();
		}
		else
		{
			cout << "Could not open output file" << endl;
			return 3;
		}
		input.close();
	}
	else
	{
		cout << "File not found" << endl;
		return 2;
	}	

}
