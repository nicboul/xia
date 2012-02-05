/*
 * See COPYRIGHTS file.
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "journal.h"
#include "utils.h"

extern int daemonize()
{
	journal_ftrace(__func__);

	pid_t pid, sid;

	if (getppid() == 1)
		return;

	pid = fork();
	if (pid < 0)
		exit(EXIT_FAILURE);

	if (pid > 0)
		exit(EXIT_SUCCESS);

	umask(0);

	sid = setsid();

	if (sid < 0)
		exit(EXIT_FAILURE);

	if ((chdir("/")) < 0)
		exit(EXIT_FAILURE);

	if (freopen("/dev/null", "r", stdin) == NULL)
		return -1;

	if (freopen("/dev/null", "w", stdout) == NULL)
		return -1;

	if (freopen("/dev/null", "w", stderr) == NULL)
		return -1;
}

extern int bv_set(const unsigned int dbit, 
		  uint8_t *vector, 
		  const unsigned int VSIZE)
{
     journal_ftrace(__func__); 

     if (dbit > 0 && dbit < VSIZE) {
		vector[dbit >> 3] |= 1 << (dbit&7);
		return 0;
	}

	journal_notice("utils]> The dbit `%i` is out of the vector `%i` \
			:: %s:%i\n", dbit, VSIZE, __FILE__, __LINE__);
	return -1;
}

extern int bv_unset(const unsigned int dbit, 
		    uint8_t *vector, 
		    const unsigned int VSIZE)
{
    journal_ftrace(__func__);

	if (dbit > 0 && dbit < VSIZE) {
		vector[dbit >> 3] |= ~(1 << (dbit&7));
		return 0;
	}

	journal_notice("utils]> The dbit `%i` is out of the vector `%i` \
			:: %s:%i\n", dbit, VSIZE, __FILE__, __LINE__);
	return -1;
}

extern int bv_test(const unsigned int dbit, 
		   uint8_t *vector, 
		   const unsigned int VSIZE)
{
	if (dbit > 0 && dbit < VSIZE)
		return vector[dbit >> 3] >> (dbit&7) & 1;
	
	journal_notice("utils]> The dbit `%i` is out of the vector `%i` \
			:: %s:%i\n", dbit, VSIZE, __FILE__, __LINE__);
	return -1;
}

extern char *trim(char *str)
{
	journal_ftrace(__func__);

	if (str == NULL)
		return NULL;

	char *a, *z;
	a = str;
	while (*a == ' ') a++;

	z = a + strlen(a);
	if (z == NULL)
		return NULL;

	while (*--z == ' ' && (z > a));
	*++z = '\0';

	return a;
}

extern char *x_strtok(char **a, char **z, char delim)
{
	journal_ftrace(__func__);

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
	journal_ftrace(__func__);

        char *s;
        s = strchr(str, '@');

        if (s == NULL || context == 0)
                return -1;

        s++;
        snprintf(s, sizeof(context), "%u", context);

        return 0;
}

char *x509_get_cn(char *path)
{
	journal_ftrace(__func__);

	FILE *f;
	size_t n;
	unsigned char *buf;

	char *needle;
	char *end;
	char *cn;

	size_t ret;

	f = fopen(path, "rb");
	if (f == NULL) {
		journal_notice("utils]> fopen() no such file : %s :: %s %i \n", path, __FILE__, __LINE__);
		return NULL;
	}
	
	ret = fseek(f, 0, SEEK_END);
	if (ret == -1) {
		journal_notice("utils]> fseek() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return NULL;
	}

	n = (size_t) ftell(f);
	
	ret = fseek(f, 0, SEEK_SET);
	if (ret == -1) {
		journal_notice("utils]> fseek() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return NULL;
	}

	buf = (unsigned char *) malloc(n+1);
	if (buf == NULL) {
		journal_notice("utils]> malloc() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return NULL;
	}

	ret = fread(buf, 1, n, f);
	if (ret != n) {
		fclose(f);
		free(buf);
		journal_notice("utils]> fread() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return NULL;
	}

	buf[n] = '\0';

	needle = strstr(buf, "Subject:");
	if (needle == NULL) {
		journal_notice("utils]> strstr() \"Subject\" substring not found :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}

	needle = strstr(needle, "CN=");
	if (needle == NULL) {
		journal_notice("utils]> strstr() \"CN=\" substring not found :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}

	needle += strlen("CN=");
	if (needle == NULL) {
		journal_notice("utils]> strstr() \"CN=\" substring not found :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}


	end = strchr(needle, '/');
	if (end == NULL) {
		journal_notice("utils]> strchr() \"/\" character not found :: %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}

	*end = '\0';

	cn = strdup(needle);
	if (cn == NULL) {
		journal_notice("utils]> strdup() %s :: %s:%i\n", strerror(errno), __FILE__, __LINE__);
		return NULL;
	}

	if (buf)
		free(buf);

	if (cn == NULL)
		return NULL;

	return cn;
}
