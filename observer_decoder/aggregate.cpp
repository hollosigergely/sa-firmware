#include "aggregate.h"
#include "timing.h"
#include <sstream>
using namespace std;

#define TAG "aggr"

Aggregate::Aggregate()
{
	this->mPacketID = 0;
	this->mData_idx = -1;
	this->mLastSFSlot = TIMING_ANCHOR_COUNT + TIMING_TAG_COUNT - 1;
	this->mLastSFStartTs.ts = 0;
}

void Aggregate::start_new_sf(int sf_id, dwm1000_ts_t rx_ts)
{
	mData_idx++;

	mData["sf_id"].resize(mData_idx+1,-1);
	mData["sf_id"][mData_idx] = sf_id;

	mData["vsf_id"].resize(mData_idx+1,-1);
	mData["vsf_id"][mData_idx] = mData_idx;

	uint32_t rx_ts_32 = rx_ts.ts >> 8;
	uint32_t last_ts_32 = mLastSFStartTs.ts >> 8;

	uint32_t duration = rx_ts_32 - last_ts_32;
	if(mLastSFStartTs.ts != 0 && (duration * 4e-6) > (1.5 * TIMING_SUPERFRAME_LENGTH_MS))
	{
		WARN(TAG,"superframe skip warning: " << duration);
	}

	mLastSFStartTs = rx_ts;

	/*if(mData["vsf_id"][mData_idx - 1] >= mData["vsf_id"][mData_idx])
	{
		WARN(TAG,"vsf id warn");
	}*/
}

int Aggregate::get_sf_slot(uint8_t *buffer)
{
	sf_header_t* hdr = (sf_header_t*)buffer;
	switch(hdr->fctrl)
	{
	case SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE:
		{
			sf_anchor_msg_t* msg = (sf_anchor_msg_t*)buffer;

			return msg->hdr.src_id;
		}
	case SF_HEADER_FCTRL_MSG_TYPE_TAG_MESSAGE:
		{
			sf_tag_msg_t* msg = (sf_tag_msg_t*)buffer;

			return TIMING_ANCHOR_COUNT + msg->hdr.src_id;
		}
	}

	return -1;
}

void Aggregate::insert_ts(const string& col, int sf_id, uint64_t ts, bool this_sf)
{
	if(this_sf)
	{
		//cerr << "insert (" << col << "): idx: " << sf_id << " size:" << data[col].size() << endl;

		/*if(mData["sf_id"][mData_idx] != sf_id)
		{
			WARN(TAG,"wrong sf id - this: " << mData["sf_id"][mData_idx] << " != " << sf_id);
		}*/

		mData[col].resize(mData_idx+1, -1);
		mData[col][mData_idx] = ts;
	}
	else
	{
		//cerr << "insert (" << col << "): idx: " << (sf_id - 1) << " size:" << data[col].size() << endl;

		if(mData["sf_id"].size() >= 2)
		{
			/*if(mData["sf_id"][mData_idx - 1] != (sf_id - 1))
			{
				WARN(TAG, "wrong sf id - prev: " << mData["sf_id"][mData_idx-1] << " != " << (sf_id-1));
			}*/

			mData[col].resize(mData_idx, -1);
			mData[col][mData_idx - 1] = ts;
		}
	}

}

void Aggregate::dump(dwm1000_ts_t rx_ts, uint8_t *buffer, size_t length)
{
	mPacketID++;

	int sfSlot = get_sf_slot(buffer);
	if(sfSlot < 0)
	{
		WARN(TAG,"slot < 0");
		return;
	}

	if(sfSlot <= mLastSFSlot)
	{
		start_new_sf(0, rx_ts);
	}

	sf_header_t* hdr = (sf_header_t*)buffer;
	switch(hdr->fctrl)
	{
	case SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE:
		{
			sf_anchor_msg_t* msg = (sf_anchor_msg_t*)buffer;

			if(mData_idx >= 0)
			{
				stringstream ss;
				ss << "A" << msg->hdr.src_id << ".tx";
				insert_ts(ss.str(), msg->tr_id, Utils::dwm1000_pu8_to_ts(msg->tx_ts).ts, true);

				for(int aidx = 0; aidx < TIMING_ANCHOR_COUNT; aidx++)
				{
					dwm1000_ts_t ts = Utils::dwm1000_pu8_to_ts(msg->anchors[aidx].rx_ts);

					if(ts.ts != 0)
					{
						stringstream ss;
						ss << "A" << aidx << ".A" << msg->hdr.src_id << ".rx";
						insert_ts(ss.str(), msg->tr_id, ts.ts, msg->hdr.src_id >= aidx);
					}
				}
			}
		}
		break;
	case SF_HEADER_FCTRL_MSG_TYPE_TAG_MESSAGE:
		{

		}
		break;
	}

	mLastSFSlot = sfSlot;
}

void Aggregate::end()
{
	if(mData.size() != 0)
	{
		cerr << "Dumping " << mData.size() << " variables and " << mData_idx << " rows" << endl;

		cout << "";
		for(auto entry : mData)
		{
			cout << entry.first << " ";
		}
		cout << endl;

		size_t idx = 0;
		while(idx < mData_idx) {
			stringstream ss;
			for(auto entry : mData)
			{
				ss << entry.second[idx] << " ";
			}
			cout << ss.str() << endl;
			idx++;
		}

	}
}

