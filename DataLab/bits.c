/* bit manipulation */
/* 
 * bitOr - x|y using only ~ and & 
 *   Example: bitOr(6, 5) = 7
 *   Legal ops: ~ &
 *   Max ops: 8
 *   Rating: 1
 */
int bitOr(int x, int y) {
  /*In this puzzle De Morgan's Laws are used*/
  return ~(~x & ~y);
}
/* 
 * implication - return x -> y in propositional logic - 0 for false, 1
 * for true
 *   Example: implication(1,1) = 1
 *            implication(1,0) = 0
 *   Legal ops: ! ~ ^ |
 *   Max ops: 5
 *   Rating: 2
 */
int implication(int x, int y) {
    return (!x) | y; 
}
/* 
 * isEqual - return 1 if x == y, and 0 otherwise 
 *   Examples: isEqual(5,5) = 1, isEqual(4,5) = 0
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int isEqual(int x, int y) {
  /* XORing two variables returns zero in case the variables are equal */
  return !(x ^ y); 
}
/* 
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n) {
  /* Using a mask to extract demanded byte */
  int shifted_word = x >> (n << 3);
  int result_byte = shifted_word & 0x000000FF;
  return result_byte;

}
/* 
 * replaceByte(x,n,c) - Replace byte n in x with c
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: replaceByte(0x12345678,1,0xab) = 0x1234ab78
 *   You can assume 0 <= n <= 3 and 0 <= c <= 255
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 10
 *   Rating: 3
 */
int replaceByte(int x, int n, int c) {
  int mask = 0x000000FF;
  int shifted_mask = mask << (n << 3);
  // mask with 0x00 on the position of the replaced byte
  int inversed_mask = ~shifted_mask;
  int num_removed_byte = x & inversed_mask;
  int byteInPosition = c << (n << 3);
  int result_word = num_removed_byte | byteInPosition; 
  return result_word;
}
/*
 * bitParity - returns 1 if x contains an odd number of 0's
 *   Examples: bitParity(5) = 0, bitParity(7) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
int bitParity(int x) {
  /*
  x = x1_x2_x3_x4_x5_x6_x7_x8, where xi - nibble
  after all the shifts x8 = (x1 ^ x2 ^ x3 ^ x4 ^ x5 ^ x6 ^ x7 ^ x8)
  thus parity of x8 after the shifts is equal to parity of the whole word (can be proven)
  0x6996 - "parity table" (the LSB of 0x6996 after the shift equals nibble and, therefore, word parity 
  */
  int parityTable = (0x69 << 8) | 0x96;
  x ^= x >> 16;
  x ^= x >> 8;
  x ^= x >> 4;
  x &= 0x0F;
  return (parityTable >> x) & 1; 
}
/* 2's complement */
/* 
 * fitsShort - return 1 if x can be represented as a 
 *   16-bit, two's complement integer.
 *   Examples: fitsShort(33000) = 0, fitsShort(-32768) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int fitsShort(int x) {
  /* 
     x can be stored in a short datatype if it has 15 significant (that are not used for signing) bits
     the idea is to see if the bits 15 - 17 are all 0 (which means that x is positive and fits into a variable of 2-bytes length) or 1 (which means that x is negative, but the significant bits fit into a variable of 2-bytes length) 
  */
  /*int shiftedX = (x & 0xFFFF8000) >> 15;
  return !(shiftedX ^ 0x1FFFF) | !(shiftedX);*/
  int shiftedX = x >> 15;
  return !(~shiftedX) | !shiftedX;
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x + 1;
}
/* 
 * sign - return 1 if positive, 0 if zero, and -1 if negative
 *  Examples: sign(130) = 1
 *            sign(-23) = -1
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 10
 *  Rating: 2
 */
int sign(int x) {
    int sign = x >> 31;
    // if x is not negative then 0
    int negOneIfNegX = (~0 + !sign);
    int oneIfXEqualsZero = (!x); 
    // !(oneIfXEqualsZero | sign) equals 0 if x <=0, else it equals 1
    return negOneIfNegX + !(oneIfXEqualsZero | sign); 
}
/* 
 * isLess - if x < y  then return 1, else return 0 
 *   Example: isLess(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLess(int x, int y) {
  int signMask = (0x80 << 24);
  int isXNeg = (x & signMask) >> 31;
  int isYNeg = (y & signMask) >> 31;
  int subXY = x + (~y + 1);
  int equal = !subXY; 
  int yGreaterX = (subXY & signMask) >> 31;
  int isXNegYPos = (isXNeg & (!isYNeg));
  int isXPosYNeg = ((!isXNeg) & isYNeg);
  return (!(isXPosYNeg | equal)) & (isXNegYPos | yGreaterX);
}
/*
 * multFiveEighths - multiplies by 5/8 rounding toward 0.
 *   Should exactly duplicate effect of C expression (x*5/8),
 *   including overflow behavior.
 *   Examples: multFiveEighths(77) = 48
 *             multFiveEighths(-22) = -13
 *             multFiveEighths(1073741824) = 13421728 (overflow)
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 3
 */
int multFiveEighths(int x) {
  int sign = (x >> 31) & 1;
  int resultAfterDivision = (x << 2) + x;
  // if x < 0 then additionForNeg = 7, else = 0
  int additionForNeg = (sign << 3) + (~sign + 1);
  int balancedResult  = resultAfterDivision + additionForNeg; 
  int resultAfterMultiplication = balancedResult >> 3;
  
  int result = resultAfterMultiplication;
  return result;
}
/* 
 * absVal - absolute value of x
 *   Example: absVal(-1) = 1.
 *   You may assume -TMax <= x <= TMax
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 10
 *   Rating: 4
 */
int absVal(int x) {
/*
  if x < 0, signSeq = 0xffffffff, else signSeq = 0x0
  if x < 0 negOne = -1, else negOne = 0 
  if x > 0, expression for absX is identical to absX = x
  if x < 0, then (x ^ signSeq) = ~(-x),  (x ^ signSeq) + negOne = ~(-x) - 1, which turns negative value to positive
  thus, if x > 0, return x, if x < 0 return -x 
*/
  int signSeq = (x >> 31);
  int negOne = ~signSeq + 1;
  int absX = (x ^ signSeq) + negOne;
  return absX;
}
