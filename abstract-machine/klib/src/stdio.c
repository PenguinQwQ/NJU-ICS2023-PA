#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)



/*
1. vsprintf is null ended?

*/

static uint32_t fmt_flags;
static uint32_t width;
// static uint32_t precision;
#define FLAG_RIGHT_ALIGN 0x01
#define FLAG_LEFT_ALIGN 0x02
#define FLAG_POSITIVE_SIGN 0x04
#define FLAG_POSITIVE_SPACE 0x08
#define FLAG_ZERO_PAD 0x10
#define FLAG_SPACE_PAD 0x20

static char minint[] = "8463847412-";
static uint32_t int_types;
#define INT_TYPE_POS 0x01
#define INT_TYPE_NEG 0x02
#define INT_TYPE_ZERO 0x04




char *_write_int(char *out, int d)
{
  int_types = 0;
  uint32_t wid = 0;
  int x = d;
  char num_buf[15];
  int top = 0;
  if(x < 0) int_types |= INT_TYPE_NEG;
  if(x == 0) int_types |= INT_TYPE_ZERO;
  if(x > 0) int_types |= INT_TYPE_POS;
  if(x == -2147483648)
  {
    wid = 11;
    top = 12;
 //   for (int i = 0 ; i <= 11 ; i++)
 //     num_buf[i] = minint[i];
    strcpy(num_buf, minint);
    goto WRITE_OUT;
  }
  else
    {
      if(x == 0)
      {
          wid++;
          top = 1;
          num_buf[0] = '0';
          goto WRITE_OUT;
      }
      //Extract the int bits;
      if(x < 0) x = -x;
      while(x > 0)
        {
          wid++;
          num_buf[top++] = x % 10 + '0';
          x /= 10;
        }
      if(d > 0 && ((fmt_flags & FLAG_POSITIVE_SIGN) || (fmt_flags & FLAG_POSITIVE_SPACE)))
          {
            wid++;
            num_buf[top++] = (fmt_flags & FLAG_POSITIVE_SIGN) ? '+' : ' ';
          }
      if(d < 0) 
        {
          wid++;
          num_buf[top++] = '-';
        }
      goto WRITE_OUT;
    }


WRITE_OUT:
//Here num_buf stores the string representation of our int
if(fmt_flags & FLAG_RIGHT_ALIGN) //Right align
  {
    if(wid >= width) //There is no need to pad
      while(top)
        *out++ = num_buf[--top];
    else //wid < width, means we need padding
    {
      if(fmt_flags & FLAG_ZERO_PAD)
        while(wid < width)
          {
            *out++ = '0';
             width--;
          }
      else
        while(wid < width)
          {
            *out++ = ' ';
             width--;
          }   
      assert(width == wid);
      while(top)
        *out++ = num_buf[--top];    
    }

  }
else //Left align
  {
    if(wid >= width) //There is no need to pad
    {
      while(top)
      {
        *out++ = num_buf[--top];
      }
    }
    else //wid < width, means we need padding
    {      
      while(top)
      {
        *out++ = num_buf[--top];  
      }
      assert((fmt_flags & FLAG_ZERO_PAD) == false);//shouldn't zero pad when left align, zero pad ignored with left align in gcc!
      while(wid < width)
        {
           *out++ = ' ';
           width--;
        }   
      assert(wid == width);    
    }
  }
  assert(top == 0);//Write all the bytes into out!
  return out;
}


int vsprintf(char *out, const char *fmt, va_list ap) {
  //First check the output string pointer is valid
  assert(out != NULL);
  char *dst = out;
  goto main_loop;


main_loop:
    while(*fmt && *fmt != '%') *out++ = *fmt++; //if fmt is valid and not reach %
    if(*fmt == '\0') goto end_field; //if fmt reaches end, we finish the conversion, break!;
    
    assert(*fmt == '%'); //Assure this step we reach % controller
    fmt++; //Go one step further
    //Parameter field, TODO
    fmt_flags = 0;
    goto flags_field;

  flags_field:
    //Flags field
    switch(*fmt)
    {
      case '-': //Minus flag, left align
        fmt_flags |= FLAG_LEFT_ALIGN;
        fmt++;
        goto flags_field;
      case '+': //Positive
        fmt_flags |= FLAG_POSITIVE_SIGN;
        fmt++;
        goto flags_field;
      case ' ': //Space, prepends a space for pos int, neg with -
        fmt_flags |= FLAG_POSITIVE_SPACE;
        fmt++;
        goto flags_field;
      case '0': //zero padding for width
        fmt_flags |= FLAG_ZERO_PAD;
        fmt++;
        goto flags_field;
      default: //There is no flags, next process step!
        if((fmt_flags & FLAG_LEFT_ALIGN) == false) fmt_flags |= FLAG_RIGHT_ALIGN;
        if((fmt_flags & FLAG_ZERO_PAD) == false) fmt_flags |= FLAG_SPACE_PAD;
        goto width_field;
    }

  width_field:
    width = 0; //init, if width is 0 means no width specified, and thus no padding required
    while(*fmt >= '0' && *fmt <= '9')
      {
        width = width * 10 + (*fmt - '0');
        fmt++;
      }
    goto type_field;
  /* TODO
  precision_field:
    precision = 0;
    if(*fmt == '.') 
    {
     fmt++;
     while(*fmt >= '0' && *fmt <= '9')
        {
          precision = precision * 10 + (*fmt - '0');
          fmt++;
        }     
    }
  */
  //length field: TODO
  //Type field
  type_field:
    switch(*fmt++)
    {
      case 'd':
        int d = va_arg(ap, int);
        out = _write_int(out, d);
        break;
      case 'c':
        char ch = (char)va_arg(ap, int); // solve the int promotion rule.
        *out++ = ch;
        break;
      case 's':
        char *str = va_arg(ap, char *);
        while(*str != '\0') *out++ = *str++;
        break;
    }
    goto main_loop;
end_field:
  *out = '\0'; //set the end of the output string to '\0'
  int ret = out - dst;
  return ret - 1;//minus the '\0' byte
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}
static char print_buf[65535];
int printf(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  int ret = vsprintf(print_buf, format, ap);
  va_end(ap);
  int len = strlen(print_buf);
  for (int i = 0 ; i < len ; i++)
    putch(print_buf[i]); //The IOE provides API for the upper klib, so we use putch to print char
    //by the serial.
  return ret;
}



//Printf n byte contents in ... into the out
int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}



//Printf n byte contents in ap into the out
int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif