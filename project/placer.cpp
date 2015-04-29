/*********************************************************
 * Placer for Physical Design Automation
 * Abishek Bhaduri
 * Chandler Brown
 * 2015
 *********************************************************/

#include "evaluate.h"

int main(int argc, char** argv)
{
  cout << "================================================================================" <<endl;
  cout << "    ICCAD 2014 Incremental Timing-Driven Placement Contest Evaluation Script    " <<endl;
  cout << "    Authors : Myung-Chul Kim, Jin Hu {mckima, jinhu}@us.ibm.com                 " <<endl;
  cout << "================================================================================" <<endl;

  if(argc != 2)
  {
    cout << "Incorrect arguments. exiting .."<<endl;
    cout << "Usage : placer [.iccad2014]" << endl ;
    return 0;
  }
	cout << "Command line : " << endl;
	for(int i=0 ; i<argc ; i++)
		cout << " " << argv[i];
	cout << endl;

  circuit ckt;
  ckt.read_iccad2014_file(argv[1]);
  ckt.copy_init_to_final();
  //ckt.update_pinlocs();

  ckt.doStuff();
  cout<<endl<<endl<<endl;
  ckt.print();
  

  
  string thePlacer = "placer";
  string identifier = "";
  ckt.write_bookshelf();
  ckt.convert_pl_to_def(thePlacer,identifier);

  return 1;
}
