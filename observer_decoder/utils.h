#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <iostream>

#define WARN(TAG,x)        (cerr << "[" TAG "] " << x << endl)

typedef union {
    uint64_t    ts;
    uint8_t     ts_bytes[5];

    uint32_t    ts_low_32;
} dwm1000_ts_t;

class Utils
{
public:
	Utils();

        static inline void dwm1000_ts_to_pu8(dwm1000_ts_t ts, uint8_t* p8)
        {
            for(int i = 0; i < 5; i++)
                p8[i] = ts.ts_bytes[i];
        }

        static inline dwm1000_ts_t dwm1000_pu8_to_ts(const uint8_t* p8)
        {
            dwm1000_ts_t _ts = { 0 };
            for(int i = 0; i < 5; i++)
                _ts.ts_bytes[i] = p8[i];
            return _ts;
        }

        static inline void dwm1000_ts_u64_to_pu8(uint64_t ts, uint8_t* p8)
        {
            dwm1000_ts_t _ts = { 0 };
            _ts.ts = ts;

            dwm1000_ts_to_pu8(_ts,p8);
        }
};

#endif // UTILS_H
