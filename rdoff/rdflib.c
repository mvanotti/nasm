/* rdflib - manipulate RDOFF library files (.rdl) */

/*
 * an rdoff library is simply a sequence of RDOFF object files, each
 * preceded by the name of the module, an ASCII string of up to 255
 * characters, terminated by a zero. 
 *
 * There may be an optional
 * directory placed on the end of the file. The format of the
 * directory will be 'RDLDD' followed by a version number, followed by
 * the length of the directory, and then the directory, the format of
 * which has not yet been designed. The module name of the directory
 * must be '.dir'. 
 *
 * All module names beginning with '.' are reserved
 * for possible future extensions. The linker ignores all such modules,
 * assuming they have the format of a six byte type & version identifier
 * followed by long content size, followed by data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* functions supported:
     create a library	(no extra operands required)
     add a module from a library (requires filename and name to give mod.)
     remove a module from a library (requires given name) (not implemented)
     extract a module from the library (requires given name and filename)
     list modules */

const char *usage = 
   "usage:\n"
   "  rdflib x libname [extra operands]\n\n"
   "  where x is one of:\n"
   "    c - create library\n"
   "    a - add module (operands = filename module-name)\n"
   "    r - remove                (module-name) [not implemented]\n"
   "    x - extract               (module-name filename)\n"
   "    t - list\n";

char **_argv;

#define _ENDIANNESS 0		/* 0 for little, 1 for big */

static void longtolocal(long * l)
{
#if _ENDIANNESS
    unsigned char t;
    unsigned char * p = (unsigned char *) l;

    t = p[0];
    p[0] = p[3];
    p[3] = t;
    t = p[1];
    p[1] = p[2];
    p[2] = p[1];
#endif
}

char copybytes(FILE *fp, FILE *fp2, int n)
{
    int i, t = 0;

    for (i = 0 ; i < n; i++ )
    {
	t = fgetc(fp);
	if (t == EOF)
	{
	    fprintf(stderr,"rdflib: premature end of file in '%s'\n",
		    _argv[2]);
	    exit(1);
	}
	if (fp2) 
	    if (fputc(t, fp2) == EOF)
	    {
		fprintf(stderr,"rdflib: write error\n");
		exit(1);
	    }
    }
    return (char) t;	/* return last char read */
}

long copylong(FILE *fp, FILE *fp2)
{
    long l;
    int i,t;
    unsigned char * p = (unsigned char *) &l;


    for (i = 0 ; i < 4; i++ )	/* skip magic no */
    {
	t = fgetc(fp);
	if (t == EOF)
	{
	    fprintf(stderr,"rdflib: premature end of file in '%s'\n",
		    _argv[2]);
	    exit(1);
	}
	if (fp2) 
	    if (fputc(t, fp2) == EOF)
	    {
		fprintf(stderr,"rdflib: write error\n");
		exit(1);
	    }
	*p++ = t;
    }
    longtolocal (&l);
    return l;
}

int main(int argc, char **argv)
{
    FILE *fp, *fp2;
    char *p, buf[256], c;
    int i;
    long l;

    _argv = argv;

    if (argc < 3 || !strncmp(argv[1],"-h",2) || !strncmp(argv[1],"--h",3))
    {
	printf(usage);
	exit(1);
    }

    switch(argv[1][0])
    {
    case 'c':		/* create library */
	fp = fopen(argv[2],"wb");
	if (! fp) {
	    fprintf(stderr,"rdflib: could not open '%s'\n",argv[2]);
	    perror("rdflib");
	    exit(1);
	}
	fclose(fp);
	break;

    case 'a':		/* add module */
	if (argc < 5) {
	    fprintf(stderr,"rdflib: required parameter missing\n");
	    exit(1);
	}
	fp = fopen(argv[2],"ab");
	if (! fp)
	{
	    fprintf(stderr,"rdflib: could not open '%s'\n",argv[2]);
	    perror("rdflib");
	    exit(1);
	}
	
	fp2 = fopen(argv[3],"rb");
	if (! fp)
	{
	    fprintf(stderr,"rdflib: could not open '%s'\n",argv[3]);
	    perror("rdflib");
	    exit(1);
	}

	p = argv[4];
	do {
	    if ( fputc(*p,fp) == EOF ) {
		fprintf(stderr,"rdflib: write error\n");
		exit(1);
	    }
	} while (*p++);

	while (! feof (fp2) ) {
	    i = fgetc (fp2);
	    if (i == EOF) {
		break;
	    }

	    if ( fputc(i, fp) == EOF ) {
		fprintf(stderr,"rdflib: write error\n");
		exit(1);
	    }
	}
	fclose(fp2);
	fclose(fp);
	break;

    case 'x':
	if (argc < 5) {
	    fprintf(stderr,"rdflib: required parameter missing\n");
	    exit(1);
	}
    case 't':
	if (argc < 3) {
	    fprintf(stderr, "rdflib: required paramenter missing\n");
	    exit(1);
	}
	fp = fopen(argv[2],"rb");
	if (! fp)
	{
	    fprintf(stderr,"rdflib: could not open '%s'\n",argv[2]);
	    perror("rdflib");
	    exit(1);
	}
	
	fp2 = NULL;
	while (! feof(fp) ) {
	    /* read name */
	    p = buf;
	    while( ( *(p++) = (char) fgetc(fp) ) )  
		if (feof(fp)) break;

	    if (feof(fp)) break;

	    fp2 = NULL;
	    if (argv[1][0] == 'x') {
		/* check against desired name */
		if (! strcmp(buf,argv[3]) )
		{
		    fp2 = fopen(argv[4],"wb");
		    if (! fp2)
		    {
			fprintf(stderr,"rdflib: could not open '%s'\n",argv[4]);
			perror("rdflib");
			exit(1);
		    }
		}
	    }
	    else
		printf("%-40s ", buf);

	    /* step over the RDOFF file, extracting type information for
	     * the listing, and copying it if fp2 != NULL */

	    if (buf[0] == '.') {

		if (argv[1][0] == 't')
		    for (i = 0; i < 6; i++)
			printf("%c", copybytes(fp,fp2,1));
		else
		    copybytes(fp,fp2,6);

		l = copylong(fp,fp2);

		if (argv[1][0] == 't') printf("   %ld bytes content\n", l);

		copybytes(fp,fp2,l);
	    }
	    else if ((c=copybytes(fp,fp2,6)) >= '2') /* version 2 or above */
	    {
		l = copylong(fp,fp2);

		if (argv[1][0] == 't')
		    printf("RDOFF%c   %ld bytes content\n", c, l);
		copybytes(fp,fp2, l);	/* entire object */
	    }
	    else
	    {
		if (argv[1][0] == 't')
		    printf("RDOFF1\n");
		/*
		 * version 1 object, so we don't have an object content
		 * length field.
		 */
		copybytes(fp,fp2, copylong(fp,fp2));	/* header */
		copybytes(fp,fp2, copylong(fp,fp2));	/* text */
		copybytes(fp,fp2, copylong(fp,fp2));	/* data */
	    }

	    if (fp2)
		break;
	}
	fclose(fp);
	if (fp2)
	    fclose(fp2);
	else if (argv[1][0] == 'x')
	{
	    fprintf(stderr,"rdflib: module '%s' not found in '%s'\n",
		    argv[3],argv[2]);
	    exit(1);
	}
	break;

    default:
	fprintf(stderr,"rdflib: command '%c' not recognised\n",
		argv[1][0]);
	exit(1);
    }
    return 0;
}

