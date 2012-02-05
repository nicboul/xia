/*
 * Copyright 2008 Mind4Networks Technologies INC.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "journal.h"
#include "utils.h"

extern int bv_set(const unsigned int dbit, 
		  uint8_t *vector, 
		  const unsigned int VSIZE)
{
     journal_strace("bv_set"); 

     if (dbit > 0 && dbit < VSIZE) {
		vector[dbit >> 3] |= 1 << (dbit&7);
		return 0;
	}

	journal_notice("The dbit `%i` is out of the vector `%i` \
			:: %s:%i\n", dbit, VSIZE, __FILE__, __LINE__);
	return -1;
}

extern int bv_unset(const unsigned int dbit, 
		    uint8_t *vector, 
		    const unsigned int VSIZE)
{
    journal_strace("bv_unset");

	if (dbit > 0 && dbit < VSIZE) {
		vector[dbit >> 3] |= ~(1 << (dbit&7));
		return 0;
	}

	journal_notice("The dbit `%i` is out of the vector `%i` \
			:: %s:%i\n", dbit, VSIZE, __FILE__, __LINE__);
	return -1;
}

extern int bv_test(const unsigned int dbit, 
		   uint8_t *vector, 
		   const unsigned int VSIZE)
{

 /*    journal_strace("bv_test");
  *    can't be used... this function have been abuse by saj@
  */

	if (dbit > 0 && dbit < VSIZE)
		return vector[dbit >> 3] >> (dbit&7) & 1;
	
	journal_notice("The dbit `%i` is out of the vector `%i` \
			:: %s:%i\n", dbit, VSIZE, __FILE__, __LINE__);
	return -1;
}

extern char *trim(char *str)
{
	journal_strace("trim");

	char *a, *z;
	a = str;
	while (*a == ' ') a++;

	z = a + strlen(a);
	while (*--z == ' ' && (z > a));
	*++z = '\0';

	return a;
}

extern char *x_strtok(char **a, char **z, char delim)
{
	journal_strace("x_strtok");

	char *r = *a;
	if (*a == NULL)
		return r;

	*z = strchr(*a, delim);
	
	if (*z == NULL) {
		*a = NULL;
		return r;
	}

	**z = '\0';
	*a = ++(*z);

	return r;
}

int swap_context(char *str, const unsigned int context)
{
	journal_strace("swap_context");

        char *s;
        s = strchr(str, '@');

        if (s == NULL || context == 0)
                return 1;

        s++;
        snprintf(s, sizeof(context), "%u", context);

        return 0;
}

char *x509_get_cn(char *path)
{
	journal_strace("x509_get_cn");

	FILE *f;
	size_t n;
	unsigned char *buf;

	char *needle;
	char *end;
	char *cn;

	size_t ret;

	f = fopen(path, "rb");
	if (f == NULL) {
		journal_failure(EXIT_ERR, "fopen() no such file : %s :: %s %i \n", path, __FILE__, __LINE__);
		return NULL;
	}
	
	fseek(f, 0, SEEK_END);
	n = (size_t) ftell(f);
	fseek(f, 0, SEEK_SET);

	buf = (unsigned char *) malloc(n+1);
	if (buf == NULL) {
		perror("malloc");
	}

	ret = fread(buf, 1, n, f);
	if (ret != n) {
		fclose(f);
		free(buf);
		perror("fread");
	}

	buf[n] = '\0';

	needle = strstr(buf, "Subject:");
	needle = strstr(needle, "CN=");
	needle += strlen("CN=");

	end = strchr(needle, '/');

	*end = '\0';

	cn = strdup(needle);
	free(buf);

	return cn;
}

