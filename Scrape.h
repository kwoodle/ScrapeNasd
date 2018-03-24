//
// Created by kwoodle on 1/10/18.
// Uses libKtrade and namespace drk.
//

#ifndef SCRAPENASD_SCRAPE_H
#define SCRAPENASD_SCRAPE_H

#include <ktrade/ktcurl.h>
#include <ktrade/HtmlDoc.h>
#include <vector>
#include <string>
#include <map>
#include <boost/date_time/posix_time/posix_time.hpp> //include all types plus i/o
#include <boost/filesystem.hpp>

using std::vector;
using std::string;
using std::pair;
namespace boopt = boost::posix_time;
namespace boofs = boost::filesystem;        // to get name of executable

using namespace drk;

// Each Stock is a pair <symbol, description>
using Stocks = vector<pair<string, string>>;

// Each Trade is a pair <price, shares> sorted by tradetime
using Trades = std::multimap<boopt::ptime, pair<string, string>>;

// Page has url=loc and encoding enc
struct Page {
    string loc;
    string enc;
};

// Get vector of stocks
// e.g. from S&P 500
Stocks getstocks(const string& loc, const string& enc="UTF-8");


void scrape(const Stocks& stocks, EzCurl& ez);
void scrape(const Stocks& rows, Multi& mc);

#endif //SCRAPENASD_SCRAPE_H
