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
	void setTrackingData(const TrackingData& inData);
	void setSmoothingConstant(float inSmoothConstant);

protected:
	struct timeval _timestamp;
	float smoothData(float observed_value, float t_minus_one_value);

private:
	TrackingData* mp_TrackingData;
	float m_smoothing_constant;
};

#endif
