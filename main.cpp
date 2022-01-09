#include <dirent.h>
#include <stdio.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>

typedef class Signal Signal;

#define FFF_HEADER  "Filetype: IR signals file"
#define FFF_VERSION "Version: 1"

class Signal {
public:
    std::string name;
    std::string protocol;
    bool raw;
    uint32_t frequency;
    float duty_cycle;
    std::vector<uint32_t> samples;
    uint32_t address;
    uint32_t command;
};

static bool stream_eof(std::istringstream& ss) {
    if (ss.bad()) {
        ss.clear();
        std::cerr << "string stream is bad" << std::endl;
        return false;
    }

    return ss.eof();
}

void print_signal_to_file(std::ofstream& ofs, const Signal& signal) {
    ofs << "# " << std::endl;
    ofs << "name: " << signal.name << std::endl;
    if (signal.raw) {
        ofs << "type: raw" << std::endl;
        ofs << "frequency: " << signal.frequency << std::endl;
        ofs << "duty_cycle: " << signal.duty_cycle << std::endl;
        ofs << "data: ";
        for(const auto& sample : signal.samples) {
            ofs << std::dec << sample << ' ';
        }
        ofs << std::endl;
    } else {
        ofs << "type: parsed" << std::endl;
        ofs << "protocol: " << signal.protocol << std::endl;
        ofs << "address: ";
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&signal.address);
        for (int i = 0; i < sizeof(uint32_t); ++i) {
            std::stringstream ss;
            ss << std::setfill ('0') << std::setw(2) << std::hex << int(ptr[i]);
            std::string str = ss.str();
            ofs << str << ' ';
        }
        ofs << std::endl;

        ofs << "command: ";
        ptr = reinterpret_cast<const uint8_t*>(&signal.command);
        for (int i = 0; i < sizeof(uint32_t); ++i) {
            std::stringstream ss;
            ss << std::setfill ('0') << std::setw(2) << std::hex << int(ptr[i]);
            std::string str = ss.str();
            ofs << str << ' ';
        }
        ofs << std::endl;
    }

}

int main(int argc, char *argv[])
{
#if 1
    if (argc != 2) {
        fprintf(stderr, "program takes 1 argument - directory to .ir files (non-recursive processing)\nUsage:\n\t./reformat /path/to/ir/files\n");
        return -1;
    }
    const char* path = argv[1];
#else
    const char* path = "./test";
#endif
    DIR *d = opendir(path);
    if (!d) {
        fprintf(stderr, "can't open dir \'%s\'\n", path);
        return -2;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type != DT_REG) continue;
        std::string line;
        std::string filename = std::string(path) + "/" + dir->d_name;

        std::vector<Signal> signals;
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            std::cerr << "can't open file for reading" << filename << std::endl;
            return -3;
        }

        if(std::getline(ifs, line)) {
            if (!line.compare(FFF_HEADER)) {
                std::cout << "already formatted file " << filename << std::endl;
                continue;
            }
            ifs.seekg(0, std::ios::beg);
        } else {
            continue;
        }

        while(std::getline(ifs, line)) {
            Signal signal;
            if (line.empty()) continue;
            if (line[0] == '/') continue;
//            std::cout << "parse \'" << line <<"\'" << std::endl;

            std::istringstream iss(line);
            std::string str;

            iss >> signal.name;
            if (stream_eof(iss)) continue;

            iss >> signal.protocol;
            if (stream_eof(iss)) continue;

            if (signal.name.empty() || signal.protocol.empty()) {
                continue;
            } else if (signal.protocol == "RAW") {
                signal.raw = true;
                iss >> str;
                if (stream_eof(iss)) continue;
                if (1 != sscanf(str.c_str(), "F:%u", &signal.frequency)) continue;

                iss >> str;
                if (stream_eof(iss)) continue;
                if (1 != sscanf(str.c_str(), "DC:%f", &signal.duty_cycle)) continue;
                signal.duty_cycle /= 100;

                while(!stream_eof(iss)) {
                    uint32_t value;
                    iss >> value;
                    if (iss.fail() || iss.bad()) {
                        iss.clear();
                        signal.samples.clear();
                        break;
                    }
                    signal.samples.push_back(value);
                }
                if (signal.samples.empty()) continue;

//                printf("raw: f: %u, dc: %f, samples: %u...(%ld)\n", signal.frequency, signal.duty_cycle, signal.samples[0], signal.samples.size());
            } else {
                signal.raw = false;
                iss >> str;
                if (stream_eof(iss)) continue;
                if (1 != sscanf(str.c_str(), "A:%X", &signal.address)) continue;

                iss >> str;
                if (!stream_eof(iss)) continue;   /* has to be last */
                if (1 != sscanf(str.c_str(), "C:%X", &signal.command)) continue;

//                printf("parsed: protocol: %s, adr: %X, cmd: %X\n", signal.protocol.c_str(), signal.address, signal.command);
            }

            signals.push_back(signal);
        }
        ifs.close();

        std::ofstream ofs(filename);
        ofs << FFF_HEADER << std::endl;
        ofs << FFF_VERSION << std::endl;

        if (!ofs.is_open()) {
            std::cerr << "can't open file for writing" << filename << std::endl;
            return -4;
        }

        std::cout << "Reformat file: " << filename << std::endl;
        for (const auto& s : signals) {
            print_signal_to_file(ofs, s);
        }

        ofs.close();
    }
    closedir(d);

    return 0;
}

