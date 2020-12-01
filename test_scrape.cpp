//
// Created by kwoodle on 11/25/19.
//

// Test Scrape Time and Sales for list of symbols from Nasd using, e.g.
// http://www.nasdaq.com/symbol/ms/time-sales?time=1&pageno=3
//

#include "Scrape_test.h"
#include <iostream>

int main(int argc, char **argv) {

    // Get name of this application
    // to be used to create somewhat unique filename
    // in which to store data
    //
    boofs::path app_path{argv[0]};
    string app_name = app_path.filename().string();

    // Get list of symbols
    //
    string enc{"UTF-8"};
    string SP100{"https://en.wikipedia.org/wiki/S%26P_100"};
    Stocks SandP = getstocks(SP100);

    // Scrape the pages
    //
    try {
//        Multi mc(app_name, true);
//        scrape(SandP, mc);
//        EzCurl ezCurl(10L);
//        EzCurl ez(app_name, true);
        scrape(SandP);

    }
    catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}