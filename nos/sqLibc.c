#include "sq.h"
#include "sqPlatformSpecific.h"

/**
 * 
 * This file contains the libc functions that we have to reimplement
 * for squeaknos platform because they are platform dependent.
 *
**/

__thread int __libc_errno;

int printf_pocho (const char *format, ...);

void exit(int _)
{
	mark(0x00f8);
	printf_pocho("exit()\n");
	ioExit();
}

#define HEAP_SIZE       1024*1024

void *malloc(unsigned int size)
{
	static char heap[HEAP_SIZE];
	static char *heap_end = &heap[HEAP_SIZE];
	static char *heap_new=heap;

	unsigned long long total;
	total = size;

	if (heap_new + total < heap_end)
	{
		char *answer = heap_new;
		heap_new += total;
		return answer;
	}
	printf_pocho("malloc(%d)\n", size);
	ioExit();
}

void *calloc(unsigned int count, unsigned int size)
{
	return malloc(count*size);
}

void free(void *p)
{
	printf_pocho("free(%x)\n",p);
	ioExit();
}

// Console especific code

/* Some screen stuff.  */
/* The number of columns.  */
#define COLUMNS                 80
/* The number of lines.  */
#define LINES                   24
/* The attribute of an character.  */
#define ATTRIBUTE               7
/* The video memory address.  */
#define VIDEO                   0xB8000

/* Variables.  */
/* Save the X position.  */
static int xpos;
/* Save the Y position.  */
static int ypos;
/* Point to the video memory.  */
static volatile unsigned char *video;

/* Clear the screen and initialize VIDEO, XPOS and YPOS.  */
static void cls (void)
{
	int i;

	video = (unsigned char *) VIDEO;

	for (i = 0; i < COLUMNS * LINES * 2; i++)
		*(video + i) = 0;

	xpos = 0;
	ypos = 0;
}

/* Scroll one line up */
static void scroll (void)
{
	int i;

	video = (unsigned char *) VIDEO;

	for (i = 0; i < COLUMNS * (LINES+1) * 2; i++)
		*(video + i) = *(video + i + COLUMNS * 2);
	for (i = 0; i < COLUMNS * 2; i++)
		*(video + i + (LINES+1)*COLUMNS*2) = 0;
}

/* Convert the integer D to a string and save the string in BUF. If
   BASE is equal to 'd', interpret that D is decimal, and if BASE is
   equal to 'x', interpret that D is hexadecimal.  */
static void
itoa (char *buf, int base, int d)
{
	char *p = buf;
	char *p1, *p2;
	unsigned long ud = d;
	int divisor = 10;

	/* If %d is specified and D is minus, put `-' in the head.  */
	if (base == 'd' && d < 0)
	{
		*p++ = '-';
		buf++;
		ud = -d;
	}
	else if (base == 'x')
		divisor = 16;

	/* Divide UD by DIVISOR until UD == 0.  */
	do
	{
		int remainder = ud % divisor;

		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
	}
	while (ud /= divisor);

	/* Terminate BUF.  */
	*p = 0;

	/* Reverse BUF.  */
	p1 = buf;
	p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}
}


/**
 * Parses the C string str interpreting its content as an integral number, which is returned as an int value.
**/
int _atoi(char *str) {
  unsigned int answer=0;
  int base = 10;

  if (*str == '0' && str[1] == 'x') {
          base = 0x10;
          str += 2;
  }

  while (*str) {
    answer *= base;
    if (*str > '9') 
      answer += (*str | 0x20) - 'a' + 0xa;
    else 
      answer += *str - '0';
    str++;
  }
  return answer;
}

/* Put the character C on the screen.  */
int putchar (int c)
{
	if (1)
	{
		std_console_put_char(c);
		return;
	}
	
	if (c == '\n' || c == '\r')
	{
newline:
		for (;xpos<COLUMNS;xpos++)
		{
			*(video + (xpos + ypos * COLUMNS) * 2) = 0;
			*(video + (xpos + ypos * COLUMNS) * 2 + 1) = 0;
		}
		xpos = 0;
		ypos++;
		if (ypos > LINES+1)
		{
			scroll();
			ypos--;
		}
		return;
	}

	*(video + (xpos + ypos * COLUMNS) * 2) = c & 0xFF;
	*(video + (xpos + ypos * COLUMNS) * 2 + 1) = ATTRIBUTE;

	xpos++;
	if (xpos >= COLUMNS)
		goto newline;
}

/* Format a string and print it on the screen, just like the libc
   function printf_pocho.  */
int
bprintf (const char *format, char **arg)
{
	int c;
	char buf[20];

	arg++;

	while ((c = *format++) != 0)
	{
		if (c != '%')
			putchar (c);
		else
		{
			char *p;

do_next:
			c = *format++;
			switch (c)
			{
			case 'l':
				goto do_next;
			case 'd':
			case 'u':
			case 'x':
				itoa (buf, c, *((int *) arg++));
				p = buf;
				goto string;
				break;

			case 's':
				p = *arg++;
				if (! p)
					p = "(null)";

string:
				while (*p)
					putchar (*p++);
				break;

			default:
				putchar (*((int *) arg++));
				break;
			}
		}
	}
}

int __printf_chk (int __flag, __const char *__restrict __format, ...)
{
	char **arg = (char **) &__format;

	printf_pocho(__format, arg);
}

int printf_pocho (const char *format, ...)
{
	char **arg = (char **) &format;

	bprintf(format, arg);
}

int puts(const char *msg)
{
	printf_pocho("%s", msg);
}

FILE *stderr = (FILE *)-2;

int fprintf(FILE *file, const char *format, ...)
{
	char **arg = (char **) &format;

	if (file == stderr) bprintf(format, arg);
	else printf_pocho("fprintf called with file %x.\n", file);
}

int fputs(const char *msg, FILE *file)
{
	if (file == stderr) printf_pocho("%s", msg);
	else printf_pocho("fputs called with file %x.\n", file);
}

size_t fwrite(const void *buf, size_t size, size_t nmemb, FILE *file)
{
	char *msg = (char*)buf;
	int total = size*nmemb;
	if (file == stderr)
		while (total--) putchar(*msg++);
	else printf_pocho("fwrite called with file %x.\n", file);
}

int getchar(void)
{
	return 0;
}

time_t time(time_t *t)
{
	time_t answer;

	answer = (time_t)ioSeconds();
	if (t) *t = answer;
	return answer;
}

void
__assert_fail(const char *assertion,
              const char *file,
              unsigned int line,
              const char *function)
{
	printf_pocho("ASSERTION (%s) FAILED in \"%s:%d\" in function \"%s\".\n",
	             assertion, file, line, function);
	exit(0);
}