# lab1 datalab笔记

主要考验位操作的相关技巧。熟练了就不难。

## 1. bitXor
```c++
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
 int bitXor(int x, int y) {
  
}
```
x^y=(x&\~y)|(\~x&y)，m|n=\~（~m&\~n）

```c++
int bitXor(int x, int y) {
  return ~(~(x&~y)&~(~x&y));
}
```

## 2. tmin
略过

## 3. isTmax
```c++
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
}
```
考虑4位，x=0111，x+1=1000,有2x+1=1111，证明k=1111：!(~k)
但x=1111时也成立，需排除掉。x+1!=0可以排除掉

```c++
int isTmax(int x) {
  int m=x+1;
  return !!m&!(~(m+x));
}
```

## 4. allOddBits
```c++
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
}
```
构造a=0xAAAAAAAA,(a&x)|(a>>1)判断是否为0xFFFFFFFF

```c++
int allOddBits(int x) {
  int a=0xAA;
  a=a|(a<<8);
  a=a|(a<<16);
  int m=x&a;
  return !(~(m|(m>>1)));
}
```

## 5. negate
考察定义
```c++
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x+1;
}
```

## 6. isAsciiDigit
```c++
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
}
```
x-0x30是否大于等于0，0x39-x是否大于等于0
```c++
int isAsciiDigit(int x) {
  int Tmin=1<<31;
  int a=x+~0x30+1;
  int b=0x3A+~x;
  return !(a&Tmin)&!(b&Tmin);
}
```

## 7. conditional
```c++
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
}
```
(y&m)+(z&~m)，当x为true时，m=0xffff，当x为false时，m=0
```c++
int conditional(int x, int y, int z) {
  int m=(!x)+~0;
  return (y&m)+(z&~m);
}
```
## 8. isLessOrEqual
```c++
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
}
```
符号相同，比差值符号。符号不同，看y符号。

注意：不能简单看差值符号！正-负有可能会正溢出，导致符号为负

```c++
int isLessOrEqual(int x, int y) {
  int Tmin=1<<31;
  int diff=!!((x^y)&Tmin); //sign diff,1; else, 0
  int a=!(y&Tmin);
  int b=!((y+~x+1)&Tmin);
  return (diff&a)|(!diff&b);
}
```

## 9. logicalNeg
```c++
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
}
```
0的特殊性：相反数的符号和自己相同（唯一特例：Tmin，需要排除）
```c++
int logicalNeg(int x) {
  int a=(x>>31)+1;
  int b=(((~x+1)^x)>>31)+1;
  return a&b;
}
```
## 10. howManyBits
```c++
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
int howManyBits(int x) {
}
```
首先，如果x为负，将x取反。
利用二分法，32位中看高16位是否有1，如果有，则x右移16位。然后看x的16位中的高8位是否有1，...以此类推即可。

```c++
int howManyBits(int x) {
  int b16,b8,b4,b2,b1;
  x=x^(x>>31);  // sign=0: x=x; sign=1: x=~x
  b16=!!(x>>16)<<4; // high 16 bits has 1: 16; else: 0
  x>>=b16;
  b8=!!(x>>8)<<3;
  x>>=b8;
  b4=!!(x>>4)<<2;
  x>>=b4;
  b2=!!(x>>2)<<1;
  x>>=b2;
  b1=!!(x>>1);
  x>>=b1;
  return b16+b8+b4+b2+b1+(!!x)+1;  //1 for sign bit
}
```
## 11. floatScale2
```c++
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
}
```
当阶数exp=0时，需要将frac右移一位。注意：可能会进位到阶数！
当阶数exp!=0且不全为1时，表示规格化的数，需要给exp+1
当阶数exp全为1时，表示无穷或NaN。

```c++
unsigned floatScale2(unsigned uf) {
  unsigned exp=0x7f800000;  // exp all 1
  if((uf&exp)==0){ // exp=0, frac right shift
    return ((uf&0x007fffff)<<1)|(uf&0x80000000);
  }else if((uf&exp)!=exp){  // exp not 0 or all 1, exp++
    return uf+0x00800000;
  }else{  // exp all 1, uf=NaN or infinity
    return uf;
  }
}
```

## 12. floatFloat2Int
```c++
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
int floatFloat2Int(unsigned uf) {
}
```
规格化的值：在阶码最后一位设为1，高位设为0，并根据阶码大小和23的差值，左移或者右移。
非规格化的值：当阶码全为1，或者E>=32（此时对应int值已经超过INT_MAX了）

```c++
int floatFloat2Int(unsigned uf) {
  int e=(uf>>23)&0xff;
  int E=e-127;
  int exp=0x7f800000;
  int s=(uf>>31)&0x1;
  unsigned INF=1<<31;

  if((uf&exp)==exp||E>=32){
    return INF;
  }
  if(E<0){
    return 0;
  }
  // uf exp last bit sets 1, high bits set 0
  uf|=0x00800000;
  uf&=0x00ffffff;
  if(E>=23){
    uf<<=(E-23);
  }else{
    uf>>=(23-E);
  }
  if(s){
    uf=~uf+1;
  }
  return uf;
}
```

## 13. floatPower2
```c++
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
}
```
尾数全为0，关键看阶码。
x=1时，$2^x=2_{10}=10_2$，要左移22位，23-(e-127)=22，e=128
得到关系：e=x+127
当x+127为负时，由于e是无符号数，不能为负，则返回0。
由于E=e-127的范围是[-126,127]，x的范围也是[-126,127]


```c++
unsigned floatPower2(int x) {
  int e=x+127;
  if(e<0){
    return 0;
  }
  if(x>127){
    return 0x7f800000u;
  }
  return e<<23;
}
```