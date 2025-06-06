
#pragma once

#include "util/sys/buffered_io.hpp"
#include <fstream>

#define MALLOB_REVERSE_READER_BUF_SIZE 65536

class ReverseFileReader : public LinearFileReader {

private:
    std::ifstream _stream;
    size_t _file_size;
    bool _valid {true};

    char _buffer[MALLOB_REVERSE_READER_BUF_SIZE];
    int _buffer_pos = -1;

public:
    ReverseFileReader() : LinearFileReader() {}
    ReverseFileReader(const std::string& filename) : LinearFileReader(),
            _stream(filename, std::ios_base::ate | std::ios_base::binary) {
        if (_stream.good()) {
            _file_size = _stream.tellg();
        } else {
            _valid = false;
        }
    }

    virtual bool endOfFile() override {
        return !valid() || done();
    }    

    bool valid() const {
        return _valid;
    }

    inline bool next(char& c) {
        if (!_valid) return false;
        if (_buffer_pos < 0) {
            refillBuffer();
            if (_buffer_pos < 0) return false;
        }
        c = _buffer[_buffer_pos];
        _buffer_pos--;
        return true;
    }

    inline virtual char next() override {
        char c;
        if (!next(c)) return '\0';
        return c;
    }

    inline bool done() {
        if (!_valid) return false;
        if (_buffer_pos >= 0) return false;
        refillBuffer();
        return _buffer_pos < 0;
    }

private:
    inline void refillBuffer() {
        if (!_valid || !_stream.good()) return;
        
        // Check by how far you can go back
        auto sizeBefore = _file_size;
        _file_size = std::max(0LL, ((long long) sizeBefore) - MALLOB_REVERSE_READER_BUF_SIZE);
        int numDesired = sizeBefore - _file_size;

        // Go back and read the corresponding chunk of data
        _stream.seekg(_file_size, std::ios_base::beg);
        _stream.read(_buffer, numDesired);

        // Check how much has been read
        if (_stream.eof()) _buffer_pos = _stream.gcount()-1;
        else _buffer_pos = numDesired-1;
    }
};
