#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"

using namespace std;


/*
function for vector<s_binentry> sorting
*/
bool mysort(const s_binentry& a, const s_binentry& b)
{
  return a.k < b.k;
}

/*
store() creates binbook.bin, after being called by makehash() and supplied
with a vector full of hash keys and moves. It sorts them, removes the
duplicates, and writes the stripped data to binbook.bin
*/


void store(vector<s_binentry> full)
{
  cout<<endl<<endl<<"Now in store, sorting to make binbook.bin";
  cout<<endl<<"Old size = "<<full.size();
  s_binentry bookentry;
  /*
  read the book into a vector, t
  */
  unsigned int q, notu;
  vector<s_binentry>::iterator p;
  vector<s_binentry> t;
  for (q = 0; q < full.size(); ++q)
    {
      notu = 0;
      bookentry = full[q];
      //entry been read, now see if it exists
      for (p = t.begin(); p < t.end(); ++p)
      {
          if( (*p).m[0] == bookentry.m[0] &&
              (*p).m[1] == bookentry.m[1] &&
              (*p).m[2] == bookentry.m[2] &&
              (*p).m[3] == bookentry.m[3] &&
              (*p).k == bookentry.k
            )
          {
              notu = 1;
              (*p).freq++;
          }
      }
      if (notu == 0)
      {
       t.push_back(bookentry);
      }
      if (q % 500 == 0)
      {
         cout<<".";
         if (q % 10000 == 0)
         {cout<<" "<<q<<" new size "<<t.size()<<endl;}
      }
    }

  /*
  sort the vector in hash key order
  */
  sort(t.begin(), t.end(), mysort);

  unsigned int c = 0;
  cout<<"\n final size "<<t.size();


  for (c = 0; c < t.size(); ++c)
  {
      fwrite(&t[c], sizeof(s_binentry), 1, bookfile);
  }

}

int book_init()
{
     int i;
     s_binentry move;
     /*
    open the files of binary moves for reading
    */
    wbookfile = fopen("binbook.bin", "rb");

    if (!wbookfile)
		{
            cout<<"\n no binbook.bin!!"<<endl;
            wbookfile = NULL;
            return -1;
        }

    // obtain file size.
    fseek (wbookfile , 0 , SEEK_END);
    bookdata.whitelsize = ftell (wbookfile);
    rewind (wbookfile);
    bookdata.whiteentries = bookdata.whitelsize/sizeof(s_binentry);

    for (i = 0; i < bookdata.whiteentries; ++i)
    {
      fseek(wbookfile, sizeof(s_binentry)*i, SEEK_SET);
      fread(&move, sizeof(s_binentry), 1, wbookfile);
      whitebook.push_back(move);
    }

    cout<<"\nwhite book size = "<<bookdata.whitelsize<<" bytes, with ";
    cout<<whitebook.size()<<" entries"<<endl;

    return 0;
}

void book_close()
{
            fclose(wbookfile);
            wbookfile = NULL;
}

/*
makehash() opens a text file of book lines called gamelines.txt,
 e2e4 e7e5 etc, reads them into a vector, vbooklines<>, and then writes the
 resulting hash key and move for each move in a vector, whihc is then passed
 to store(). Store takes this vector, removes the duplicates, and writes the
 data, sorted by key, into binbook.bin

Input : gamelines.txt

Output : binary.bin
*/

void makehash()
{

    char line[32576];


    /*
    open the file of text moves for reading
    */
    book_file = fopen("gamelines.txt", "r");

    if (!book_file)
		{
            cout<<"\n gamelines.txt!!";
            return;
        }

     /*
     read the text moves into a vector, vbooklines<>
     */
     cout<<"\n\n...beginning vector read..."<<endl<<endl;
     char bookfileline[32576];

     while (fgets(bookfileline, 32576, book_file))
     {
       vbooklines.push_back(bookfileline);

      // cout<<endl<<"\nwriting in "<<bookfileline<<endl;
     }
     cout<<"\ntotal lines = "<<vbooklines.size()<<endl<<endl;

    /*
    now open a file for writing the binary keys
    */
    bookfile = fopen("binbook.bin", "wb");
     if (!bookfile)
		{
            cout<<"\n no binbook.bin created to write to!!";
            return;
        }
    /*
    for each line in the vector, parse the line and write the keys
    */
    movestotal = 0;
    cout<<"\n\n...beginning to fill vector with hash entries..."<<endl<<endl;
    for (unsigned int i = 0; i < vbooklines.size(); ++i)
    {
      if (i%100 == 0)
      {
         cout<<".";
         if(i%1000 == 0)
         cout<<" "<<i<<endl;
      }
      strcpy(line, vbooklines[i].c_str());
      parseopeningline(line);
   }
   cout<<"\ntotal moves,"<<movestotal;
   cout<<"\nhash entry vector size = "<<hashentries.size();

   /*
   now use store to sort and remove duplicates, creating binbook.bin
   */
   store(hashentries);

   /*
   the binbook.bin file now contains the hashkeys for an opening book,
   so close the files and assign pointers to NULL
   */
    if (book_file){
		fclose(book_file);
        //book_file=NULL;
        }
   	if (bookfile){
		fclose(bookfile);
        //bookfile=NULL;
        }

        vbooklines.clear();
        hashentries.clear();

}

void parseopeningline(char string[])
{
     char *moves;
     const char *ptr;
     char *ptrtwo;
     char move_string[256];
     int flag,t;
     int movemade;
     moves = string;
     unsigned __int64 oldkey;
    // cout<<"\nparsing line "<<string;
     moves++;

     setboard(startfen);
     /*
     now, if we have moves, parse and make them
     */
     while (moves != NULL)
     {
       ptr = moves;
      // cout<<"\n ptr = "<<ptr;
       movemade = 0;

       while(*ptr != '\0')
       {
          move_string[0] = *ptr++;
          move_string[1] = *ptr++;
          move_string[2] = *ptr++;
          move_string[3] = *ptr++;

          if(*ptr == '\0' || *ptr == ' ' || *ptr == '+')
           {
              move_string[4] = '\0';
              if (*ptr == '+') {*ptr++;}
           }
          oldkey = p->hashkey;
         // cout<<"\nunderstanding "<<move_string;
          flag =  understandmove(move_string, &t);

          if (flag == -1)
          {
             ptr = NULL;
             moves = NULL;
             return;
          }
          ptrtwo = move_string;
          binenter.k = oldkey;
          binenter.m[0] = move_string[0];
          binenter.m[1] = move_string[1];
          binenter.m[2] = move_string[2];
          binenter.m[3] = move_string[3];
          binenter.m[4] = move_string[4];
          binenter.freq = 0;
          hashentries.push_back(binenter);
        //  fwrite(&binenter, sizeof(s_binentry), 1, bookfile);

          while (*ptr == ' ') ptr++;
          movemade++;
          movestotal++;
          if (movemade > 20)
          {
             ptr = NULL;
             ptrtwo = NULL;
             moves = NULL;
             return;
          }
         }
      }
           ptr = NULL;
           ptrtwo = NULL;
           moves = NULL;
           return;
}

/*
 findhashbookmove() finds a book move from the hash book.
*/
int wfindhashbookmove()
{
     movegen();
     int f[2] = {-1,-1}, m[2] = {-1,-1};//2 most popular matches

    int match,r;
    srand(double(clock()));
    vector<s_binentry>::iterator it;

    for (it = whitebook.begin(); it < whitebook.end(); it++)
    {
      if ((*it).k == p->hashkey)
      {
          match = myparse( (*it).m);
          if (match != -1)
          {
              if((*it).freq>f[0])
              {
                 f[1] = f[0];
                 m[1] = m[0];
                 f[0] = (*it).freq;
                 m[0] = match;
              }
              else if((*it).freq>f[1])
              {
                f[1] = (*it).freq;
                m[1] = match;
              }
          }

      }//---end if
    }//---end for loop through book array
    if (m[0] == -1)//no match
     {
        return -1;
     }

     srand( (unsigned)time( NULL ) ); // init Rand() function
     r = rand()%2;
     cout<<"\n book hit, freq "<<f[r];
     cout<<" position "<<r;

   return m[r];
}







