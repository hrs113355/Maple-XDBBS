#include "bbs.h"
#define ESCAPE_SLASH "#%slash%#"

char slashbuf[512];
char * escape_slash(const char * src)
{
    char *ptr;
    const char *copy;

    memset(slashbuf, 0, sizeof(slashbuf));
    copy = src;
    ptr = slashbuf;

    while (*copy)
    {
        if (*copy == '/')
        {
            strcat(ptr, ESCAPE_SLASH);
            ptr+= strlen(ESCAPE_SLASH);
        }
        else
            *ptr++ = *copy;

        copy++;
    }
    *ptr = '\0';
    return slashbuf;
}

int main(int argc, char *argv[])
{
    chdir(BBSHOME);

    FILE *fp = fopen(FN_RSS_LIST, "rb");
    rssfeedlist_t feedlist;
    char cmd[768]; 

    while (fread(&feedlist, sizeof(feedlist), 1, fp))
	if (*(feedlist.feedurl) && !strchr(feedlist.feedurl, '\"'))
	{
	    sprintf(cmd, "src/util/rss.php \"%s\" %s"
		, escape_slash(feedlist.feedurl), (argc >= 2 && !strcmp("DEBUG", argv[1])) ? "" : "> /dev/null");
	    system(cmd);
	}

    fclose(fp);
    return 0;
}
