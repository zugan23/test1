#include "service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void swapBytes(uint32_t* n)
{
	uint32_t result = 0;

	result |= (*n & 0x000000FF) << 24;
	result |= (*n & 0x0000FF00) << 8;
	result |= (*n & 0x00FF0000) >> 8;
	result |= (*n & 0xFF000000) >> 24;

	*n = result;
}

void safeIntDecode(uint32_t* n)
{
	uint32_t result = 0;

	result |= (*n & 0x000000FF) >> 0;
	result |= (*n & 0x0000FF00) >> 1;
	result |= (*n & 0x00FF0000) >> 2;
	result |= (*n & 0xFF000000) >> 3;

	*n = result;
}

void safeIntEncode(uint32_t* n)
{
	uint32_t answer = 0;

	answer |= ((*n & 0x0000007F) << 0);
	answer |= ((*n & (0x7F << 7)) << 1);
	answer |= ((*n & (0x7F << 14)) << 2);
	answer |= ((*n & (0x7F << 21)) << 3);

	*n = answer;
}

void setUTF8(Frame* frame)
{
	uint32_t newDataSize = frame->dataOffset;
	uint16_t symbUTF16;
	uint8_t unicodeDirSymbPresented = 0;
	uint16_t unicodeDirSymb = 0xFEFF;
	for (int i = frame->dataOffset; i < frame->frameHeader->dataSize; i += 2)
	{
		symbUTF16 = *(uint16_t*)&(frame->data[i]);
		if (symbUTF16 != 0xFEFF && symbUTF16 != 0xFFFE)
		{
			newDataSize += 1;
		} 
		else
		{
			unicodeDirSymbPresented = 1;
			unicodeDirSymb = symbUTF16;
		}
	}

	uint8_t* newData = (uint8_t*)malloc(sizeof(uint8_t) * newDataSize);
	for (int i = 0; i < frame->dataOffset; i++)
	{
		newData[i] = frame->data[i];
	}

	int j = frame->dataOffset;
	int i = frame->dataOffset;
	while (j < newDataSize && i < frame->frameHeader->dataSize)
	{
		symbUTF16 = *(uint16_t*)&(frame->data[i]);
		if (symbUTF16 != 0xFFFE && symbUTF16 != 0xFEFF)
		{
			if (unicodeDirSymbPresented == 1 && unicodeDirSymb == 0xFFFE)
			{
				symbUTF16 >>= 8;
			}
			newData[j] = symbUTF16;
			if (newData[j] > 0x7F)
			{
				frame->mayBeShown = 0;
			}
			j++;
		}
		i += 2;
	}

	if (frame->mayBeShown == 1)
	{
		free(frame->data);
		frame->data = newData;
		frame->frameHeader->dataSize = newDataSize;
		frame->data[0] = UTF_8;
		frame->encoding = UTF_8;
	}
}

uint8_t getDataOffset(Frame* frame)
{
	uint8_t dataOffset = 0;

	if (!strcmp(frame->frameHeader->frameID, "USLT")
		|| !strcmp(frame->frameHeader->frameID, "COMM")
		|| !strcmp(frame->frameHeader->frameID, "TXXX"))
	{
		dataOffset = (!strcmp(frame->frameHeader->frameID, "TXXX") ? 1 : 4);
		if (!strcmp(frame->frameHeader->frameID, "TXXX"))
		{
			dataOffset += 3;
		}
		if (frame->encoding != UTF_16 && frame->encoding != UTF_16BE)
		{
			for (int i = dataOffset; frame->data[i] != '\0'; i++)
			{
				dataOffset++;
			}
			dataOffset++;
		}
		else
		{
			uint16_t symbol2bytes;
			for (int i = dataOffset; ; i += 2)
			{
				symbol2bytes = *(uint16_t*)&frame->data[i];
				if (symbol2bytes != 0 && symbol2bytes!=0xFFFE && symbol2bytes != 0xFEFF)
				{
					dataOffset += 2;
				}
				else break;
			}
		}
	}
	else if (!strcmp(frame->frameHeader->frameID, "UFID"))
	{
		for (int i = dataOffset; frame->data[i] != 0; i++)
		{
			dataOffset++;
		}
		dataOffset++;

	}
	else if (!strcmp(frame->frameHeader->frameID, "SYLT"))
	{
		dataOffset = 6;
	}
	else if (!strcmp(frame->frameHeader->frameID, "USER"))
	{
		dataOffset = 4;
	}
	else if (frame->frameHeader->frameID[0] == 'T') //text frames
	{
		dataOffset = 1;
	}
	else
	{
		frame->mayBeShown = 0;
	}

	return dataOffset;
}

void updateTagTotalSize(ID3v2_Tag* tag)
{
	tag->totalSize = 10;//header

	for (int i = 0; i < tag->numOfFrames; i++)
	{
		tag->totalSize += 10 + tag->frames[i]->frameHeader->dataSize;
	}
}

Frame* getFrameByID(ID3v2_Tag* tag, const uint8_t* frameID)
{
	Frame* frame = NULL;
	for (int i = 0; i < tag->numOfFrames; i++)
	{
		if (!strcmp(tag->frames[i]->frameHeader->frameID, frameID))
		{
			frame = tag->frames[i];
			break;
		}
	}
	return frame;
}

void freeMemory(ID3v2_Tag* tag)
{
	free(tag->header);
	for (int i = 0; i < tag->numOfFrames; i++)
	{
		free(tag->frames[i]->frameHeader);
		free(tag->frames[i]->data);
		free(tag->frames[i]);
	}
	free(tag->frames);
	free(tag);
}

void clearFileFromTag(FILE* file, ID3v2_Tag* tag)
{
	fseek(file, 0, SEEK_SET);
	for (int i = 0; i < tag->header->size; i++)
	{
		fputc(0, file);
	}
}

void writeTagIntoFileStart(FILE* file, ID3v2_Tag* tag)
{
	fseek(file, 0, SEEK_SET);
	
	uint8_t* bufferData = (uint8_t*)malloc(10 * sizeof(uint8_t));
	uint32_t temp;
	uint8_t* bytes;

	//writing tag header
	for (int i = 0; i < 3; i++)
	{
		bufferData[i] = tag->header->signature[i];
	}
	
	bufferData[3] = tag->header->version[0];
	bufferData[4] = tag->header->version[1];
	bufferData[5] = tag->header->flagData;
	//fwrite(bufferData, 1, 6, file);

	temp = tag->header->size;
	if (tag->header->version[0] == 4)
	{
		safeIntEncode(&temp);
	}
	swapBytes(&temp);
	bytes = getBytesOfuint32_t(temp);

	bufferData[6] = bytes[3];
	bufferData[7] = bytes[2];
	bufferData[8] = bytes[1];
	bufferData[9] = bytes[0];
	free(bytes);

	fwrite(bufferData, sizeof(uint8_t), 10, file);
	
	for (int i = 0; i < tag->numOfFrames; i++)
	{
		Frame* curFrame = tag->frames[i];
		//writing frame header
		for (int j = 0; j < 4; j++)
		{
			bufferData[j] = curFrame->frameHeader->frameID[j];
		}
		temp = curFrame->frameHeader->dataSize;
		if (tag->header->version[0] == 4)
		{
			safeIntEncode(&temp);
		}
		swapBytes(&temp);

		bytes = getBytesOfuint32_t(temp);
		bufferData[4] = bytes[3];
		bufferData[5] = bytes[2];
		bufferData[6] = bytes[1];
		bufferData[7] = bytes[0];
		free(bytes);

		bufferData[8] = curFrame->frameHeader->fs.asByte;
		bufferData[9] = curFrame->frameHeader->ff.asByte;

		fwrite(bufferData, sizeof(uint8_t), 10, file);

		//writing frame data
		fwrite(curFrame->data, sizeof(uint8_t), curFrame->frameHeader->dataSize, file);
	}
	free(bufferData);
}

uint8_t* getBytesOfuint32_t(uint32_t n)
{
	uint8_t* bytes = (uint8_t*)malloc(4);
	bytes[0] = (n & (0xFF000000))>>24;
	bytes[1] = (n & (0x00FF0000))>>16;
	bytes[2] = (n & (0x0000FF00))>>8;
	bytes[3] = (n & (0x000000FF))>>0;
	return bytes;
}

char* getFrameDataAsString(Frame* frame)
{
	unsigned int strLen = (frame->frameHeader->dataSize) - (frame->dataOffset);
	char* str = (char*)malloc(sizeof(char) * (strLen + 1));
	int i = 0;
	int j = frame->dataOffset;
	while (i < strLen && j < frame->frameHeader->dataSize)
	{
		str[i++] = frame->data[j++];
	}
	str[strLen] = '\0';
	return str;
}

int isFrameIdCorrect(const uint8_t* id)
{
	int correct = 1;
	for (int i = 0; i < 4; i++)
	{
		if (id[i] > 'Z')
		{
			
		}
		if (id[i] < 'A' && !(id[i] >= '0' && id[i] <= '9'))
		{
			correct = 0;
			break;
		}
	}
	return correct;
}

