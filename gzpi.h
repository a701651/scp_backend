#pragma once
#include"common.h"
using namespace std;
inline string gzip(const std::string& data) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);

    zs.avail_in = (uInt)data.size();
    zs.next_in = (Bytef*)data.data();

    size_t bound = deflateBound(&zs, (uLong)data.size());
    std::string out(bound, '\0');
    zs.avail_out = (uInt)bound;
    zs.next_out = (Bytef*)out.data();

    deflate(&zs, Z_FINISH);
    deflateEnd(&zs);

    out.resize(zs.total_out);
    return out;
}

inline string base64(const std::string& data) {
    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        uint32_t n = ((uint32_t)(uint8_t)data[i]) << 16;
        if (i + 1 < data.size()) n |= ((uint32_t)(uint8_t)data[i + 1]) << 8;
        if (i + 2 < data.size()) n |= (uint32_t)(uint8_t)data[i + 2];
        out.push_back(tbl[(n >> 18) & 0x3F]);
        out.push_back(tbl[(n >> 12) & 0x3F]);
        out.push_back(i + 1 < data.size() ? tbl[(n >> 6) & 0x3F] : '=');
        out.push_back(i + 2 < data.size() ? tbl[n & 0x3F] : '=');
    }
    return out;
}

inline string j2gzip(const std::string& json_str) {
    return base64(gzip(json_str));
}
