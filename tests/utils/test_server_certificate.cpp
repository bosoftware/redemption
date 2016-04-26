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
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Meng Tan

*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestXXX
#include <boost/test/auto_unit_test.hpp>

#define LOGPRINT

#include <stdio.h>
#include "system/ssl_calls.hpp"

// uint8_t data[512];

// UdevRandom udevRand;

// udevRand.random(data, sizeof(data));
// hexdump_d(data, sizeof(data), 8);


BOOST_AUTO_TEST_CASE(TestReadServer)
{

uint8_t data[] = {
    // 00000000
     0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x43, 0x45, 0x52, 0x54, 0x49,
       // |-----BEGIN CERTI|
     0x46, 0x49, 0x43, 0x41, 0x54, 0x45, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a, 0x4d, 0x49, 0x49, 0x43,
       // |FICATE-----.MIIC|
     0x34, 0x6a, 0x43, 0x43, 0x41, 0x63, 0x71, 0x67, 0x41, 0x77, 0x49, 0x42, 0x41, 0x67, 0x49, 0x51,
       // |4jCCAcqgAwIBAgIQ|
     0x65, 0x5a, 0x46, 0x12, 0x6f, 0x32, 0x55, 0x50, 0x44, 0x34, 0x64, 0x44, 0x4b, 0x61, 0x6b, 0x2b,
       // |eZF2o2UPD4dDKak+|
     0x55, 0x72, 0x34, 0x66, 0x41, 0x6a, 0x41, 0x4e, 0x42, 0x67, 0x6b, 0x71, 0x68, 0x6b, 0x69, 0x47,
       // |Ur4fAjANBgkqhkiG|
     0x39, 0x77, 0x30, 0x42, 0x41, 0x51, 0x55, 0x46, 0x41, 0x44, 0x41, 0x61, 0x0a, 0x4d, 0x52, 0x67,
       // |9w0BAQUFADAa.MRg|
     0x77, 0x46, 0x67, 0x59, 0x44, 0x56, 0x51, 0x51, 0x44, 0x45, 0x77, 0x39, 0x58, 0x53, 0x55, 0x34,
       // |wFgYDVQQDEw9XSU4|
     0x74, 0x4d, 0x56, 0x49, 0x31, 0x53, 0x6b, 0x64, 0x44, 0x56, 0x55, 0x6b, 0x32, 0x56, 0x45, 0x73,
       // |tMVI1SkdDVUk2VEs|
     0x77, 0x48, 0x68, 0x63, 0x4e, 0x4d, 0x54, 0x55, 0x77, 0x4f, 0x54, 0x49, 0x30, 0x4d, 0x54, 0x51,
       // |wHhcNMTUwOTI0MTQ|
     0x31, 0x4f, 0x54, 0x45, 0x34, 0x57, 0x68, 0x63, 0x4e, 0x4d, 0x54, 0x59, 0x77, 0x0a, 0x4d, 0x7a,
       // |1OTE4WhcNMTYw.Mz|
     0x49, 0x31, 0x4d, 0x54, 0x51, 0x31, 0x4f, 0x54, 0x45, 0x34, 0x57, 0x6a, 0x41, 0x61, 0x4d, 0x52,
       // |I1MTQ1OTE4WjAaMR|
     0x67, 0x77, 0x46, 0x67, 0x59, 0x44, 0x56, 0x51, 0x51, 0x44, 0x45, 0x77, 0x39, 0x58, 0x53, 0x55,
       // |gwFgYDVQQDEw9XSU|
     0x34, 0x74, 0x4d, 0x56, 0x49, 0x31, 0x53, 0x6b, 0x64, 0x44, 0x56, 0x55, 0x6b, 0x32, 0x56, 0x45,
       // |4tMVI1SkdDVUk2VE|
     0x73, 0x77, 0x67, 0x67, 0x45, 0x69, 0x4d, 0x41, 0x30, 0x47, 0x43, 0x53, 0x71, 0x47, 0x0a, 0x53,
       // |swggEiMA0GCSqG.S|
     0x49, 0x62, 0x33, 0x44, 0x51, 0x45, 0x42, 0x41, 0x51, 0x55, 0x41, 0x41, 0x34, 0x49, 0x42, 0x44,
       // |Ib3DQEBAQUAA4IBD|
     0x77, 0x41, 0x77, 0x67, 0x67, 0x45, 0x4b, 0x41, 0x6f, 0x49, 0x42, 0x41, 0x51, 0x43, 0x75, 0x6a,
       // |wAwggEKAoIBAQCuj|

    // 00000100
     0x64, 0x52, 0x66, 0x6c, 0x77, 0x37, 0x30, 0x38, 0x6a, 0x5a, 0x55, 0x4a, 0x66, 0x4b, 0x78, 0x33, 
      // |dRflw708jZUJfKx3|
     0x76, 0x35, 0x74, 0x63, 0x35, 0x37, 0x55, 0x66, 0x4c, 0x71, 0x67, 0x4c, 0x32, 0x44, 0x39, 0x0a, 
      // |v5tc57UfLqgL2D9.|
     0x70, 0x7a, 0x4b, 0x75, 0x36, 0x41, 0x4a, 0x55, 0x63, 0x52, 0x53, 0x58, 0x63, 0x45, 0x30, 0x45, 
      // |pzKu6AJUcRSXcE0E|
     0x50, 0x70, 0x5a, 0x35, 0x51, 0x4a, 0x78, 0x62, 0x76, 0x50, 0x46, 0x6a, 0x79, 0x6d, 0x30, 0x75, 
      // |PpZ5QJxbvPFjym0u|
     0x31, 0x54, 0x79, 0x56, 0x76, 0x47, 0x55, 0x37, 0x34, 0x4e, 0x58, 0x71, 0x43, 0x54, 0x52, 0x46, 
      // |1TyVvGU74NXqCTRF|
     0x46, 0x58, 0x49, 0x6a, 0x52, 0x56, 0x69, 0x43, 0x37, 0x4f, 0x35, 0x4a, 0x54, 0x53, 0x71, 0x4d, 
      // |FXIjRViC7O5JTSqM|
     0x0a, 0x77, 0x73, 0x2f, 0x34, 0x44, 0x4c, 0x56, 0x63, 0x51, 0x4a, 0x4d, 0x49, 0x59, 0x55, 0x77, 
      // |.ws/4DLVcQJMIYUw|
     0x63, 0x38, 0x55, 0x34, 0x30, 0x6b, 0x77, 0x67, 0x41, 0x46, 0x56, 0x68, 0x66, 0x4f, 0x44, 0x4a, 
      // |c8U40kwgAFVhfODJ|
     0x2f, 0x79, 0x74, 0x6d, 0x32, 0x33, 0x49, 0x7a, 0x6c, 0x48, 0x7a, 0x4f, 0x79, 0x67, 0x42, 0x75, 
      // |/ytm23IzlHzOygBu|
     0x37, 0x6f, 0x6e, 0x39, 0x39, 0x54, 0x70, 0x64, 0x4e, 0x74, 0x36, 0x78, 0x4b, 0x71, 0x55, 0x4e, 
      // |7on99TpdNt6xKqUN|
     0x52, 0x0a, 0x48, 0x69, 0x49, 0x46, 0x4d, 0x33, 0x6e, 0x49, 0x45, 0x48, 0x4b, 0x74, 0x5a, 0x5a, 
      // |R.HiIFM3nIEHKtZZ|
     0x61, 0x74, 0x74, 0x74, 0x47, 0x52, 0x51, 0x74, 0x50, 0x52, 0x4e, 0x6e, 0x55, 0x4c, 0x76, 0x73, 
      // |atttGRQtPRNnULvs|
     0x4b, 0x48, 0x37, 0x4d, 0x37, 0x61, 0x75, 0x56, 0x53, 0x42, 0x68, 0x32, 0x33, 0x33, 0x71, 0x4a, 
      // |KH7M7auVSBh233qJ|
     0x2b, 0x77, 0x2b, 0x6c, 0x69, 0x32, 0x49, 0x4a, 0x4e, 0x68, 0x79, 0x72, 0x6e, 0x5a, 0x49, 0x43, 
      // |+w+li2IJNhyrnZIC|
     0x67, 0x44, 0x0a, 0x37, 0x2f, 0x66, 0x42, 0x55, 0x6c, 0x47, 0x58, 0x6a, 0x51, 0x43, 0x6d, 0x4e, 
      // |gD.7/fBUlGXjQCmN|
     0x48, 0x64, 0x33, 0x4c, 0x79, 0x68, 0x56, 0x2f, 0x4a, 0x37, 0x49, 0x50, 0x2b, 0x45, 0x6b, 0x41, 
      // |Hd3LyhV/J7IP+EkA|

    // 00000200
     0x4a, 0x6a, 0x45, 0x75, 0x37, 0x73, 0x45, 0x39, 0x50, 0x72, 0x70, 0x6e, 0x57, 0x76, 0x2b, 0x51, 
      // |JjEu7sE9PrpnWv+Q|
     0x6b, 0x7a, 0x6c, 0x44, 0x70, 0x38, 0x77, 0x65, 0x4e, 0x6c, 0x54, 0x4b, 0x38, 0x6e, 0x50, 0x75, 
      // |kzlDp8weNlTK8nPu|
     0x44, 0x47, 0x32, 0x0a, 0x46, 0x45, 0x55, 0x51, 0x63, 0x4d, 0x34, 0x64, 0x4a, 0x75, 0x4a, 0x31, 
      // |DG2.FEUQcM4dJuJ1|
     0x71, 0x51, 0x74, 0x74, 0x6a, 0x70, 0x65, 0x72, 0x45, 0x78, 0x35, 0x69, 0x62, 0x48, 0x45, 0x76, 
      // |qQttjperEx5ibHEv|
     0x4c, 0x62, 0x4e, 0x4f, 0x75, 0x31, 0x78, 0x74, 0x4b, 0x6b, 0x49, 0x46, 0x6b, 0x53, 0x4b, 0x30, 
      // |LbNOu1xtKkIFkSK0|
     0x72, 0x45, 0x51, 0x45, 0x68, 0x31, 0x70, 0x2f, 0x41, 0x67, 0x4d, 0x42, 0x41, 0x41, 0x47, 0x6a, 
      // |rEQEh1p/AgMBAAGj|
     0x4a, 0x44, 0x41, 0x69, 0x0a, 0x4d, 0x42, 0x4d, 0x47, 0x41, 0x31, 0x55, 0x64, 0x4a, 0x51, 0x51, 
      // |JDAi.MBMGA1UdJQQ|
     0x4d, 0x4d, 0x41, 0x6f, 0x47, 0x43, 0x43, 0x73, 0x47, 0x41, 0x51, 0x55, 0x46, 0x42, 0x77, 0x4d, 
      // |MMAoGCCsGAQUFBwM|
     0x42, 0x4d, 0x41, 0x73, 0x47, 0x41, 0x31, 0x55, 0x64, 0x44, 0x77, 0x51, 0x45, 0x41, 0x77, 0x49, 
      // |BMAsGA1UdDwQEAwI|
     0x45, 0x4d, 0x44, 0x41, 0x4e, 0x42, 0x67, 0x6b, 0x71, 0x68, 0x6b, 0x69, 0x47, 0x39, 0x77, 0x30, 
      // |EMDANBgkqhkiG9w0|
     0x42, 0x41, 0x51, 0x55, 0x46, 0x0a, 0x41, 0x41, 0x4f, 0x43, 0x41, 0x51, 0x45, 0x41, 0x51, 0x63, 
      // |BAQUF.AAOCAQEAQc|
     0x30, 0x34, 0x66, 0x6b, 0x2b, 0x2f, 0x70, 0x75, 0x75, 0x5a, 0x68, 0x39, 0x54, 0x67, 0x5a, 0x48, 
      // |04fk+/puuZh9TgZH|
     0x39, 0x2f, 0x2f, 0x4c, 0x71, 0x68, 0x41, 0x74, 0x2b, 0x6d, 0x55, 0x70, 0x69, 0x51, 0x32, 0x37, 
      // |9//LqhAt+mUpiQ27|
     0x43, 0x76, 0x6f, 0x69, 0x32, 0x71, 0x72, 0x42, 0x76, 0x56, 0x34, 0x7a, 0x69, 0x56, 0x64, 0x35, 
      // |Cvoi2qrBvV4ziVd5|
     0x58, 0x73, 0x38, 0x52, 0x49, 0x31, 0x0a, 0x37, 0x56, 0x76, 0x76, 0x79, 0x35, 0x50, 0x36, 0x63, 
      // |Xs8RI1.7Vvvy5P6c|
     0x35, 0x77, 0x54, 0x57, 0x41, 0x4c, 0x4a, 0x4e, 0x58, 0x47, 0x33, 0x49, 0x48, 0x62, 0x61, 0x4d, 
      // |5wTWALJNXG3IHbaM|

    // 00000300
     0x75, 0x58, 0x52, 0x35, 0x49, 0x6e, 0x47, 0x35, 0x4e, 0x77, 0x6e, 0x5a, 0x4d, 0x79, 0x52, 0x64, 
      // |uXR5InG5NwnZMyRd|
     0x41, 0x52, 0x50, 0x2b, 0x79, 0x34, 0x2f, 0x56, 0x49, 0x79, 0x30, 0x49, 0x31, 0x56, 0x56, 0x4a, 
      // |ARP+y4/VIy0I1VVJ|
     0x45, 0x53, 0x6e, 0x4a, 0x5a, 0x75, 0x77, 0x0a, 0x57, 0x50, 0x33, 0x66, 0x75, 0x46, 0x50, 0x49, 
      // |ESnJZuw.WP3fuFPI|
     0x41, 0x4d, 0x61, 0x7a, 0x42, 0x51, 0x73, 0x41, 0x64, 0x79, 0x4a, 0x52, 0x79, 0x64, 0x78, 0x54, 
      // |AMazBQsAdyJRydxT|
     0x76, 0x6a, 0x76, 0x5a, 0x4a, 0x67, 0x51, 0x5a, 0x5a, 0x74, 0x59, 0x56, 0x4e, 0x78, 0x76, 0x38, 
      // |vjvZJgQZZtYVNxv8|
     0x32, 0x54, 0x2f, 0x2b, 0x42, 0x4b, 0x68, 0x72, 0x57, 0x4f, 0x42, 0x65, 0x78, 0x62, 0x39, 0x41, 
      // |2T/+BKhrWOBexb9A|
     0x78, 0x6a, 0x72, 0x54, 0x58, 0x53, 0x48, 0x4a, 0x0a, 0x64, 0x61, 0x33, 0x38, 0x57, 0x2f, 0x44, 
      // |xjrTXSHJ.da38W/D|
     0x66, 0x54, 0x6b, 0x42, 0x70, 0x79, 0x64, 0x47, 0x67, 0x63, 0x36, 0x59, 0x72, 0x6d, 0x4e, 0x43, 
      // |fTkBpydGgc6YrmNC|
     0x70, 0x44, 0x30, 0x46, 0x44, 0x7a, 0x57, 0x50, 0x53, 0x33, 0x74, 0x32, 0x30, 0x4c, 0x75, 0x38, 
      // |pD0FDzWPS3t20Lu8|
     0x52, 0x2b, 0x61, 0x70, 0x67, 0x4f, 0x4a, 0x50, 0x7a, 0x77, 0x77, 0x51, 0x35, 0x51, 0x45, 0x51, 
      // |R+apgOJPzwwQ5QEQ|
     0x62, 0x51, 0x56, 0x69, 0x6a, 0x32, 0x6d, 0x62, 0x4e, 0x0a, 0x6d, 0x62, 0x62, 0x34, 0x57, 0x39, 
      // |bQVij2mbN.mbb4W9|
     0x46, 0x37, 0x64, 0x31, 0x6f, 0x31, 0x7a, 0x50, 0x50, 0x53, 0x76, 0x49, 0x65, 0x56, 0x55, 0x41, 
      // |F7d1o1zPPSvIeVUA|
     0x63, 0x6e, 0x2f, 0x35, 0x65, 0x58, 0x30, 0x52, 0x6e, 0x5a, 0x31, 0x45, 0x44, 0x6d, 0x4c, 0x73, 
      // |cn/5eX0RnZ1EDmLs|
     0x6b, 0x6d, 0x36, 0x55, 0x58, 0x67, 0x73, 0x4b, 0x4a, 0x59, 0x2b, 0x58, 0x53, 0x55, 0x79, 0x35, 
      // |km6UXgsKJY+XSUy5|
     0x53, 0x6a, 0x43, 0x75, 0x54, 0x75, 0x48, 0x59, 0x66, 0x57, 0x0a, 0x76, 0x35, 0x68, 0x61, 0x62, 
      // |SjCuTuHYfW.v5hab|
     0x47, 0x72, 0x4f, 0x54, 0x79, 0x45, 0x53, 0x72, 0x6e, 0x45, 0x61, 0x79, 0x53, 0x67, 0x76, 0x49, 
      // |GrOTyESrnEaySgvI|

    // 00000400
     0x33, 0x65, 0x66, 0x76, 0x50, 0x61, 0x70, 0x30, 0x67, 0x3d, 0x3d, 0x0a, 0x2d, 0x2d, 0x2d, 0x2d,
      // |3efvPap0g==.----|
     0x2d, 0x45, 0x4e, 0x44, 0x20, 0x43, 0x45, 0x52, 0x54, 0x49, 0x46, 0x49, 0x43, 0x41, 0x54, 0x45,
      // |-END CERTIFICATE|
     0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a                                   // |-----.|
    };

    FILE * in = fmemopen(data, sizeof(data), "r");

//    // generates the name of certificate file associated with RDP target
//    snprintf(filename, sizeof(filename) - 1, CERTIF_PATH "/X509-%s-%d.pem", this->ip_address, this->port);
//    filename[sizeof(filename) - 1] = '\0';
//    FILE *fp = ::fopen(filename, "r");

    X509 *px509Existing = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    if (!px509Existing) {
        // failed to read stored certificate file
        printf("Failed to read stored certificate: \"%s\"\n", filename);
        if (this->error_message) {
            *this->error_message = "Failed to read stored certificate: \"";
            *this->error_message += filename;
            *this->error_message += "\"\n";
        }
        throw Error(ERR_TRANSPORT, 0);
    }
    ::fclose(fp);


}
