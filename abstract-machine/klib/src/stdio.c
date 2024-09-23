#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

/*
static void reverse(char* str, int st, int ed)
{
	assert(st <= ed);
	while(st < ed)
	{
		char tmp = str[st];
		str[st] = str[ed];
		str[ed] = tmp;
		st++, ed--;
		}
		}
		*/

int sprintf(char *out, const char *fmt, ...);

int format_type(const char *fmt, int index)
{
		if(fmt[index] != '%') return 0;
		if(fmt[index + 1] == 's') return 10;
		else if (fmt[index + 1] == 'd') return 11;
		else if (fmt[index + 1] == 'c') return 12;
		else if (fmt[index + 1] == '0' && fmt[index + 2] > '0' 
						&&fmt[index + 2] <= '9' && fmt[index + 3] == 'd')
				return fmt[index + 2] - '0';
		else return 0;
}

int printf(const char *fmt, ...) {
		va_list ap;
		int i = 0;
		int j = 0;
		char* s;
		int d;
		int ch;
		char buf[4096];

		char arg[64];

		va_start(ap, fmt);
		while(fmt[i] != '\0')
		{
				int type = format_type(fmt, i);
				if(type)
				{
						if(type == 10)
						{
								s = va_arg(ap, char*);
								j += sprintf(buf + j, "%s", s);
								i += 2;
						}
						else if(type == 11 || (type > 0 && type <= 9))
						{
								d = va_arg(ap, int);
								arg[0] = '%';
								if (type > 0 && type <= 9) 
								{
										arg[1] = '0';
										arg[2] = type + '0';
										arg[3] = 'd';
										arg[4] = '\0';
										i += 4;
								}
								else if (type == 11)
								{
										arg[1] = 'd';
										arg[2] = '\0';
										i += 2;
								}
								j += sprintf(buf + j, arg, d);
						}
						else if (type == 12)
						{
								ch = va_arg(ap, int);
								j += sprintf(buf + j, "%c", ch);
								i += 2;
						}
				}
				else
						buf[j++] = fmt[i++];
		}
		buf[j] = '\0';
		j = 0;
		while(buf[j] != '\0')
		{
				putch(buf[j]);
				j++;
		}

		va_end(ap);
		return j;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}



int sprintf(char *out, const char *fmt, ...) {
		va_list ap;
		int i = 0;
		int j = 0;
		char* s;
		int d;
		int ch;
		va_start(ap, fmt);
		while(fmt[i] != '\0')
		{
				int type = format_type(fmt, i);
				if(type)
				{
						if(type == 10)
						{
								s = va_arg(ap, char*);
								int k = 0;
								while(s[k] != '\0')
										out[j++] = s[k++];
								i += 2;
						}
						else if (type == 11 || (type > 0 && type <= 9))
						{
								d = va_arg(ap, int);
								int st = j;
								int ed;
								bool isneg = false;
								int digit = 0;
								if (d < 0)
								{
										isneg = true;
										d = -d;
								}

								do{
										out[j++] = d % 10 + '0';
										d /= 10;
										digit++;
								}while(d > 0);

								if(type > 0 && type <= 9 && digit < type)
								{
										while(digit < type)
										{
												out[j++] = '0';
												digit++;
										}
								}

								if(isneg) out[j++] = '-';
								ed = j - 1;

								// reverse [st ~ ed]

								while(st < ed)
								{
										char tmp = out[st];
										out[st] = out[ed];
										out[ed] = tmp;
										st++, ed--;
								}

								if(type == 11) i += 2;
								else if(type > 0 && type <= 9) i += 4;
						}
						else if (type == 12)
						{
								ch = va_arg(ap, int);
								out[j++] = ch;
								i += 2;
						}
				}
				else out[j++] = fmt[i++];
		}
		va_end(ap);
		out[j] = '\0';
		return j; // or j + 1?
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
