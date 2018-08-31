#include "stdafx.h"
#include "RealSenseTracker.h"

RealSenseTracker::RealSenseTracker(vrpn_Connection *c /*= 0 */) :
	vrpn_Tracker("TTFRealSenseTracker0", c),
	mp_TrackingData(nullptr)
{
	mp_TrackingData = new TrackingData();
}

void RealSenseTracker::mainloop()
{
	vrpn_gettimeofday(&_timestamp, NULL);

	vrpn_Tracker::timestamp = _timestamp;

	// We will just put a fake data in the position of our tracker
	static float angle = 0; angle += 0.001f;

	// the pos array contains the position value of the tracker
	// XXX Set your values here
	if (mp_TrackingData != nullptr) {
		pos[0] = mp_TrackingData->tx;
		pos[1] = mp_TrackingData->ty;
		pos[2] = mp_TrackingData->tz;

		// the d_quat array contains the orientation value of the tracker, stored as a quaternion
		// XXX Set your values here
		d_quat[0] = mp_TrackingData->rx;
		d_quat[1] = mp_TrackingData->ry;
		d_quat[2] = mp_TrackingData->rz;
		d_quat[3] = mp_TrackingData->rw;
	}
	else {
		pos[0] = 0.0f;
		pos[1] = 0.0f;
		pos[2] = 0.0f;

		// the d_quat array contains the orientation value of the tracker, stored as a quaternion
		// XXX Set your values here
		d_quat[0] = 0.0f;
		d_quat[1] = 0.0f;
		d_quat[2] = 0.0f;
		d_quat[3] = 0.0f;
	}

	char msgbuf[1000];

	d_sensor = 0;

	int  len = vrpn_Tracker::encode_to(msgbuf);

	if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf,
		vrpn_CONNECTION_LOW_LATENCY))
	{
		fprintf(stderr, "can't write message: tossing\n");
	}

	server_mainloop();
}

void RealSenseTracker::setTrackingData(TrackingData inData) {
	if (mp_TrackingData != nullptr)
	{
		mp_TrackingData->tx = inData.tx;
		mp_TrackingData->ty = inData.ty;
		mp_TrackingData->tz = inData.tz;

		mp_TrackingData->rx = inData.rx;
		mp_TrackingData->ry = inData.ry;
		mp_TrackingData->rz = inData.rz;
		mp_TrackingData->rw = inData.rw;
	}
}

RealSenseTracker::~RealSenseTracker()
{
	delete mp_TrackingData;
	mp_TrackingData = nullptr;
}