#include "ID3v2.h"
#include "service.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ID3header* getID3Header(FILE* file)
{
    ID3header* header = (ID3header*)malloc(sizeof(ID3header));//10 bytes
    uint8_t* data = (uint8_t*)malloc(sizeof(uint8_t) * 10);
    fread(data, sizeof(uint8_t) * 10, 1, file);

    for (int i = 0; i < 3; i++)
    {
        header->signature[i] = data[i];
    }
    header->signature[3] = '\0';

    if (strcmp(header->signature, "ID3"))
    {
        free(data);
        free(header);
        return NULL;
    }

    header->version[0] = data[3];
    header->version[1] = data[4];

    header->flagData = data[5];

    header->size = *(uint32_t*)&data[6];
    swapBytes(&header->size);

    if (header->version[0] == 4)
    {
        safeIntDecode(&header->size);
    }

    free(data);
    return header;
}

FrameHeader* getFrameHeader(FILE* file, ID3header* header)
{
    FrameHeader* fh = (FrameHeader*)malloc(sizeof(FrameHeader));
    uint8_t* data = (uint8_t*)malloc(sizeof(uint8_t) * 10);
    fread(data, 1, sizeof(uint8_t) * 10, file);
    short correct = 1;

    for (int i = 0; i < 4; i++)
    {
        fh->frameID[i] = data[i];
    }
    fh->frameID[4] = '\0';

    correct = isFrameIdCorrect(fh->frameID);
    
    if (!correct)
    {
        free(data);
        return NULL;
    }

    fh->dataSize = *(uint32_t*)&data[4];

    swapBytes(&fh->dataSize);
    if (header->version[0] == 4)
    {
        safeIntDecode(&fh->dataSize);
    }

    //frame status flags %0abc0000 - ID3v2.4.0
    //frame status flags %abc00000 - ID3v2.3.0 sh#t
    uint8_t offset = (header->version[0] == 3 ? 7 : 6);
    fh->fs.tagAlterPreservation =  data[8] & (1 << offset);
    fh->fs.fileAlterPreservation = data[8] & (1 << (offset-1));
    fh->fs.readOnly =              data[8] & (1 << (offset-2));

    //frame format flags %0h00kmnp - ID3v2.4.0
    fh->ff.groupingIdentity =    data[9] & (1 << 6);
    fh->ff.compression =         data[9] & (1 << 3);
    fh->ff.encryption =          data[9] & (1 << 2);
    fh->ff.unsynchronisation =   data[9] & (1 << 1);
    fh->ff.dataLengthIndicator = data[9] & (1 << 0);
    
    free(data);
    return fh;
}

uint8_t* getFrameData(FILE* file, FrameHeader* header)
{
    uint8_t* data = (uint8_t*)malloc(header->dataSize);
    fread(data, 1, header->dataSize, file);
    return data;
}

Frame* getFrame(FILE* file, ID3header* header)
{
    Frame* frame = (Frame*)malloc(sizeof(Frame));

    frame->frameHeader = getFrameHeader(file, header);
    if (frame->frameHeader == NULL)
    {
        return NULL;
    }
    frame->data = getFrameData(file, frame->frameHeader);
    frame->encoding = (Encoding)frame->data[0];
    frame->mayBeShown = 1;
    frame->dataOffset = getDataOffset(frame);

    if (frame->encoding == UTF_16 || frame->encoding == UTF_16BE)
    {
        setUTF8(frame);
    }

    return frame;
}

ID3v2_Tag* getTag(FILE* file)
{
    ID3v2_Tag* tag = (ID3v2_Tag*)malloc(sizeof(ID3v2_Tag));
    tag->header = getID3Header(file);

    if (tag->header == NULL)
    {
        printf("asdf\n");
        return NULL;
    }

    uint32_t possibleFramesNumber = ((tag->header->size) / 10) * sizeof(Frame);
    Frame** frames = (Frame**)malloc(possibleFramesNumber * sizeof(Frame*));

    uint32_t bytesRead = 0;
    uint32_t totalFrames = 0;
    uint32_t i = 0;
    while (bytesRead < tag->header->size && i < possibleFramesNumber)
    {
        Frame* frame = getFrame(file, tag->header);
        uint8_t adding = 1;
        if (frame != NULL)
        {
            for (int j = 0; j < totalFrames; j++)
            {
                if (!strcmp(frame->frameHeader->frameID, frames[j]->frameHeader->frameID))
                {
                    if (!strcmp(frame->frameHeader->frameID, "COMM"))
                    {
                        break;
                    }
                    adding = 0;
                    free(frame);
                    break;
                }
            }
        }
        else
        {
            frames[i] = NULL;
            break;
        }

        if (adding)
        {
            frames[i] = frame;

            bytesRead += frames[i]->frameHeader->dataSize + 10;

            totalFrames++;
            i++;
        }
    }

    tag->frames = (Frame**)malloc(totalFrames * sizeof(Frame*));
    memcpy(tag->frames, frames, totalFrames*sizeof(Frame*));

    tag->numOfFrames = totalFrames;

    updateTagTotalSize(tag);

    free(frames);
    return tag;
}

int changeFrameData(ID3v2_Tag* tag, const uint8_t* frameID,  uint8_t* newData, uint32_t newDataSize)
{
    Frame* frame = getFrameByID(tag, frameID);
    if (frame == NULL)
    {
        return 0;
    }

    uint8_t* buf = (uint8_t*)malloc((newDataSize + frame->dataOffset) * (sizeof(uint8_t)));
    memcpy(buf, frame->data, frame->dataOffset);
    for (int i = frame->dataOffset; i < frame->dataOffset + newDataSize; i++)
    {
        buf[i] = newData[i - frame->dataOffset];
    }
    free(frame->data);
    frame->data = buf;
    frame->frameHeader->dataSize = frame->dataOffset + newDataSize;

    updateTagTotalSize(tag);

    return 1;
}
