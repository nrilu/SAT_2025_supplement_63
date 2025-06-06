
#pragma once

#include "util/reverse_file_reader.hpp"
#include "lrat_line.hpp"
#include "serialized_lrat_line.hpp"
#include "util/assert.hpp"

class ReverseBinaryLratParser {

private:
    ReverseFileReader _reader;
    std::vector<char> _num_buffer;
    bool _exhausted = false;

    enum LineParseState {
        PARSING_ZERO, 
        PARSING_HINTS,
        PARSING_HINTS_MAYBE_ENDING, 
        PARSING_LITS_AND_ID,
        PARSING_LITS_AND_ID_MAYBE_ENDING
    } _state = PARSING_ZERO;

    LratLine _line;
    SerializedLratLine _sline;

    unsigned long _num_read_bytes = 0;

public:
    ReverseBinaryLratParser(const std::string& filename) : _reader(filename) {}

    bool getNextLine(SerializedLratLine& out) {

        out.clear();
        _line.literals.clear();
        _line.hints.clear();

        if (_exhausted || !_reader.valid()) return false;

        // Anatomy of a binary LRAT proof line:
        // a <id> <lit1> <lit2> <lit3> 0 <proof1> <proof2> 0

        constexpr char LINE_TYPE_ADD = 'a'; // 97 - 01100001

        char byte;
        bool readCompleteLine = false;
        while (!readCompleteLine) {

            // Fetching of next (previous) byte unsuccessful?
            if (!_reader.next(byte)) {
                if (_state == PARSING_LITS_AND_ID_MAYBE_ENDING) {
                    // File has been read completely!
                    // Finalize last read line
                    publishNumbers(PublishNumbersMode::LITERALS_THEN_ID);
                    readCompleteLine = true;
                }
                _exhausted = true;
                break;
            }
            _num_read_bytes++;

            LOG(V6_DEBGV, "HANDLE_BYTE %i tbc:%s\n", (int) *((unsigned char*)&byte), 
                isNonFinalByteOfNumber(byte) ? "yes":"no");

            // Process read byte
            switch (_state) {

            case PARSING_ZERO:
                assert(byte == 0);
                _state = PARSING_HINTS;
                break;

            case PARSING_HINTS:
                if (byte == 0) {
                    // Could be separator zero transitioning to literals
                    // OR final byte of a number.
                    // => Decided by the next byte (isNonFinalByteOfNumber)
                    _state = PARSING_HINTS_MAYBE_ENDING;
                } else {
                    // Handle a byte of a hint number
                    handleByteOfNumber(byte);
                }
                break;

            case PARSING_HINTS_MAYBE_ENDING:
                if (isNonFinalByteOfNumber(byte)) {
                    // Last read zero was not a separator but the ending of a number.
                    handleByteOfNumber(0);
                    _state = PARSING_HINTS;
                } else {
                    // Indeed transitioned to parsing literals / ID
                    publishNumbers(PublishNumbersMode::HINTS);
                    _state = PARSING_LITS_AND_ID;
                }
                handleByteOfNumber(byte);
                break;

            case PARSING_LITS_AND_ID:
                if (byte == LINE_TYPE_ADD) {
                    // Could be line type indicator transitioning to previous line
                    // OR final byte of a number.
                    // => Decided by the next byte (isNonFinalByteOfNumber)
                    _state = PARSING_LITS_AND_ID_MAYBE_ENDING;
                } else {
                    // Handle a byte of a literal or clause ID
                    handleByteOfNumber(byte);
                }
                break;

            case PARSING_LITS_AND_ID_MAYBE_ENDING:
                if (byte != 0 || isNonFinalByteOfNumber(byte)) {
                    // Last read 'a' was not a line type indicator but a number's ending.
                    handleByteOfNumber(LINE_TYPE_ADD);
                    _state = PARSING_LITS_AND_ID;
                    handleByteOfNumber(byte);
                } else {
                    // Indeed transitioned to the line's beginning.
                    // Finalize completely read line
                    publishNumbers(PublishNumbersMode::LITERALS_THEN_ID);
                    readCompleteLine = true;
                    // Begin to handle next (earlier) line
                    _state = PARSING_HINTS;
                }
                break;
            }
        }

        if (readCompleteLine) {
            _sline.reset(_line);
            _sline.data().swap(out.data());
            return true;
        } else {
            return false;
        }
    }

    unsigned long getNumReadBytes() const {
        return _num_read_bytes;
    }

private:

    void handleByteOfNumber(char byte) {
        // Last byte in a number (= first read byte) must be the final byte
        if (_num_buffer.empty()) 
            assert(!isNonFinalByteOfNumber(byte));
        // Append byte
        _num_buffer.push_back(byte);
    }

    enum PublishNumbersMode {HINTS, LITERALS_THEN_ID};
    void publishNumbers(PublishNumbersMode mode) {
        // The buffer contains the parsed bytes in reverse order.
        // => Traverse the featured NUMBERS from the buffer's end towards its start
        //    and handle the BYTES of each number from right to left.
        int numberRightIdx = _num_buffer.size()-1;
        assert(numberRightIdx >= 0);
        int i = numberRightIdx;
        bool firstNum = true;
        while (i >= 0) {
            // Done reading a number?
            if (i == 0 || !isNonFinalByteOfNumber(_num_buffer[i])) {
                int numberLeftIdx = i;

                if (mode == HINTS) {
                    // Publish hint
                    auto hint = readSignedClauseId(numberLeftIdx, numberRightIdx);
                    LOG(V6_DEBGV, "PUSH_HINT %ld [%i,%i]\n", hint, numberLeftIdx, numberRightIdx);
                    _line.hints.push_back(hint);
                } else if (firstNum) {
                    // Publish ID
                    auto id = readSignedClauseId(numberLeftIdx, numberRightIdx);
                    assert(id > 0);
                    LOG(V6_DEBGV, "PUSH_ID %ld [%i,%i]\n", id, numberLeftIdx, numberRightIdx);
                    _line.id = id;
                    firstNum = false;
                } else {
                    // Publish literal
                    auto lit = readLiteral(numberLeftIdx, numberRightIdx);
                    LOG(V6_DEBGV, "PUSH_LIT %i [%i,%i]\n", lit, numberLeftIdx, numberRightIdx);
                    _line.literals.push_back(lit);
                }

                numberRightIdx = i-1;
            }
            i--;
        }
        _num_buffer.clear();
    }

    bool isNonFinalByteOfNumber(char byte) {
        unsigned char uByte = *((unsigned char*) &byte);
        return uByte & 0b10000000;
    }

    // Read a signed literal int64 in the variable-length encoding of binary DRAT/LRAT
    // https://github.com/marijnheule/drat-trim#binary-drat-format
    // Adapted from code by Dawn Michaelson
    int64_t readSignedClauseId(int leftIdx, int rightIdx) {
        int64_t unadjusted = 0;
        int64_t coefficient = 1;
        for (int i = rightIdx; i >= leftIdx; i--) {
            int32_t tmp = *((unsigned char*) (_num_buffer.data()+i));
            // continuation bit set?
            if (tmp & 0b10000000) {
                unadjusted += coefficient * (tmp & 0b01111111); // remove first bit
            } else {
                // last byte
                unadjusted += coefficient * tmp; // first bit is 0, so can leave it
            }
            coefficient *= 128; // 2^7 because we essentially have 7-bit bytes
        }
        int64_t id;
        if (unadjusted % 2) { // odds map to negatives
            id = -(unadjusted - 1) / 2;
        } else {
            id = unadjusted / 2;
        }
        return id;
    }

    // Read a signed literal in the variable-length encoding of binary DRAT/LRAT
    // https://github.com/marijnheule/drat-trim#binary-drat-format
    // Adapted from code by Dawn Michaelson
    int32_t readLiteral(int leftIdx, int rightIdx) {
        int32_t unadjusted = 0;
        int32_t coefficient = 1;
        for (int i = rightIdx; i >= leftIdx; i--) {
            int32_t tmp = *((unsigned char*) (_num_buffer.data()+i));
            // continuation bit set?
            if (tmp & 0b10000000) {
                unadjusted += coefficient * (tmp & 0b01111111); // remove first bit
            } else {
                // last byte
                unadjusted += coefficient * tmp; // first bit is 0, so can leave it
            }
            coefficient *= 128; // 2^7 because we essentially have 7-bit bytes
        }
        int32_t lit;
        if (unadjusted % 2) { // odds map to negatives
            lit = -(unadjusted - 1) / 2;
        } else {
            lit = unadjusted / 2;
        }
        return lit;
    }
};
