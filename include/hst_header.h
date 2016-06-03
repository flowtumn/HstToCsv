#ifndef HST_HEADER_H_INCLUDE__
#define HST_HEADER_H_INCLUDE__

#include <ctime>

typedef struct _tag_HistoryHeader {
	int version; // version of the base
	char copyright[64]; // copyright information
	char symbol[12]; // security
	int period; // security period
	int digits; // the amount of digits after point shown for the symbol
	uint32_t timesign; // timesign of the base creation
	uint32_t last_sync; // last synchronization time
	int unused[13]; // for future use
} HistoryHeader;

#pragma pack(push,1)
typedef struct _tag_RateInfo {
	uint32_t ctm; // current time in seconds
	double open;
	double low;
	double high;
	double close;
	double vol;
} RateInfo;
#pragma pack(pop)

#endif
