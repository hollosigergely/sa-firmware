#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstring>
#include <inttypes.h>
#include <fcntl.h>
#include <istream>
#include <unistd.h>
#include <signal.h>
using namespace std;

map<string,vector<int64_t>> data;

bool is_running = true;
int data_idx = -1;
int sf_correction = 0;

int lineCount = 0;
std::string line;

#define READ_ERROR()    do { cerr << "Error at " <<  lineCount << ": " << line.c_str() << endl; } while(0);
#define WARN(str) do { cerr << str << " (line: " << lineCount << " | " << line << ")" << endl;  } while(0);
#define ERROR(str) do { cerr << str << " (line: " << lineCount << " | " << line << ")" << endl; exit(1); } while(0);

void start_new_sf(int sf_id) {
    static int last_sf_id = 0;

    data_idx++;

    data["sf_id"].resize(data_idx+1,-1);
    data["sf_id"][data_idx] = sf_id;

    if(sf_id < last_sf_id)
    {
        sf_correction++;
    }

    last_sf_id = sf_id;

    data["vsf_id"].resize(data_idx+1,-1);
    data["vsf_id"][data_idx] = sf_id + sf_correction * 256;

    if(data["vsf_id"][data_idx - 1] >= data["vsf_id"][data_idx])
    {
        WARN("vsf id warn");
    }
}

void insert_ts(const string& col, int sf_id, uint64_t ts, bool this_sf)
{
    if(this_sf)
    {
        //cerr << "insert (" << col << "): idx: " << sf_id << " size:" << data[col].size() << endl;

        if(data["sf_id"][data_idx] != sf_id)
        {
            WARN("wrong sf id - this: " << data["sf_id"][data_idx] << " != " << sf_id);
        }

        data[col].resize(data_idx+1, -1);
        data[col][data_idx] = ts;
    }
    else
    {
        //cerr << "insert (" << col << "): idx: " << (sf_id - 1) << " size:" << data[col].size() << endl;

        if(data["sf_id"].size() >= 2)
        {
            if(data["sf_id"][data_idx - 1] != (sf_id - 1))
            {
                WARN("wrong sf id - prev: " << data["sf_id"][data_idx-1] << " != " << (sf_id-1));
            }

            data[col].resize(data_idx, -1);
            data[col][data_idx - 1] = ts;
        }
    }

}

bool convert_ts(char* str, int64_t* i)
{
    if(strlen(str) != 12)
    {
        WARN("Ts length is wrong");
        return false;
    }

    if(sscanf(str, "0x%lX", i) != 1)
    {
        WARN("Wrong ts");
        return false;
    }

    return true;
}

void test_serial(const char* file) {
    uint8_t byte, last_byte;
    int fp = open(file,O_RDONLY);
    while(read(fp,&byte,sizeof(uint8_t)) == 1)
    {
        uint8_t res = byte - last_byte;
        if(res != 1)
        {
            printf("Error at: %d\n", last_byte);
        }
        last_byte = byte;
    }
}

void sigint_handler(int s) {
    is_running = false;
}

int main(int argc, char** argv)
{
    istream& file = cin;

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigint_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    int last_id = -1;
    while (std::getline(file, line) && is_running)
    {
        lineCount++;

        if(line.size() != 27)
        {
            WARN("line length != 27");
            continue;
        }

        int id;
        int src_id;
        int dst_id;
        int sf_id;
        int64_t ts;

        if(sscanf(line.c_str(),"%d %02d %02d %06d 0x%010lx",&id,&dst_id,&src_id,&sf_id,&ts) != 5)
        {
            READ_ERROR();
            continue;
        }

        //fprintf(stderr,"data (id: %d, dst: %d, src: %d, sf: %d, ts: %010lX\n", id, dst_id, src_id, sf_id, ts);

        if(last_id == 3)
        {
            start_new_sf(sf_id);
        }
        last_id = id;

        if(ts > 0xFFFFFFFFFFll)
        {
            cerr << "suspicious ts: " << lineCount << endl;
            exit(1);
        }

        stringstream ss;
        if(data_idx >= 0)
        {
            switch(id)
            {
            case 1:
                {
                    ss << "A" << src_id << ".tx";
                    insert_ts(ss.str(), sf_id, ts, true);
                }
                break;
            case 2:
                {
                    ss << "A" << src_id << ".T.rx";
                    insert_ts(ss.str(), sf_id, ts, true);
                }
                break;
            case 3:
                {
                    ss << "T.tx";
                    insert_ts(ss.str(), sf_id, ts, true);
                }
                break;
            case 4:
                {
                    ss << "T" << src_id << ".A" << dst_id << ".rx";
                    insert_ts(ss.str(), sf_id, ts, false);
                }
                break;
            case 5:
                {
                    ss << "A" << src_id << ".A" << dst_id << ".rx";
                    insert_ts(ss.str(), sf_id, ts, dst_id >= src_id);
                }
                break;
            }
        }
    }

    if(data.size() != 0)
    {
        cerr << "Dumping " << data.size() << " entries" << endl;

        cout << "";
        for(auto entry : data)
        {
            cout << entry.first << " ";
        }
        cout << endl;

        size_t idx = 0;
        while(1) {
            stringstream ss;
            for(auto entry : data)
            {
                if(idx > entry.second.size())
                    exit(0);

                ss << entry.second[idx] << " ";
            }
            cout << ss.str() << endl;
            idx++;
        }

    }
    return 0;
}
