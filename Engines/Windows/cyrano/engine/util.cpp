//
// Cyrano Chess engine
//
// Copyright (C) 2007,2008  Harald JOHNSEN
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#include "engine.hpp"
#include "util.hpp"
#include <ctype.h>

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

// my_stricmp()  copied from Fruit (and changed a bit /)

bool my_stricmp(const char string_1[], const char string_2[]) {


   Assert(string_1!=NULL);
   Assert(string_2!=NULL);

   while (true) {

      int c1 = *string_1++;
      int c2 = *string_2++;

      if (tolower(c1) != tolower(c2)) return !false;
      if (c1 == '\0') return !true;
   }

   return !false;
}

