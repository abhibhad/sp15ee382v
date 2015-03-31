/*-----------------------------------------------------------------*/
/*  Desc:     A main function to read and evaluate solutions       */
/*                                                                 */
/*  Authors:  Myung-Chul Kim, IBM Corporation (mckima@us.ibm.com)  */
/*            Jin Hu, IBM Corporation (jinhu@us.ibm.com)           */
/*                                                                 */
/*  Created:  03/12/2014                                           */
/*-----------------------------------------------------------------*/

#include "evaluate.h"

int main(int argc, char** argv)
{
  cout << "================================================================================" <<endl;
  cout << "    ICCAD 2014 Incremental Timing-Driven Placement Contest Evaluation Script    " <<endl;
  cout << "    Authors : Myung-Chul Kim, Jin Hu {mckima, jinhu}@us.ibm.com                 " <<endl;
  cout << "================================================================================" <<endl;

  if(argc != 4 && argc != 5)
  {
    cout << "Incorrect arguments. exiting .."<<endl;
    cout << "Usage : iccad2014_evaluation ICCAD14.parm [.iccad2014] [target_util] (optional)[final .def]" << endl ;
    return 0;
  }
	cout << "Command line : " << endl;
	for(int i=0 ; i<argc ; i++)
		cout << " " << argv[i];
	cout << endl;

  circuit ckt;
	ckt.read_parameters(argv[1]);
  ckt.read_iccad2014_file(argv[2]);
#ifdef BENCHMARK_GEN
	string AcademicPlacer = "ComPLx";
	string Identifier = "-ComPLx";
	ckt.write_bookshelf();
	// insert command line to run the placer 
	ckt.convert_pl_to_def(AcademicPlacer, Identifier);
	string final_def=benchmark+"_"+AcademicPlacer+".def";
	ckt.read_def(final_def, FINAL);
#else
	if(argc == 5)
	{
		string final_def(argv[4]);
		ckt.read_def(final_def, FINAL);
	}
	else
		ckt.copy_init_to_final();
#endif
#ifdef DEBUG
  ckt.print();
#endif

  cout << endl;
  cout << "Evaluating the solution file ..                                                 " <<endl;
  cout << "--------------------------------------------------------------------------------" <<endl;

  cout << "Analyzing placement .. "<<endl;
  ckt.measure_displacement();
  cout << "  max displ. (um) : " << ckt.displacement << endl;
  ckt.measure_ABU(BIN_DIM, atof(argv[3]));
  cout << "  ABU penalty     : " << ckt.ABU_penalty <<endl;
  cout << "  alpha           : " << ALPHA  <<endl;
  cout << endl;

  cout << "Analyzing timing .. "<<endl;
  ckt.measure_HPWL();
  bool timer_done = ckt.measure_timing();
  cout << "  HPWL, StWL (um) : " << ckt.total_HPWL << ", " <<ckt.total_StWL << endl;
  cout << "  Scaled StWL     : " << ckt.total_StWL * (1 + ALPHA * ckt.ABU_penalty) << " ( "<< ALPHA * ckt.ABU_penalty*100 << "% )"<<endl;
	cout << "  Clock period    : " << ckt.clock_period << endl;
	if(timer_done)
	{
		cout << "  early WNS, TNS  : " << ckt.eWNS << ", " << ckt.eTNS <<endl;
		cout << "  late  WNS, TNS  : " << ckt.lWNS << ", " << ckt.lTNS <<endl;
	}
	else
		cout << "  WNS, TNS        : Timer failed. The values are not available." <<endl;

  cout << "--------------------------------------------------------------------------------" <<endl;
	cout << endl;
	cout << "Checking legality .. " <<endl;
  cout << "--------------------------------------------------------------------------------" <<endl;
  if(ckt.check_legality())
  	cout << "Placement is LEGAL."<<endl;
	else
  	cout << "Placement is ILLEGAL. see check_legality.log."<<endl;
  cout << "--------------------------------------------------------------------------------" <<endl;

  return 1;
}
