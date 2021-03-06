/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2012
   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen, Meng Tan

   Transport layer abstraction
*/


#pragma once

#include <new>
#include <memory>
#include <stdexcept>
#include <sstream>

#include <fstream>
#include <sstream>
#include <string>

#include <algorithm>
#include <cstring>

#include "system/redemption_unit_tests.hpp"
#include "transport/transport.hpp"
#include "utils/log.hpp"
#include "utils/stream.hpp"
#include "utils/hexdump.hpp"

struct RemainingError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

namespace redemption_unit_test__
{
    struct hexdump
    {
        const_byte_array sig;

        std::size_t size() const
        {
            return this->sig.size();
        }

        uint8_t const * data() const
        {
            return this->sig.data();
        }
    };

    struct hexdump_with_remaining
    {
        char const * type;
        uint8_t const * p;
        std::size_t cur;
        std::size_t len;

        std::size_t size() const
        {
            return this->len - this->cur;
        }

        uint8_t const * data() const
        {
            return p;
        }
    };

    inline std::ostream & operator<<(std::ostream & out, hexdump const & x)
    {
        char buffer[2048];
        for (size_t j = 0 ; j < x.size(); j += 16){
            char * line = buffer;
            line += std::sprintf(line, "/* %.4x */ \"", static_cast<unsigned>(j));
            size_t i = 0;
            for (i = 0; i < 16; i++){
                if (j+i >= x.size()){ break; }
                line += std::sprintf(line, "\\x%.2x", static_cast<unsigned>(x.data()[j+i]));
            }
            line += std::sprintf(line, "\"");
            if (i < 16){
                line += std::sprintf(line, "%*c", static_cast<int>((16-i)*4), ' ');
            }
            line += std::sprintf(line, " // ");
            for (i = 0; i < 16; i++){
                if (j+i >= x.size()){ break; }
                unsigned char tmp = x.data()[j+i];
                if ((tmp < ' ') || (tmp > '~') || (tmp == '\\')){
                    tmp = '.';
                }
                line += std::sprintf(line, "%c", tmp);
            }

            if (line != buffer){
                line[0] = 0;
                out << buffer << "\n";
                buffer[0]=0;
            }
        }
        return out;
    }

    inline std::ostream & operator<<(std::ostream & out, hexdump_with_remaining const & x)
    {
        return out
            << hexdump{{x.data(), x.size()}} << '\n'
            << "~" << x.type << "() remaining=" << x.size() << " len=" << x.len
        ;
    }
}

struct GeneratorTransport : Transport
{
    GeneratorTransport(const void * data, size_t len)
    : len(len)
    {
        this->data.reset(new(std::nothrow) uint8_t[len]);
        if (!this->data) {
            throw Error(ERR_TRANSPORT_OPEN_FAILED);
        }
        if (data) {
            memcpy(this->data.get(), data, len);
        }
    }

    ~GeneratorTransport()
    {
        this->disconnect();
    }

    void disable_remaining_error()
    {
        this->remaining_is_error = false;
    }

    bool disconnect() override
    {
        if (this->remaining_is_error && this->len != this->current) {
            this->remaining_is_error = false;
            RED_CHECK_MESSAGE(this->len == this->current, (
                redemption_unit_test__::hexdump_with_remaining{
                    "GeneratorTransport", this->data.get(), this->current, this->len
                })
            );
            std::ostringstream out;
            out << "~GeneratorTransport() remaining=" << (this->len-this->current) << " len=" << this->len;
            throw RemainingError{out.str()};
        }
        return true;
    }

private:
    Read do_atomic_read(uint8_t * buffer, size_t len) override
    {
        size_t const remaining = this->len - this->current;
        if (!remaining) {
            return Read::Eof;
        }
        if (len > remaining) {
            throw Error(ERR_TRANSPORT_READ_FAILED);
        }
        memcpy(buffer, this->data.get() + this->current, len);
        this->current += len;
        return Read::Ok;
    }

    size_t do_partial_read(uint8_t* buffer, size_t len) override
    {
        size_t const remaining = this->len - this->current;
        if (!remaining) {
            return 0;
        }
        len = std::min(len, remaining);
        memcpy(buffer, this->data.get() + this->current, len);
        this->current += len;
        return len;
    }

    void do_send(const uint8_t * const buffer, size_t len) override
    {
        LOG(LOG_INFO, "Sending on target (-1) %zu bytes", len);
        hexdump_c(buffer, len);
        LOG(LOG_INFO, "Sent dumped on target (-1) %zu bytes", len);
    }

    std::unique_ptr<uint8_t[]> data;
    std::size_t len = 0;
    std::size_t current = 0;
    bool remaining_is_error = true;
};


struct BufTransport : Transport
{
    std::string buf;

private:
    void do_send(const uint8_t * const data, size_t len) override
    {
        this->buf.append(reinterpret_cast<char const *>(data), len);
    }
};


struct CheckTransport : Transport
{
    CheckTransport(const char * data, size_t len)
    : data(new(std::nothrow) uint8_t[len])
    , len(len)
    , current(0)
    {
        if (!this->data) {
            throw Error(ERR_TRANSPORT, 0);
        }
        memcpy(this->data.get(), data, len);
    }

    void disable_remaining_error()
    {
        this->remaining_is_error = false;
    }

    ~CheckTransport() override {
        this->disconnect();
    }

    bool disconnect() override
    {
        if (this->remaining_is_error && this->len != this->current) {
            this->remaining_is_error = false;
            RED_CHECK_MESSAGE(this->len == this->current, "check remaining == 0 failed\n" << (
                redemption_unit_test__::hexdump_with_remaining{
                    "CheckTransport", this->data.get(), this->current, this->len
                })
            );
            std::ostringstream out;
            out << "~CheckTransport() remaining=" << (this->len-this->current) << " len=" << this->len;
            throw RemainingError{out.str()};
        }
        return true;
    }

private:
    void do_send(const uint8_t * const data, size_t len) override
    {
        const size_t available_len = std::min<size_t>(this->len - this->current, len);
        if (0 != memcmp(data, this->data.get() + this->current, available_len)){
            // data differs, find where
            uint32_t differs = 0;
            while (differs < available_len && data[differs] == this->data.get()[this->current+differs]) {
                ++differs;
            }
            RED_CHECK_MESSAGE(false, "\n"
                "=============== Common Part =======\n" <<
                (redemption_unit_test__::hexdump{{data, differs}}) <<
                "=============== Expected =======\n" <<
                (redemption_unit_test__::hexdump{{this->data.get() + this->current + differs, available_len - differs}}) <<
                "=============== Got =======\n" <<
                (redemption_unit_test__::hexdump{{data + differs, available_len - differs}})
            );
            this->data.reset();
            this->remaining_is_error = false;
            throw Error(ERR_TRANSPORT_DIFFERS);
        }

        this->current += available_len;

        if (available_len != len){
            RED_CHECK_MESSAGE(available_len == len,
                "check transport out of reference data available="
                    << available_len << " len=" << len << " failed\n"
                "=============== Common Part =======\n" <<
                (redemption_unit_test__::hexdump{{data, available_len}}) <<
                "=============== Got Unexpected Data =======\n" <<
                (redemption_unit_test__::hexdump{{data + available_len, len - available_len}})
            );
            this->data.reset();
            this->remaining_is_error = false;
            throw Error(ERR_TRANSPORT_DIFFERS);
        }
    }

    std::unique_ptr<uint8_t[]> data;
    std::size_t len;
    std::size_t current;
    bool remaining_is_error = true;
};


struct TestTransport : public Transport
{
    TestTransport(
        const char * indata, size_t inlen,
        const char * outdata, size_t outlen)
    : check(outdata, outlen)
    , gen(indata, inlen)
    , public_key_length(0)
    {}

    void disable_remaining_error()
    {
        this->check.disable_remaining_error();
        this->gen.disable_remaining_error();
    }

    void set_public_key(const uint8_t * data, size_t data_size)
    {
        this->public_key.reset(new uint8_t[data_size]);
        this->public_key_length = data_size;
        memcpy(this->public_key.get(), data, data_size);
    }

    const uint8_t * get_public_key() const override
    {
        return this->public_key.get();
    }

    size_t get_public_key_length() const override
    {
        return this->public_key_length;
    }

private:
    Read do_atomic_read(uint8_t * buffer, size_t len) override
    {
        return this->gen.atomic_read(buffer, len);
    }

    size_t do_partial_read(uint8_t* buffer, size_t len) override
    {
        return this->gen.partial_read(buffer, len);
    }

    void do_send(const uint8_t * const buffer, size_t len) override
    {
        this->check.send(buffer, len);
    }

    CheckTransport check;
    GeneratorTransport gen;
    std::unique_ptr<uint8_t[]> public_key;
    std::size_t public_key_length;
};


class LogTransport : public Transport
{
    void do_send(const uint8_t * const buffer, size_t len) override
    {
        LOG(LOG_INFO, "Sending on target (-1) %zu bytes", len);
        hexdump_c(buffer, len);
        LOG(LOG_INFO, "Sent dumped on target (-1) %zu bytes", len);
    }
};


class MemoryTransport : public Transport
{
    uint8_t buf[65536];
    bool remaining_is_error = true;

public:
    InStream    in_stream{buf};
    OutStream   out_stream{buf};

    ~MemoryTransport()
    {
        this->disconnect();
    }

    void disable_remaining_error()
    {
        this->remaining_is_error = false;
    }

    bool disconnect() override
    {
        if (this->remaining_is_error && this->in_stream.get_offset() != this->out_stream.get_offset()) {
            std::ostringstream out;
            out << "~MemoryTransport() remaining=" << this->in_stream.get_offset() << " len=" << this->out_stream.get_offset();
            throw RemainingError{out.str()};
        }
        return true;
    }

private:
    Read do_atomic_read(uint8_t * buffer, size_t len) override
    {
        auto const in_offset = this->in_stream.get_offset();
        auto const out_offset = this->out_stream.get_offset();
        if (in_offset == out_offset){
            return Read::Eof;
        }
        if (in_offset + len > out_offset){
            throw Error(ERR_TRANSPORT_READ_FAILED);
        }
        this->in_stream.in_copy_bytes(buffer, len);
        return Read::Ok;
    }

    size_t do_partial_read(uint8_t* buffer, size_t len) override
    {
        auto const in_offset = this->in_stream.get_offset();
        auto const out_offset = this->out_stream.get_offset();
        if (in_offset == out_offset){
            return 0;
        }
        len = std::min(out_offset - out_offset, len);
        this->in_stream.in_copy_bytes(buffer, len);
        return len;
    }

    void do_send(const uint8_t * const buffer, size_t len) override
    {
        if (len > this->out_stream.tailroom()) {
            throw Error(ERR_TRANSPORT_WRITE_FAILED);
        }
        this->out_stream.out_copy_bytes(buffer, len);
    }
};
