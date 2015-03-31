/*---------------------------------------------------------------------------------------------*/
/*  Desc:     Functions to analyze placement density (penalty), timing & displacement          */
/*                                                                                             */
/*  Authors:  Myung-Chul Kim, IBM Corporation (mckima@us.ibm.com)                              */
/*                                                                                             */
/*  Created:  03/12/2014                                                                       */
/*---------------------------------------------------------------------------------------------*/

#define _DEBUG

#include "evaluate.h"
#include "Flute/flute.h"

/* ******************************************************* */
/*  Desc: measure timing (TNS, WNS) of the given circuit   */
/* ******************************************************* */
bool circuit::measure_timing()
{
  update_pinlocs();
  build_steiner();
  slice_longwires(MAX_WIRE_SEGMENT_IN_MICRON * static_cast<double>(DEFdist2Microns));

  map<string, double> pincap;              /* map from pin name -> cap */
  ofstream netlist("feed.netlist");
	stringstream feed;
	feed.precision(5);
	feed<<scientific;

	// 0. writing inputs & output
	for(vector<unsigned>::iterator PI=PIs.begin() ; PI!=PIs.end() ; ++PI)
		feed << "input " << pins[ *PI ].name <<endl;
	for(vector<unsigned>::iterator PO=POs.begin() ; PO!=POs.end() ; ++PO)
	{
		feed << "output " << pins[ *PO ].name <<endl;
		// load capacitances at outputs
		pincap[ pins[ *PO ].name ] = pins[ *PO ].cap;
	}

	// 1.a writing input driver specs
	for(vector<unsigned>::iterator PI=PIs.begin() ; PI!=PIs.end() ; ++PI)
	{
		if(pins[ *PI ].name == clock_port)
			continue;

		int driver=pins[ *PI ].driverType;
		feed << "instance " << macros[driver].name;
		for(map<string, macro_pin>::iterator theMacPin = macros[driver].pins.begin() ; theMacPin != macros[driver].pins.end() ; ++theMacPin)
		{
			if((theMacPin->second).direction == "INPUT")
				feed << " " << theMacPin->first << ":" << pins[ *PI ].name + "_drvin";
			else if((theMacPin->second).direction == "OUTPUT")
				feed << " " << theMacPin->first << ":" << pins[ *PI ].name + "_drvout";
		}
		feed << endl;
	}

  // 1.b writing instance specs
  for(vector<cell>::iterator theCell = cells.begin() ; theCell != cells.end() ; ++theCell)
  {
		if(theCell->ports.size() == 0)
			continue;
    feed << "instance " << macros[theCell->type].name;
    for(map<string, unsigned>::iterator thePort = theCell->ports.begin() ; thePort != theCell->ports.end() ; ++thePort)
		{
			if(pins[ thePort->second ].type == PI_PIN && pins[ thePort->second ].name != clock_port)
			  feed << " " << thePort->first << ":" << pins[ thePort->second ].name + "_drvout";
			else
				feed << " " << thePort->first << ":" << pins[ thePort->second ].name;
		}
		feed <<endl;
  }

  // 2. writing wire specs
  map<string, bool> cap_written;
  // 2.a. adding up pin caps from wires (PI model)
  for(vector<net>::iterator theNet = nets.begin() ; theNet != nets.end() ; ++theNet)
    for(vector< pair< pair<string, string>, double > >::iterator theSeg=theNet->wire_segs.begin();
        theSeg != theNet->wire_segs.end() ; ++theSeg)
    {
			assert(theNet->name.length() < MAX_PIN_NAME_LENGTH);
			if(theNet->name == clock_port)
			{
				pincap [ theSeg->first.first ] += theSeg->second  / static_cast<double>(DEFdist2Microns) * GLOBAL_WIRE_CAP_PER_MICRON * 0.5;
				pincap [ theSeg->first.second ] += theSeg->second / static_cast<double>(DEFdist2Microns) * GLOBAL_WIRE_CAP_PER_MICRON * 0.5;
			}
			else
			{
				pincap [ theSeg->first.first ] += theSeg->second  / static_cast<double>(DEFdist2Microns) * LOCAL_WIRE_CAP_PER_MICRON * 0.5;
				pincap [ theSeg->first.second ] += theSeg->second / static_cast<double>(DEFdist2Microns) * LOCAL_WIRE_CAP_PER_MICRON * 0.5;
			}
      cap_written [ theSeg->first.first ] = false;
      cap_written [ theSeg->first.second ] = false;
    }

	// 2.b. zero-resistance wires between PIs and input drivers
	for(vector<unsigned>::iterator PI=PIs.begin() ; PI!=PIs.end() ; ++PI)
	{
		if(pins[ *PI ].name == clock_port)
			continue;
		feed << "wire " << pins[ *PI ].name << " " << pins[ *PI ].name+"_drvin" <<endl;
		feed << "  res " << pins[ *PI ].name << " " << pins[ *PI ].name+"_drvin "<< 0.0 <<endl;
	}	
  // 2.c. write wire specs (PI-model)
  for(vector<net>::iterator theNet = nets.begin() ; theNet != nets.end() ; ++theNet)
  {
		if(pins[ theNet->source ].type == PI_PIN && pins[ theNet->source ].name != clock_port)
			feed << "wire "<< pins[ theNet->source ].name + "_drvout";
		else
			feed << "wire "<< pins[ theNet->source ].name;
		assert(theNet->sinks.size() < MAX_WIRE_TAPS);
    for(vector<unsigned>::iterator theSink = theNet->sinks.begin() ; theSink != theNet->sinks.end() ; ++theSink)
      feed << " " << pins[ *theSink ].name;
    feed << endl;

		assert(theNet->wire_segs.size() < MAX_INTERNAL_NODES + MAX_WIRE_TAPS);
    for(vector< pair< pair<string, string>, double > >::iterator theSeg=theNet->wire_segs.begin() ; 
        theSeg != theNet->wire_segs.end() ; ++theSeg)
    {
      if(!cap_written[theSeg->first.first])
      {
        feed << "	cap " << theSeg->first.first << " " << pincap[ theSeg->first.first ]<<endl;
        cap_written[theSeg->first.first] = true;
      }
			if(theNet->name == clock_port)
      	feed << "	res " << theSeg->first.first << " " << theSeg->first.second << " " << theSeg->second / static_cast<double>(DEFdist2Microns) * GLOBAL_WIRE_RES_PER_MICRON <<endl;
			else
      	feed << "	res " << theSeg->first.first << " " << theSeg->first.second << " " << theSeg->second / static_cast<double>(DEFdist2Microns) * LOCAL_WIRE_RES_PER_MICRON <<endl;
      if(!cap_written[theSeg->first.second])
      {
        feed << "	cap " << theSeg->first.second << " " << pincap[ theSeg->first.second ]<<endl;
        cap_written[theSeg->first.second] = true;
      }
    }
  }
  cap_written.clear();
  pincap.clear();
	
	// 3 writing clock period
	feed << "clock "<< clock_port << " " << clock_period <<endl;

	// 4. writing arrival time and slew
	for(vector<unsigned>::iterator PI=PIs.begin() ; PI!=PIs.end() ; ++PI)
	{
		feed << "at " << pins[ *PI ].name << " " ;
		feed << pins[ *PI ].delay << " " << pins[ *PI ].delay << " " << pins[ *PI ].delay << " " << pins[ *PI ].delay <<endl;
		feed << "slew " << pins[ *PI ].name << " " ;
		feed << pins[ *PI ].fTran << " " << pins[ *PI ].rTran << endl;
	}
	// 4. writing require arrival time for outputs
	for(vector<unsigned>::iterator PO=POs.begin() ; PO!=POs.end() ; ++PO)
	{
		feed << "rat " << pins[ *PO ].name << " early " << 0.0 << " " << 0.0 << endl;
		feed << "rat " << pins[ *PO ].name << " late " << clock_period - pins[ *PO ].delay << " " << clock_period - pins[ *PO ].delay << endl;
	}
	netlist << feed.str();
	feed.clear();
	cout << "  Netlist file for timer is written : " << "feed.netlist" <<endl; 

  // 5. run timer
  netlist.close();
  system("timerFiles/timer timerFiles/cell.lib feed.netlist timing.out");

	// 6. read out results
	ifstream result("timing.out");
	if(!result.good())
	{
		cout << "ERROR - timer did not generate results" <<endl;
		return false;
	}

	string tmpStr, pinName;
	result >> tmpStr;
	if(tmpStr =="")
	{
		cout << "ERROR - timer did not generate results" <<endl;
		return false;
	}

	while(!result.eof())
	{
		assert(tmpStr == "slack");
		result >> pinName;
		result >> tmpStr;
		assert(tmpStr == "early" || tmpStr == "late");
		if(pin2id.find(pinName) == pin2id.end()) // _drvin or _drvout
			result >> tmpStr; 
		else
		{
			if(tmpStr == "early")
				result >> pins[ pin2id[pinName] ].earlySlk;
			else
				result >> pins[ pin2id[pinName] ].lateSlk;
		}
		result >> tmpStr;
	}
	eWNS=eTNS=0.0;
	lWNS=lTNS=0.0;
	for(vector<pin>::iterator thePin=pins.begin() ; thePin!=pins.end() ; ++thePin)
	{
		// for timing end points, i.e., POs and input of flops
		if(thePin->type == PO_PIN || thePin->isFlopInput)
		{
#ifdef DEBUG
		cout << thePin->name << " " << thePin->earlySlk << " " << thePin->lateSlk <<endl;
#endif
			eWNS = min(eWNS, thePin->earlySlk);
			lWNS = min(lWNS, thePin->lateSlk);

			eTNS += min(0.0, thePin->earlySlk);
			lTNS += min(0.0, thePin->lateSlk);
		}
	}
  return true;
}

/* ************************************************************************* */
/*  Desc: update pin locations based on owner cell locations & pin offsets   */
/* ************************************************************************* */
void circuit::update_pinlocs()
{
	for(vector<pin>::iterator thePin = pins.begin() ; thePin != pins.end() ; ++thePin)
	{
		if(thePin->isFixed || thePin->owner == numeric_limits<unsigned>::max())
		{
			thePin->x_coord = thePin->x_coord + thePin->x_offset;
			thePin->y_coord = thePin->y_coord + thePin->y_offset;
		}
		else
		{
			thePin->x_coord = cells[ thePin->owner ].x_coord + thePin->x_offset;
			thePin->y_coord = cells[ thePin->owner ].y_coord + thePin->y_offset;
		}
	}
  return;
}

/* ******************************************************************* */
/*  Desc: call FLUTE and populate nets->fluteTree (pair of loctaions)  */
/* ******************************************************************* */
void circuit::build_steiner()
{
	double max_clk_StWL=0.0;
  unsigned steiner_points_cnt=0;
  total_StWL=0.0;

  readLUT();

  for(vector<net>::iterator theNet = nets.begin() ; theNet != nets.end() ; ++theNet)
  {
    // clear theNet->wire_segs before population
    theNet->wire_segs.clear();
    unsigned numpins=theNet->sinks.size()+1; 

    // two pin nets
    if(numpins==2)
    {
      double wirelength = fabs(pins[ theNet->source ].x_coord - pins[ theNet->sinks[0] ].x_coord) +
        fabs(pins[ theNet->source ].y_coord - pins[ theNet->sinks[0] ].y_coord);
      total_StWL += wirelength;

			if(pins[ theNet->source ].type == PI_PIN && pins[ theNet->source ].name != clock_port)
				theNet->wire_segs.push_back( make_pair (make_pair(pins[ theNet->source ].name+"_drvout", pins[ theNet->sinks[0] ].name), wirelength) );
			else
				theNet->wire_segs.push_back( make_pair (make_pair(pins[ theNet->source ].name, pins[ theNet->sinks[0] ].name), wirelength) );
    }

    // otherwise, let's build a FLUTE tree
    else if(numpins > 2)
    {
			map< pair<string, string>, bool> pinpair_covered;
      unsigned *x = new unsigned[numpins];
      unsigned *y = new unsigned[numpins];

      map< pair<unsigned, unsigned>, string > pinmap;  // pinmap : map from pair< x location, y location> to pinName 
      x[0]=(unsigned)(max(pins[ theNet->source ].x_coord, 0.0));
      y[0]=(unsigned)(max(pins[ theNet->source ].y_coord, 0.0));
      pinmap[ make_pair(x[0], y[0]) ] = pins[ theNet->source ].name;
			if(pins[ theNet->source ].type == PI_PIN && pins[ theNet->source ].name != clock_port)
				pinmap[ make_pair(x[0], y[0]) ] = pins[ theNet->source ].name+"_drvout";
			else
				pinmap[ make_pair(x[0], y[0]) ] = pins[ theNet->source ].name;

      unsigned j=1;
      for(vector<unsigned>::iterator theSink=theNet->sinks.begin() ; theSink != theNet->sinks.end() ; ++theSink, j++)
      {
        x[j]=(unsigned)(max(pins[ *theSink ].x_coord, 0.0));
        y[j]=(unsigned)(max(pins[ *theSink ].y_coord, 0.0));
        pinmap[ make_pair(x[j], y[j]) ] = pins[ *theSink ].name;
      }
      Tree flutetree = flute(numpins, x, y, ACCURACY);
      delete [] x;
      delete [] y;

      int branchnum = 2*flutetree.deg - 2; 
      for(int j = 0; j < branchnum; ++j) 
      {
        int n = flutetree.branch[j].n;
        if(j == n) continue;

        double wirelength = fabs((double)flutetree.branch[j].x - (double)flutetree.branch[n].x) +
          fabs((double)flutetree.branch[j].y - (double)flutetree.branch[n].y);

				total_StWL += wirelength;
				if(theNet->name == "iccad_clk")
					max_clk_StWL = max(max_clk_StWL, wirelength);
				// any internal Steiner points will have blank names, so assign them to sp_<counter>
				string pin1=pinmap[ make_pair(flutetree.branch[j].x, flutetree.branch[j].y) ]; 
				if(pin1 == "")
				{
					steiner_points_cnt++;
					pin1="sp_"+to_string(static_cast<long long unsigned>(steiner_points_cnt));
					pinmap[ make_pair(flutetree.branch[j].x, flutetree.branch[j].y) ] = pin1;					
				}
				string pin2=pinmap[ make_pair(flutetree.branch[n].x, flutetree.branch[n].y) ]; 
				if(pin2 == "")
				{
					steiner_points_cnt++;
					pin2="sp_"+to_string(static_cast<long long unsigned>(steiner_points_cnt));
					pinmap[ make_pair(flutetree.branch[n].x, flutetree.branch[n].y) ] = pin2;					
				}
				if(pin1 == pin2) // NOTE: this only happens when two pin locations are the same (i.e., critically stacked)
				{
					for(vector<unsigned>::iterator theSink2=theNet->sinks.begin() ; theSink2 != theNet->sinks.end() ; ++theSink2)
					{
						if(pins[ *theSink2 ].name == pin1)
							continue;
						unsigned x=(unsigned)(max(pins[ *theSink2 ].x_coord, 0.0));
						unsigned y=(unsigned)(max(pins[ *theSink2 ].y_coord, 0.0));
						if(flutetree.branch[j].x == x && flutetree.branch[j].y == y)
						{
							if(pinpair_covered[ make_pair(pin1, pins[ *theSink2 ].name) ])
								continue;
							else
							{
								// find another pin's name
								pin2 = pins[ *theSink2 ].name;
								break;
							}
						}
					}
				}
				if(pin1 != pin2)
				theNet->wire_segs.push_back( make_pair( make_pair(pin1, pin2), wirelength ) );
				pinpair_covered[ make_pair(pin1, pin2) ]=true;
      }
			pinpair_covered.clear();
      pinmap.clear();
      free(flutetree.branch);
    }
  }
  cout << "  FLUTE: Total "<< steiner_points_cnt << " internal Steiner points are found." <<endl;
	total_StWL /= static_cast<double>(DEFdist2Microns);

#ifdef DEBUG
  for(vector<net>::iterator theNet=nets.begin() ; theNet != nets.end() ; ++theNet)
    for(vector< pair< pair<string, string>, double > >::iterator theSeg=theNet->wire_segs.begin() ; theSeg != theNet->wire_segs.end() ; theSeg++)
			cout << theSeg->first.first << ", "<< theSeg->first.second << " - "<< theSeg->second <<endl;
#endif

	cout << "  Longest clock wire segment : "<< max_clk_StWL/static_cast<double>(DEFdist2Microns) << " um" <<endl;

  return;
}

/* ******************************************************** */
/*  Desc: slice long wires into wire segements < threshold  */
/* ******************************************************** */
void circuit::slice_longwires(unsigned threshold)
{
	unsigned org_tot_wire_segments=0;
  unsigned tot_slice_cnt=0;
  unsigned tot_wire_segments=0;
  for(vector<net>::iterator theNet=nets.begin() ; theNet != nets.end() ; ++theNet)
  {
    unsigned orig_wire_seg_cnt=theNet->wire_segs.size();
		org_tot_wire_segments+=orig_wire_seg_cnt;
		for(unsigned i=0 ; i<orig_wire_seg_cnt ; i++)
    {
			pair< pair<string, string>, double > *theSeg = &theNet->wire_segs[i];
      // if the length of this wire segment is longer than the threshold, 
      // let's slice it equally and add proper nodes
      if(theSeg->second  > threshold)
      {
        string end_pin_name1=theSeg->first.first;
        string end_pin_name2=theSeg->first.second;
        unsigned slice_cnt = ceil(theSeg->second/(double)threshold);
        double sliced_wirelength=theSeg->second/(double)slice_cnt;

        string prev_pin_name=end_pin_name1+"_"+end_pin_name2+"_0";
//				if(prev_pin_name.length() >= MAX_PIN_NAME_LENGTH)
//					cout << prev_pin_name.length() <<endl;
				assert(prev_pin_name.length() < MAX_PIN_NAME_LENGTH);
        theSeg->first.second=prev_pin_name;
        theSeg->second=sliced_wirelength;
        for(unsigned j=1 ; j<slice_cnt ; j++, tot_slice_cnt++)
        {
          string cur_pin_name=end_pin_name1+"_"+end_pin_name2+"_"+to_string(static_cast<long long unsigned>(j));
					assert(cur_pin_name.length() < MAX_PIN_NAME_LENGTH);
          theNet->wire_segs.push_back( make_pair( make_pair(prev_pin_name, cur_pin_name), sliced_wirelength ) );
					prev_pin_name = cur_pin_name;
        }
        theNet->wire_segs[ theNet->wire_segs.size()-1 ].first.second=end_pin_name2;
      }
    }
    tot_wire_segments += theNet->wire_segs.size();
  }
  cout << "  Slicing wire segments: " << org_tot_wire_segments << " --> " << tot_wire_segments << " ( < "<< MAX_WIRE_SEGMENT_IN_MICRON << " um )" << endl;
  return;
}

/* ************************************************************* */
/*  Desc: measure max cell displacement from an input placement  */
/* ************************************************************* */
void circuit::measure_displacement()
{
	displacement=0.0;
	for(vector<cell>::iterator theCell=cells.begin() ; theCell!=cells.end() ; ++theCell)
		if(!theCell->isFixed)
	    displacement=max(displacement, fabs(theCell->init_x_coord - theCell->x_coord) + fabs(theCell->init_y_coord - theCell->y_coord));
	
	displacement /= static_cast<double>(DEFdist2Microns);
	return;
}

/* *************************************** */
/*  Desc: measure ABU and density penalty  */
/* *************************************** */
void circuit::measure_ABU(double unit, double targUt)
{
  double gridUnit = unit * rowHeight;
  int x_gridNum = (int) ceil ((rx - lx)/gridUnit);
  int y_gridNum = (int) ceil ((ty - by)/gridUnit);
  int numBins = x_gridNum*y_gridNum;

  cout << "  numBins         : "<<numBins << " ( "<< x_gridNum << " x "<< y_gridNum << " )"<< endl;
  cout << "  bin dimension   : "<< gridUnit << " x " << gridUnit <<endl;

  /* 0. initialize density map */
  vector<density_bin> bins(numBins);
  for(int j=0;j<y_gridNum;j++)
    for(int k=0;k<x_gridNum;k++)
    {
      unsigned binId= j*x_gridNum+k;
      bins[binId].lx=lx + k*gridUnit;
      bins[binId].ly=by + j*gridUnit;
      bins[binId].hx=bins[binId].lx + gridUnit;
      bins[binId].hy=bins[binId].ly + gridUnit;

      bins[binId].hx=min(bins[binId].hx, rx);
      bins[binId].hy=min(bins[binId].hy, ty);

      bins[binId].area=max((bins[binId].hx - bins[binId].lx) * (bins[binId].hy - bins[binId].ly), 0.0);
      bins[binId].m_util=0.0;
      bins[binId].f_util=0.0;
      bins[binId].free_space=0.0;
    }

  /* 1. build density map */
	/* (a) calculate overlaps with row sites, and add them to free_space */
	for(vector<row>::iterator theRow=rows.begin() ; theRow != rows.end() ; ++theRow)
	{
 	 	int lcol=max((int)floor((theRow->origX - lx)/gridUnit), 0);
    int rcol=min((int)floor((theRow->origX + theRow->numSites*theRow->stepX - lx)/gridUnit), x_gridNum-1);
    int brow=max((int)floor((theRow->origY - by)/gridUnit), 0);
    int trow=min((int)floor((theRow->origY + rowHeight - by)/gridUnit), y_gridNum-1);

    for(int j=brow;j<=trow;j++)
      for(int k=lcol;k<=rcol;k++)
      {
        unsigned binId= j*x_gridNum+k;

        /* get intersection */
        double lx=max(bins[binId].lx, (double)theRow->origX);
        double hx=min(bins[binId].hx, (double)theRow->origX + theRow->numSites*theRow->stepX);
        double ly=max(bins[binId].ly, (double)theRow->origY);
        double hy=min(bins[binId].hy, (double)theRow->origY + rowHeight);

        if((hx-lx) > 1.0e-5 && (hy-ly) > 1.0e-5)
        {
          double common_area = (hx-lx) * (hy-ly);
          bins[binId].free_space+= common_area;
					bins[binId].free_space = min(bins[binId].free_space, bins[binId].area);
        }
      }
	}

  /* (b) add utilization by fixed/movable objects */
  for(vector<cell>::iterator theCell=cells.begin() ; theCell != cells.end() ; ++theCell)
  {
    int lcol=max((int)floor((theCell->x_coord-lx)/gridUnit), 0);
    int rcol=min((int)floor((theCell->x_coord+theCell->width-lx)/gridUnit), x_gridNum-1);
    int brow=max((int)floor((theCell->y_coord-by)/gridUnit), 0);
    int trow=min((int)floor((theCell->y_coord+theCell->height-by)/gridUnit), y_gridNum-1);

    for(int j=brow;j<=trow;j++)
      for(int k=lcol;k<=rcol;k++)
      {
        unsigned binId= j*x_gridNum+k;

        /* get intersection */
        double lx=max(bins[binId].lx, (double)theCell->x_coord);
        double hx=min(bins[binId].hx, (double)theCell->x_coord+theCell->width);
        double ly=max(bins[binId].ly, (double)theCell->y_coord);
        double hy=min(bins[binId].hy, (double)theCell->y_coord+theCell->height);

        if((hx-lx) > 1.0e-5 && (hy-ly) > 1.0e-5)
        {
          double common_area = (hx-lx) * (hy-ly);
          if(theCell->isFixed)
            bins[binId].f_util+=common_area;
          else 
            bins[binId].m_util+=common_area;
        }
      }
  }

  int skipped_bin_cnt=0;
  vector<double> util_array(numBins, 0.0);
  /* 2. determine the free space & utilization per bin */
  for(int j=0;j<y_gridNum;j++)
    for(int k=0;k<x_gridNum;k++)
    {
      unsigned binId= j*x_gridNum+k;
      if(bins[binId].area > gridUnit*gridUnit*BIN_AREA_THRESHOLD)
      {
        bins[binId].free_space-= bins[binId].f_util;
        if(bins[binId].free_space > FREE_SPACE_THRESHOLD*bins[binId].area)
          util_array[binId] = bins[binId].m_util / bins[binId].free_space;
        else
          skipped_bin_cnt++;
#ifdef DEBUG
        if(util_array[binId] > 1.0)
        {
          cout << binId  << " is not legal. "<< endl;
          cout << " m_util: " << bins[binId].m_util << " f_util " << bins[binId].f_util << " free_space: " << bins[binId].free_space<< endl;
          exit(1);
        }
#endif
      }
    }

// Plot --------------------------------------------------------------------
	ofstream plotfile;
	// Plot a the utilization map.
	plotfile.open("grid.plt");
    
	plotfile << "set term png\n";
	plotfile << "set output \"grid.png\"\n";
	plotfile << "set autoscale fix\n";
	plotfile << "\n";
	plotfile << 
		"set palette defined ( 0 '#000090',\\\n"
		"                      1 '#000fff',\\\n"
		"                      2 '#0090ff',\\\n"
		"                      3 '#0fffee',\\\n"
		"                      4 '#90ff70',\\\n"
		"                      5 '#ffee00',\\\n"
		"                      6 '#ff7000',\\\n"
		"                      7 '#ee0000',\\\n"
		"                      8 '#7f0000')\n\n"
	;
	
	plotfile << "plot '-' matrix with image t ''\n";

	for (int j = 0; j < y_gridNum; j++) {
		for (int k = 0; k < x_gridNum; k++) {
			const unsigned binId = j*x_gridNum+k;
			plotfile << util_array[binId] << " ";
		} // end for
		plotfile << "\n";
	} // end for

	plotfile.close();

	// Plot a binary map of utilized bins.
	plotfile.open("circuit.plt");
    
	plotfile << "set term png\n";
	plotfile << "set output \"circuit.png\"\n";
	plotfile << "set autoscale fix\n";
	plotfile << "\n";
	plotfile << "set palette defined ( 0 0 0 0, 1 1 1 1 )\n";
	plotfile << "\n";
	
	plotfile << "plot '-' matrix with image t ''\n";

	for (int j = 0; j < y_gridNum; j++) {
		for (int k = 0; k < x_gridNum; k++) {
			const unsigned binId = j*x_gridNum+k;
			if ( bins[binId].f_util > 0 )
				plotfile << 0 << " ";
			else
				plotfile << 1 << " ";
		} // end for
		plotfile << "\n";
	} // end for
	plotfile.close();
  // -------------------------------------------------------------------------
  bins.clear();
  sort(util_array.begin(), util_array.end());

  /* 3. obtain ABU numbers */
  double abu1=0.0, abu2=0.0, abu5=0.0, abu10=0.0, abu20=0.0;
  int clip_index = 0.01*(numBins-skipped_bin_cnt);
  for(int j=numBins-1;j>numBins-1-clip_index;j--) {
    abu1+=util_array[j];
  }
  abu1=(clip_index) ? abu1/clip_index : util_array[numBins-1];

  clip_index = 0.02*(numBins-skipped_bin_cnt);
  for(int j=numBins-1;j>numBins-1-clip_index;j--) {
    abu2+=util_array[j];
  }
  abu2=(clip_index) ? abu2/clip_index : util_array[numBins-1];

  clip_index = 0.05*(numBins-skipped_bin_cnt);
  for(int j=numBins-1;j>numBins-1-clip_index;j--) {
    abu5+=util_array[j];
  }
  abu5=(clip_index) ? abu5/clip_index : util_array[numBins-1];

  clip_index = 0.10*(numBins-skipped_bin_cnt);
  for(int j=numBins-1;j>numBins-1-clip_index;j--) {
    abu10+=util_array[j];
  }
  abu10=(clip_index) ? abu10/clip_index : util_array[numBins-1];

  clip_index = 0.20*(numBins-skipped_bin_cnt);
  for(int j=numBins-1;j>numBins-1-clip_index;j--) {
    abu20+=util_array[j];
  }
  abu20=(clip_index) ? abu20/clip_index : util_array[numBins-1];
  util_array.clear();

  cout << "  target util     : "<< targUt <<endl;
  cout << "  ABU_2,5,10,20   : "<< abu2 <<", "<< abu5 <<", "<< abu10 <<", "<< abu20 <<endl;

  /* calculate overflow & ABU_penalty */
  abu1=max(0.0, abu1/targUt-1.0);
  abu2=max(0.0, abu2/targUt-1.0);
  abu5=max(0.0, abu5/targUt-1.0);
  abu10=max(0.0, abu10/targUt-1.0);
  abu20=max(0.0, abu20/targUt-1.0);
  ABU_penalty=(ABU2_WGT*abu2+ABU5_WGT*abu5+ABU10_WGT*abu10+ABU20_WGT*abu20)/(double)(ABU2_WGT+ABU5_WGT+ABU10_WGT+ABU20_WGT);
  return;
}

/* ***************************************** */
/*  Desc: measure Half-perimeter Wirelength  */
/* ***************************************** */
void circuit::measure_HPWL()
{
  update_pinlocs();
  double totalX=0.0;
  double totalY=0.0;

  for(vector<net>::iterator theNet=nets.begin() ; theNet != nets.end() ; ++theNet)
  {
    double netMaxX, netMinX;
    double netMaxY, netMinY;
    netMaxX=netMinX=pins[ theNet->source ].x_coord;
    netMaxY=netMinY=pins[ theNet->source ].y_coord;
    for(vector<unsigned>::iterator thePin=theNet->sinks.begin() ; thePin != theNet->sinks.end() ; ++thePin)
    {
      netMaxX=max(netMaxX, pins[ *thePin ].x_coord);
      netMinX=min(netMinX, pins[ *thePin ].x_coord);
      netMaxY=max(netMaxY, pins[ *thePin ].y_coord);
      netMinY=min(netMinY, pins[ *thePin ].y_coord);
    }
    totalX+=netMaxX-netMinX;
    totalY+=netMaxY-netMinY;
  }

  total_HPWL=(totalX+totalY) / static_cast<double>(DEFdist2Microns);
  return;
}
