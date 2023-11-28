#include "ux_api.h"

UCHAR   inpb(ULONG addr){return(0);}
USHORT  inpw(ULONG addr){return(0);}
ULONG   inpl(ULONG addr){return(0);}

UCHAR outpb(ULONG addr, UCHAR b){return(b);}
USHORT  outpw(ULONG addr, USHORT w){return(w);}
ULONG outpl(ULONG addr, ULONG l){return(l);}

void ux_test_assert_hit(char* file, INT line){};