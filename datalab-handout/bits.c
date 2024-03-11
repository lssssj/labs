/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  int a = x & y;
  int b = ~x & ~y;
  return ~a & ~b;
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1 << 31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  // int a = 1 | 1 << 1;
  // int b = a | a << 2;
  // int c = b | b << 4;
  // int d = c | c << 8;
  // int e = d | d << 15;
  // return !(x ^ e);
  //return (!(~(x + 1) ^ x)) & (!!(x ^ -1));
 // 就是x+1之后再取反等于原来的只有Tmax和-1，然后排除-1即可1
  int allOnes = ~0;           // 0xFFFFFFFF in two's complement
  int increment = x + 1;      // Increment x
  int sum = x + increment;    // Sum of x and x+1
  int equalZero = !increment;  // Check if increment is zero
  int isMax = !(sum ^ allOnes);// Check if sum is all ones
  return isMax & !equalZero;   // E
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  int a = 2;
  int b = a | a << 2;
  int c = b | b << 4;
  int d = c | c << 8;
  int e = d | d << 16;
  return !((x & e) ^ e); 
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  // 取反加1
  return ~x + 1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  int a = x >> 4;
  int y = x & 0b1111;
  int b = !(a ^ 3); // 1
  int a1 = !(y & 0b1000);
  int a2 = !(y ^ 0b1000);
  int a3 = !(y ^ 0b1001);
  return b & (a1 | a2 | a3);
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  int mask = (!x + ~0);
  return (y & mask) | (z & ~mask);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int diff = y + (~x + 1);
  int signx = x >> 31 & 1;
  int signy = y >> 31 & 1;
  int signDiff = diff >> 31 & 1;
  int u = (signx ^ signy); // 相同为0，不同为1
 // 符号不同时，0 1 1 0 返回x的符号
 // 符号相同时，返回diff的符号，diff为0 说明该返回1，diff为1时，y小于x, 返回为0 
  return (u & signx) | (!u & !signDiff);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  // 1111
  // 1111
  // 1111
  // 1111
  // 0110
  // 1001
  // 1111
  // 1001
  // 01000
  // 00111
  // 00000
  // 11111
  // 11111 
  // 01111
  // 11111
  // 10000
  // (x | (x - 1)) ^ -1 >> 
  // 10000 0111
  // int y = x | x << 1;
  // y = y | y << 2;
  // y = y | y << 4;
  // y = y | y << 8;
  // y = y | y << 16;
  // y = y | y >> 1;
  // y = y | y >> 2;
  // y = y | y >> 4;
  // y = y | y >> 8;
  // y = y | y >> 16;
  // return ~y & 1;

  int isZero = ~x + 1 | x;  // If x is 0, isZero will be 0; otherwise, isZero will be different from 0.
  return (isZero >> 31) + 1;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
 // 1111 1011  0100
int howManyBits(int x) {
  int op=x>>31;
  x=(op&~x)+(~op&x);
  int op5=!!(x>>16)<<4; x=x>>op5;
  int op4=!!(x>>8)<<3;  x=x>>op4;
  int op3=!!(x>>4)<<2;  x=x>>op3;
  int op2=!!(x>>2)<<1;  x=x>>op2;
  int op1=!!(x>>1);     x=x>>op1;
  int op0=x;
  return op0+op1+op2+op3+op4+op5+1;
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  unsigned exp = (uf >> 23) & 0xff;
  unsigned sign = uf & (1 << 31);
  if (exp == 0xff) {
    return uf;
  }
  if (exp == 0) {
    return sign | uf << 1;
  }
  // 0x 807f
  // 0x1 000 0000 0111 f f f f
  exp += 1;
  return (uf & 0x807fffff) | exp << 23;
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
 // sign exp num
int floatFloat2Int(unsigned uf)
{
  unsigned sign = (uf & 0x80000000);
  unsigned exp = (uf >> 23) & 0xff;
  unsigned real = (uf & 0x7fffff) | (1 << 23);
  unsigned t;
  uf = uf & 0x7fffffff;

  if (exp < 127) // uf < 1.0
    return 0;
  if (exp >= 150) // we can keep all 23 frac
  {
    t = exp - 150;
    if (t < 8) // round(uf) is in int range, and we can put no more than 7 zeros after real
    {
      real = real << t;
      return (sign == 0) ? real : (-real);
    }
    return 0x80000000; // out of range, we can't put more than 7 zeros after real
  }
  real = real >> (150 - exp); // the last serval bits of frac should be thrown
  return (sign == 0) ? real : (-real);
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x)
{
  if (x >= 128) {
    return 0x7f800000;
  }
  int exp = x + 127;
  if (1 <= exp && exp <= 254) // 2^x can be a normalized number
    return exp << 23;
  
  
  else // x <= -127 (exp <= 0), 2^x may be a denormalized number, maybe zero
  {
    if (x >= -149)
      return 1 << (149 + x);
    else
      return 0;
  }
  
}

