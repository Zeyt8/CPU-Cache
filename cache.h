#pragma once

#include <iostream>

namespace Tmpl8 {

// single-level fully associative cache

#define DRAMSIZE		3276800				// 3.125MB; 1024x800 pixels

struct CacheLine
{
	CacheLine() = default;
	CacheLine(int lineWidth) : bytes(new uchar[lineWidth]), tag(0), dirty(false), lineWidth(lineWidth) {}
	CacheLine(const CacheLine& other) : bytes(new uchar[other.lineWidth]), tag(other.tag), dirty(other.dirty), lineWidth(other.lineWidth)
	{
		memcpy(bytes, other.bytes, lineWidth);
	}
	~CacheLine() { delete[] bytes; }
	CacheLine& operator=(const CacheLine& other)
	{
		if (this != &other)
		{
			delete[] bytes;
			lineWidth = other.lineWidth;
			bytes = new uchar[lineWidth];
			memcpy(bytes, other.bytes, lineWidth);
			tag = other.tag;
			dirty = other.dirty;
		}
		return *this;
	}
	uchar* bytes = nullptr;
	uint tag = 0;
	bool dirty = false;
	int lineWidth = 0;
};

class Level // abstract base class for a level in the memory hierarchy
{
public:
	virtual void WriteLine( uint address, const CacheLine& line ) = 0;
	virtual CacheLine ReadLine( uint address ) = 0;
	Level* nextLevel = 0;
	uint r_hit = 0, r_miss = 0, w_hit = 0, w_miss = 0;
	int lineWidth = 0;
	int size = 0;
};

class Cache : public Level // cache level for the memory hierarchy
{
public:
	Cache(int size, int lineWidth, int setSize)
	{
		this->size = size;
		this->lineWidth = lineWidth;
		this->setSize = setSize;
		numSets = (size / lineWidth) / setSize;
		slot = new CacheLine*[numSets];
		for (int i = 0; i < numSets; i++)
		{
			slot[i] = new CacheLine[setSize];
			for (int j = 0; j < setSize; j++)
			{
				slot[i][j] = CacheLine(lineWidth);
			}
		}
	}
	~Cache()
	{
		for (int i = 0; i < numSets; i++)
		{
			delete[] slot[i];
		}
		delete[] slot;
	}
	void WriteLine( uint address, const CacheLine& line );
	CacheLine ReadLine( uint address );
	CacheLine& backdoor(int set, int i) { return slot[set][i]; } /* for visualization without side effects */
	int numSets = 0;
	int setSize = 0;
private:
	CacheLine** slot = nullptr;

	int getSetIndex(uint address) const
	{
		return (address / lineWidth) % numSets;
	}
	uint getTag(uint address) const
	{
		return (address / lineWidth) / numSets;
	}
};

class Memory : public Level // DRAM level for the memory hierarchy
{
public:
	Memory(int lineWidth)
	{
		this->lineWidth = lineWidth;
		mem = new uchar[DRAMSIZE];
		memset( mem, 0, DRAMSIZE ); 
	}
	~Memory() { delete[] mem; }
	void WriteLine( uint address, const CacheLine& line );
	CacheLine ReadLine( uint address );
	uchar* backdoor() { return mem; } /* for visualization without side effects */
private:
	uchar* mem = 0;
};

class MemHierarchy // memory hierarchy
{
public:
	MemHierarchy()
	{
		l1 = new Cache(4096, 64, 64);
		l1->nextLevel = l2 = new Cache(4096*2, 64, 64 * 2);
		l2->nextLevel = l3 = new Cache(4096*4, 64, 64 * 4);
		l3->nextLevel = memory = new Memory(64);
	}
	void WriteByte( uint address, uchar value );
	uchar ReadByte( uint address );
	void WriteUint( uint address, uint value );
	uint ReadUint( uint address );
	void ResetCounters()
	{
		l1->r_hit = l1->w_hit = l1->r_miss = l1->w_miss = 0;
		l2->r_hit = l2->w_hit = l2->r_miss = l2->w_miss = 0;
		l3->r_hit = l3->w_hit = l3->r_miss = l3->w_miss = 0;
		memory->r_hit = memory->w_hit = memory->r_miss = memory->w_miss = 0;
	}
	Level* l1, *l2, *l3, *memory; 
};

} // namespace Tmpl8