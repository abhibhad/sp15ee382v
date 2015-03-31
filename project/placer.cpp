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
    cout << "Usage : iccad2014_evaluation [.iccad2014]" << endl ;
    return 0;
  }
	cout << "Command line : " << endl;
	for(int i=0 ; i<argc ; i++)
		cout << " " << argv[i];
	cout << endl;

  circuit ckt;
  ckt.read_iccad2014_file(argv[1]);

  ckt.print();

  return 1;
}
