#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"
#include "TreeNode.h"
#include "BStar.h"
#include "MCTS_AB.h"
#include "Uci.h"

#include "TreeNode.h"
#include "SmpManager.h"
SmpManager SmpWorkers;

/*********************************************************************
**
Cuatro días se le pasaron en imaginar qué nombre le podría: porque, 
según se decía él a sí mismo, no era razón que caballo de caballero tan famoso, 
y tan bueno él por sí, estuviese sin nombre conocido; 
y así procuraba acomodársele, de manera que declarase quien había sido, 
antes que fuese de caballero andante, y lo que era entonces: 
pues estaba muy puesto en razón, que mudando su señor estado, 
mudase él también el nombre; y le cobrase famoso y de estruendo, 
como convenía a la nueva orden y al nuevo ejercicio que ya profesaba: 
y así después de muchos nombres que formó, borró y quitó, añadió, 
deshizo y tornó a hacer en su memoria e imaginación, 
al fin le vino a llamar ROCINANTE, nombre a su parecer alto, 
sonoro y significativo de lo que había sido cuando fue rocín, 
antes de lo que ahora era, que era antes y primero de todos los rocines del mundo. 
**
Four days were spent in thinking what name to give him, 
because (as he said to himself) it was not right that a horse 
belonging to a knight so famous, and one with such merits of his own,
should be without some distinctive name, 
and he strove to adapt it so as to indicate what he had been before belonging 
to a knight-errant, and what he then was; for it was only reasonable that, 
his master taking a new character, he should take a new name, 
and that it should be a distinguished and full-sounding one, 
befitting the new order and calling he was about to follow. And so, 
after having composed, struck out, rejected, added to, unmade, 
and remade a multitude of names out of his memory and fancy, 
he decided upon calling him Rocinante, a name, to his thinking, 
lofty, sonorous, and significant of his condition as a hack before 
he became what he now was, the first and foremost of all the hacks in the world. 
**
** Miguel de Cervantes.
*/


int main(int argc,char **argv)
{
	Board::SetLevel(2); // knowledge level by default.
	CUci *protocol = new CUci();	
    protocol->start();
	delete protocol;
	// Stop all things
	SmpWorkers.StopWorkers();
	while(!SmpWorkers.AllStopped())SmpWorkers.Sleep();

	return 0;
}