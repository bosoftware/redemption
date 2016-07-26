/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2016
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestGCC
#include "system/redemption_unit_tests.hpp"

#define LOGNULL
//#define LOGPRINT

#include "core/RDP/clipboard.hpp"


BOOST_AUTO_TEST_CASE(TestFormatDataResponsePDU)
{

    RDPECLIP::FormatDataResponsePDU formatDataResponsePDU(true);


    {
        //  emit_fileList
        StaticOutStream<1024> ou_stream_fileList;

        uint8_t UTF16nameData[20];
        std::string nameUTF8("abcde.txt");
        int UTF16nameSize = ::UTF8toUTF16(reinterpret_cast<const uint8_t *>(nameUTF8.c_str()), UTF16nameData, nameUTF8.size() *2);
        std::string nameUTF16 = std::string(reinterpret_cast<char *>(UTF16nameData), UTF16nameSize);
        std::string namesList[] = { nameUTF16 };
        uint64_t sizesList[] = { 17 };
        int cItems = 1;

        formatDataResponsePDU.emit_fileList(ou_stream_fileList, namesList, sizesList, cItems);

        std::string out_data(reinterpret_cast<char *>(ou_stream_fileList.get_data()), 604);

        const uint8_t file_list_out_data[] =
            "\x05\x00\x01\x00\x54\x02\x00\x00\x01\x00\x00\x00\x64\x40\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x04\xb5\x9f\x37\xa0\xe2\xd1\x01\x00\x00\x00\x00"
            "\x11\x00\x00\x00\x61\x00\x62\x00\x63\x00\x64\x00\x65\x00\x2e\x00\x74\x00\x78\x00"
            "\x74\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00";

        std::string expected(reinterpret_cast<const char *>(file_list_out_data), 604);

        BOOST_CHECK_EQUAL(expected, out_data);

    }



    {
        // emit_metaFilePic
        StaticOutStream<1600> ou_stream_metaFilePic;

        int height=300;
        int width=400;
        int bpp=24;
        int data_lenght = height * width * 3;

        formatDataResponsePDU.emit_metaFilePic(ou_stream_metaFilePic, data_lenght, width, height, bpp);

        std::string out_data(reinterpret_cast<char *>(ou_stream_metaFilePic.get_data()), 130);

        const char metafilepic_out_data[] =
            "\x05\x00\x01\x00\xc2\x7e\x05\x00\x08\x00\x00\x00\x58\x29\x00\x00\x02\x1f\x00\x00"
            "\x01\x00\x09\x00\x00\x03\x5b\xbf\x02\x00\x00\x00\x41\xbf\x02\x00\x00\x00\x04\x00"
            "\x00\x00\x03\x01\x08\x00\x05\x00\x00\x00\x0c\x02\xd4\xfe\x90\x01\x05\x00\x00\x00"
            "\x0b\x02\x00\x00\x00\x00\x41\xbf\x02\x00\x41\x0b\x20\x00\xcc\x00\x2c\x01\x90\x01"
            "\x00\x00\x00\x00\xd4\xfe\x90\x01\x00\x00\x00\x00\x28\x00\x00\x00\x90\x01\x00\x00"
            "\xd4\xfe\xff\xff\x01\x00\x18\x00\x00\x00\x00\x00\x40\x7e\x05\x00\x00\x00\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

        std::string expected(reinterpret_cast<const char *>(metafilepic_out_data), 130);

        BOOST_CHECK_EQUAL(expected, out_data);

    }
}


BOOST_AUTO_TEST_CASE(TestMetaFilePicDescriptor)
{
    const char metafilepic_in_data[] =
        "\x05\x00\x01\x00\xc2\x7e\x05\x00\x08\x00\x00\x00\x58\x29\x00\x00\x02\x1f\x00\x00"
        "\x01\x00\x09\x00\x00\x03\x5b\xbf\x02\x00\x00\x00\x41\xbf\x02\x00\x00\x00\x04\x00"
        "\x00\x00\x03\x01\x08\x00\x05\x00\x00\x00\x0c\x02\xd4\xfe\x90\x01\x05\x00\x00\x00"
        "\x0b\x02\x00\x00\x00\x00\x41\xbf\x02\x00\x41\x0b\x20\x00\xcc\x00\x2c\x01\x90\x01"
        "\x00\x00\x00\x00\xd4\xfe\x90\x01\x00\x00\x00\x00\x28\x00\x00\x00\x90\x01\x00\x00"
        "\xd4\xfe\xff\xff\x01\x00\x18\x00\x00\x00\x00\x00\x40\x7e\x05\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

    InStream in_stream_metaFilePic(metafilepic_in_data, 130);
    RDPECLIP::MetaFilePicDescriptor mfpd;
    mfpd.receive(in_stream_metaFilePic);

    BOOST_CHECK_EQUAL(mfpd.recordSize, 180033);
    BOOST_CHECK_EQUAL(mfpd.type, 2881);
    BOOST_CHECK_EQUAL(mfpd.height, 300);
    BOOST_CHECK_EQUAL(mfpd.width, 400);
    BOOST_CHECK_EQUAL(mfpd.bpp, 24);
    BOOST_CHECK_EQUAL(mfpd.imageSize, 360000);
}


BOOST_AUTO_TEST_CASE(TestFileDescriptor)
{
    const char in_data[] =
            "\x64\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // d@..............
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // .... ...........
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1e\x9b\x65\x20\xb2\xc6\x01" // ...........e ...
            "\x00\x00\x00\x00\x5d\x1b\x00\x00\x45\x00\x75\x00\x6c\x00\x61\x00" // ....]...E.u.l.a.
            "\x2e\x00\x74\x00\x78\x00\x74\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ..t.x.t.........
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // ................
        ;
    InStream in_stream(in_data, sizeof(in_data) - 1);

    RDPECLIP::FileDescriptor file_descriptor;

    file_descriptor.receive(in_stream);

    //file_descriptor.log(LOG_INFO);

    BOOST_CHECK_EQUAL(sizeof(in_data) - 1, file_descriptor.size());

    char out_data[sizeof(in_data)];

    OutStream out_stream(out_data, sizeof(out_data));

    file_descriptor.emit(out_stream);
    //LOG(LOG_INFO, "out_stream_size=%u", (unsigned)out_stream.get_offset());
    //hexdump(out_stream.get_data(), out_stream.get_offset());

    //BOOST_CHECK_EQUAL(out_stream.get_offset(), in_stream.get_offset());
    BOOST_CHECK_EQUAL(0, memcmp(in_data, out_data, sizeof(in_data) - 1));
}




