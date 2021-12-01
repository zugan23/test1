#ifndef NUM_SERV
#define NUM_SERV
#include <stdint.h>
#include "ID3v2.h"

void swapBytes(uint32_t* n);
void safeIntDecode(uint32_t* n);
void safeIntEncode(uint32_t* n);

void setUTF8(Frame* frame);
uint8_t getDataOffset(Frame* frame);

void updateTagTotalSize(ID3v2_Tag* tag);

Frame* getFrameByID(ID3v2_Tag* tag, const uint8_t* frameID);

void freeMemory(ID3v2_Tag* tag);

void clearFileFromTag(FILE* file, ID3v2_Tag* tag);

void writeTagIntoFileStart(FILE* file, ID3v2_Tag* tag);

uint8_t* getBytesOfuint32_t(uint32_t n);

char* getFrameDataAsString(Frame* frame);

int isFrameIdCorrect(const uint8_t* id);

#endif // !NUM_SERV

