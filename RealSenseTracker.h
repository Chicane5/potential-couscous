#pragma once
#ifndef __REALSENSETRACKER_H__
#define __REALSENSETRACKER_H__
//include the VRPN tracking stuff

#include <stdio.h>
#include <tchar.h>

#include <math.h>

#include <vrpn_Text.h>
#include <vrpn_Tracker.h>
#include <vrpn_Connection.h>

struct TrackingData
{
	float tx;
	float ty;
	float tz;

	float rx;
	float ry;
	float rz;
	float rw;
};

class RealSenseTracker : public vrpn_Tracker
{
public:
	RealSenseTracker(vrpn_Connection *c = 0);
	virtual ~RealSenseTracker();
	virtual void mainloop();
	void setTrackingData(TrackingData inData);

protected:
	struct timeval _timestamp;

private:
	TrackingData* mp_TrackingData;
};

#endif
