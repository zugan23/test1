#ifndef ID3_HEADER
#define ID3_HEADER

#include <stdio.h>
#include <stdint.h>

//ID3 header struct

typedef struct
{
	uint8_t signature[4];
	uint8_t version[2]; // 0 - major, 1 - revision number
	uint8_t flagData;
	uint32_t size;

} ID3header;
ID3header* getID3Header(FILE* file);

//frame header
typedef union
{
	struct //%0abc0000
	{
		unsigned int tagAlterPreservation : 1;// a
		unsigned int fileAlterPreservation : 1;// b
		unsigned int readOnly : 1;// c
	};
	uint8_t asByte;
}FrameStatusFlags;

typedef union
{
	struct // %0h00kmnp
	{
		unsigned int groupingIdentity : 1;// h
		unsigned int compression : 1;// k
		unsigned int encryption : 1;// m
		unsigned int unsynchronisation : 1;// n
		unsigned int dataLengthIndicator : 1;// p
	};
	uint8_t asByte;
} FrameFormatFlags;

typedef struct
{
	uint8_t frameID[5];
	uint32_t dataSize;
	FrameStatusFlags fs;
	FrameFormatFlags ff;

} FrameHeader;

typedef enum
{
	ISO_8859_1,
	UTF_16,
	UTF_16BE,
	UTF_8
} Encoding;

typedef struct
{
	FrameHeader* frameHeader;
	Encoding encoding;
	uint32_t dataOffset;
	uint8_t* data;
	uint8_t mayBeShown;
} Frame;

FrameHeader* getFrameHeader(FILE* file, ID3header* header);
uint8_t* getFrameData(FILE* file, FrameHeader* header);
Frame* getFrame(FILE* file, ID3header* header);

//ID3v2 tag
typedef struct
{
	ID3header* header;
	Frame** frames;
	uint32_t numOfFrames;
	uint32_t totalSize;
} ID3v2_Tag;

ID3v2_Tag* getTag(FILE* file);

int changeFrameData(ID3v2_Tag* tag, const uint8_t* frameID, uint8_t* newData, uint32_t newDataSize);


#endif 

