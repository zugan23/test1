#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ID3v2.h"
#include "service.h"
#include "UIstuff.h"
#include "argparse.h"

int main(int argc, char **argv)
{

	args settings = getArgs(argc, argv);

	printf("%s\n", settings.fileName);

	ID3v2_Tag* tag = NULL;
	FILE* file = NULL;
	if ((file = fopen(settings.fileName, "r+b")) == NULL)
	{
		printf("  file not found\n");
		exit(1);
	}

	tag = getTag(file);
	
	if (tag == NULL || tag->header->version[0] < 3)
	{
		printf("  no mp3 tag or incorrect tag format\n");
		fclose(file);
		exit(1);
	}
	
		

	if (settings.show)
		printFrames(tag, "all");
	else if (settings.get)
		printFrames(tag, settings.tagName);
	else if (settings.set) {
		changeFrameData(tag, settings.tagName, settings.tagVal + 1, strlen(settings.tagVal + 1));
		writeTagIntoFileStart(file, tag);
		printf("  changes saved\n");
		freeMemory(tag);
		tag = NULL;
		fclose(file);
		printf("  file closed\n");
	}

	return 0;
}