#include "move.h"
 
void Move::clear()
{
       moveInt = 0;
}
 
void Move::setFrom(unsigned int from)  
{   // bits  0.. 5
       moveInt &= 0xffffffc0; moveInt |= (from & 0x0000003f);
}
 
void Move::setTosq(unsigned int tosq)  
{   // bits  6..11      
       moveInt &= 0xfffff03f; moveInt |= (tosq & 0x0000003f) << 6;
}
 
void Move::setPiec(unsigned int piec)  
{   // bits 12..15
       moveInt &= 0xffff0fff; moveInt |= (piec & 0x0000000f) << 12;
} 
 
void Move::setCapt(unsigned int capt)  
{   // bits 16..19
       moveInt &= 0xfff0ffff; moveInt |= (capt & 0x0000000f) << 16;
} 
 
void Move::setProm(unsigned int prom)  
{   // bits 20..23
       moveInt &= 0xff0fffff; moveInt |= (prom & 0x0000000f) << 20;
} 
      
// read move information:
// first shift right, then mask to get the info
 
unsigned int Move::getFrom()  
{   // 6 bits (value 0..63), position  0.. 5
       return (moveInt        & 0x0000003f);
}  
 
unsigned int Move::getTosq()  
{   // 6 bits (value 0..63), position  6..11
       return (moveInt >>  6) & 0x0000003f; 
}   
 
unsigned int Move::getPiec()  
{   // 4 bits (value 0..15), position 12..15
       return (moveInt >> 12) & 0x0000000f; 
}   
 
unsigned int Move::getCapt()  
{   // 4 bits (value 0..15), position 16..19
       return (moveInt >> 16) & 0x0000000f; 
}   
 
unsigned int Move::getProm()  
{   // 4 bits (value 0..15), position 20..23
       return (moveInt >> 20) & 0x0000000f; 
}   
 
// boolean checks for some types of moves.
// first mask, then compare
// Note that we are using the bit-wise properties of piece identifiers, so we cannot just change them anymore !
 
BOOLTYPE Move::isWhitemove()  
{   // piec is white: bit 15 must be 0
       return (~moveInt & 0x00008000) == 0x00008000;
} 
 
BOOLTYPE Move::isBlackmove()  
{   // piec is black: bit 15 must be 1
       return ( moveInt & 0x00008000) == 0x00008000;
} 
 
BOOLTYPE Move::isCapture()    
{   // capt is nonzero, bits 16 to 19 must be nonzero
       return ( moveInt & 0x000f0000) != 0x00000000;
} 
 
BOOLTYPE Move::isKingcaptured()
{   // bits 17 to 19 must be 010
       return ( moveInt & 0x00070000) == 0x00020000;
} 
 
BOOLTYPE Move::isRookmove()
{   // bits 13 to 15 must be 110
       return ( moveInt & 0x00007000) == 0x00006000;
} 
 
BOOLTYPE Move::isRookcaptured()
{   // bits 17 to 19 must be 110
       return ( moveInt & 0x00070000) == 0x00060000;
} 
 
BOOLTYPE Move::isKingmove()
{   // bits 13 to 15 must be 010
       return ( moveInt & 0x00007000) == 0x00002000;
} 
 
BOOLTYPE Move::isPawnmove()
{   // bits 13 to 15 must be 001
       return ( moveInt & 0x00007000) == 0x00001000;
} 
 
BOOLTYPE Move::isPawnDoublemove()
{   // bits 13 to 15 must be 001 &
       //     bits 4 to 6 must be 001 (from rank 2) & bits 10 to 12 must be 011 (to rank 4)
    // OR: bits 4 to 6 must be 110 (from rank 7) & bits 10 to 12 must be 100 (to rank 5)
 
       return ((( moveInt & 0x00007000) == 0x00001000) && (((( moveInt & 0x00000038) == 0x00000008) && ((( moveInt & 0x00000e00) == 0x00000600))) || 
                                                          ((( moveInt & 0x00000038) == 0x00000030) && ((( moveInt & 0x00000e00) == 0x00000800)))));
} 
 
BOOLTYPE Move::isEnpassant()  
{   // prom is a pawn, bits 21 to 23 must be 001
       return ( moveInt & 0x00700000) == 0x00100000;
} 
 
BOOLTYPE Move::isPromotion()  
{   // prom (with color bit removed), .xxx > 2 (not king or pawn)
       return ( moveInt & 0x00700000) >  0x00200000;
} 
 
BOOLTYPE Move::isCastle()     
{   // prom is a king, bits 21 to 23 must be 010
       return ( moveInt & 0x00700000) == 0x00200000;
} 
 
BOOLTYPE Move::isCastleOO()   
{   // prom is a king and tosq is on the g-file
       return ( moveInt & 0x007001c0) == 0x00200180;
} 
 
BOOLTYPE Move::isCastleOOO()  
{   // prom is a king and tosq is on the c-file
       return ( moveInt & 0x007001c0) == 0x00200080;
} 
