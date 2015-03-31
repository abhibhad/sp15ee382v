////////////////////////////////////////////////////////////////////////////////
////
//// Guilherme Flach - 20/Jul/2014
////
//// This small test case shows that FLUTE can provide diferent results for 
//// large nets (where it's suboptimal) depending on the pin order. I haven't
//// taken a look deeply inside FLUTE's algorithm, but it sorts the pins first 
//// by x and then by y. 
////
//// So the order is only an issue when there are two or more pins in the same x
//// so that the x and y unmatch in both cases after sorting.
////
//// Solution: One can avoid this by using y as a tie break when FLUTE sorts
//// the node. This way even if there are more than one point at same x, the
//// sorted point will be the same independently of input order.
////
//// Replace the following in flute():
////
//// if (minval > ptp[j]->x ) {
////
//// by
////
//// if (minval > ptp[j]->x || 
////     (minval == ptp[j]->x  && ptp[minidx]->y > ptp[j]->y) {
////
//////////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <iostream>
using std::cout;
#include <algorithm>
#include <vector>
using std::vector;
#include <utility>
using std::pair;
using std::make_pair;
#include "Flute/flute.h"

const int N = 30;

DTYPE x0[N] = { 77470, 80500, 78060, 77810, 79160, 80070, 78290, 78040, 79210, 77590, 77940, 77890, 78100, 77620, 77720, 78610, 78260, 79030, 77860, 78210, 78500, 77380, 77570, 78020, 78040, 78310, 77850, 78130, 78190, 78150 };
DTYPE y0[N] = { 39300, 45900, 45300, 46700, 45700, 42900, 46900, 47900, 49900, 45700, 46500, 46100, 42100, 46500, 38900, 46900, 49700, 46300, 44300, 46100, 45100, 42500, 46100, 42500, 44500, 38900, 45500, 40500, 42300, 46700 };

DTYPE x1[N] = { 80500, 78060, 77810, 79160, 80070, 78290, 78040, 79210, 77590, 77940, 77890, 78100, 77620, 77720, 78610, 78260, 79030, 77860, 78210, 78500, 77380, 77570, 78020, 78040, 78310, 77850, 78130, 78190, 78150, 77470 };
DTYPE y1[N] = { 45900, 45300, 46700, 45700, 42900, 46900, 47900, 49900, 45700, 46500, 46100, 42100, 46500, 38900, 46900, 49700, 46300, 44300, 46100, 45100, 42500, 46100, 42500, 44500, 38900, 45500, 40500, 42300, 46700, 39300 };

int main() {
	cout << "Reading LUTs...\n";
	readLUT();
	cout << "Reading LUTs... Done\n" << std::endl;

	int mapping0[N];
	int mapping1[N];

	Tree flutetree0 = flute(N, x0, y0, ACCURACY);
	Tree flutetree1 = flute(N, x1, y1, ACCURACY);

	DTYPE wl0 = wirelength(flutetree0);
	DTYPE wl1 = wirelength(flutetree1);

	cout << "Flute Wirelength: " << wl0 << "\n";
	cout << "Flute Wirelength: " << wl1 << "\n";
	cout << "Diff: " << abs(wl0 - wl1) << "\n";

	if(wl0 != wl1)
	{
		cout << "Tree0:" << "\n";
		printtree(flutetree0);
		cout << "Tree1:" << "\n";
		printtree(flutetree1);
	}
	
	// Sanity check.
	vector< pair<DTYPE, DTYPE> > v0(N);
	vector< pair<DTYPE, DTYPE> > v1(N);
	for ( int i = 0; i < N; i++ ) {
		v0[i] = make_pair(x0[i], y0[i]);
		v1[i] = make_pair(x1[i], y1[i]);
	} // end for

	std::sort(v0.begin(), v0.end());
	std::sort(v1.begin(), v1.end());

	for ( int i = 0; i < N; i++ ) {
		assert(v0[i] == v1[i]);
	} // end for

	return 0;
} // end main
