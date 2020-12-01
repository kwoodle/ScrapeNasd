//
// Created by kwoodle on 1/10/18.
//
// Use Curl to download web pages
// Use xpath in libxml to collect Time and Sales from NASDAQ,
// e.g. http://www.nasdaq.com/symbol/SYMBOL/time-sales?time=1
//
// NOTE: When connected remotely to nasdaq.com, one seems to get dropped by their
// Akima server after retrieving some amount of data. I suppose this is done as a bandwidth
// limiting measure. This seems to be cured by using a VPN connection.
//

//#include <fstream>
#include "Scrape.h"
#include <regex>

using std::cout;
using std::endl;

//
// encod is the character encoding (typically UTF-8) associated with the web page
// It's used by libxml2. Here it's assumed to apply to every page.
//
// rows is a vector of stock names, and EzCurl is a curl handle used to
// retrieve HTML pages into files on disk
//
void scrape(const Stocks& stocks, EzCurl& EzCurl)
{
    //
    // build base url for nasd Time and Sales
    // http://www.nasdaq.com/symbol/ms/time-sales?time=1&pageno=3
    // https: !!
    //
//    static const string root{"https://www.nasdaq.com/symbol/"};
    static const string root{"https://old.nasdaq.com/symbol/"};
    static const string tsl0{"/time-sales"};
    static const string tsls{"/time-sales?time="};
    static const string pgnos{"&pageno="};

    // step through stocks in the index
    //
    static const int ntest{2};
    static const int ptest{3};
    int itest{0};
    int iptest{0};
    for (auto &stock: stocks) {
        Trades trds;
        // static const string eloc{"http://www.nasdaq.com/symbol/SYMBOL/time-sales?time=1"};
        string symbol{stock.first};
        string eloc = root + symbol += tsls;

        // drop-down list of times of day
        // eleven by end of day
        for (int it = 1; it <= 11; ++it) {
            string loc = eloc + std::to_string(it);
//            if (itest++ == ntest) return;
            //
            ////////////////////////////////////////////////////////////////////////////////////
            //
            /// For each time interval for each stock get the last page, which is the earliest
            /// time. Work backwards from there. Each page is retrieved by Curl, which attempts
            /// to reuse connections to the host.
            //
            ///////////////////////////////////////////////////////////////////////////////////

            URL nasdloc = eloc+std::to_string(it);
            int mxpno{0};
            string doc_file = EzCurl.GetResponseFile(nasdloc);
            //
            // parse the file
            HtmlDoc NasdDoc{doc_file};

            // xpath for last page
            // <a href="http://www.nasdaq.com/symbol/ms/time-sales?time=1&pageno=32"
            // id="quotes_content_left_lb_LastPage" class="pagerlink">last &gt;&gt;</a>
            //
            static const string anch{"quotes_content_left_lb_LastPage"}; // anchor
            Xpath ancid = "//a[@id='"+anch+"']";
            NasdDoc.GetNodeset(ancid);
            string lstpage;
            if (NasdDoc.result()) {
                lstpage = NasdDoc.GetProp(string{"href"});
            }
            static const std::regex pat{R"(pageno=(\d+))"};
            std::smatch matches;
            bool foun = regex_search(lstpage, matches, pat);
            if (!foun) {
                throw std::runtime_error{"Couldn't find last page number"};
            }
            //
            // convert max page string to int
            mxpno = std::stoi(matches[1]);
            std::cout << "Last page for " << symbol << " is " << mxpno << endl;
            //
            ///////////////////////////////////////////////////////////////////////////////////////////////////////
            //
            // Step through pages, retrieving HTML doc for each
            //
            ///////////////////////////////////////////////////////////////////////////////////////////////////////
            for (int ip = mxpno; ip>=1; --ip) {
                if (iptest++==ptest) return;
                // URL for curl
                URL ploc = loc+pgnos+std::to_string(ip);
                doc_file = EzCurl.GetResponseFile(ploc);
                HtmlDoc Nasd2{doc_file};
                //
                // Get the page date which will be used to create date-time for each trade
                //
                static const string span{"qwidget_markettime"};
                Xpath spid = "//span[@id='"+span+"']";
                Nasd2.GetNodeset(spid);
                string pdate;
                if (Nasd2.result()) {
                    pdate = Nasd2.GetNodeString(Nasd2.nodeset()->nodeTab[0]->xmlChildrenNode, 1);
                }
                else {
                    std::cerr << "Failed to get page date" << endl;
                    return;
                }
//                cout<<"Page Date = "<<pdate<<" for symbol "<<symbol<<" time interval "<<it<<" and page "<<ip<<endl;


                ////////////////////////////////////////////////////////////////////////////////////////////////////
                //
                // get Time and Sales for this symbol and this interval and this page of 50
                //
                ///////////////////////////////////////////////////////////////////////////////////////////////////
                static const string table_class = "AfterHoursPagingContents_Table";
                string qs = "//table[@id='"+table_class+"']/tr/td";
                Nasd2.GetNodeset(qs);
                if (Nasd2.result()) {
                    int i{0};
                    for (; i<Nasd2.nodeset()->nodeNr; i += 3) {
                        string ti = Nasd2.GetNodeString(Nasd2.nodeset()->nodeTab[i]->xmlChildrenNode, 1);
                        ti.erase(std::remove_if(ti.begin(), ti.end(),
                                [](unsigned char x) { return std::isspace(x); }), ti.end());
                        //
                        // construct timestamp
                        //
                        struct tm tm;
                        memset(&tm, 0, sizeof(struct tm));
                        string dattim = pdate+" " += ti;
                        // e.g. Nov. 7, 2017
                        //
                        strptime(dattim.c_str(), "%b. %d, %Y %H:%M:%S", &tm);
                        //
                        // use boost posix time as key to multimap to provide sorting
                        //
                        boopt::ptime pt(boopt::ptime_from_tm(tm));
//                        string dtim(boop::to_iso_extended_string(pt));
                        //
                        // Replace T in 2017-11-07T09:38:00
//                        std::replace(dtim.begin(), dtim.end(), 'T', ' ');

                        // MySQL TIMESTAMP 2038-01-19 03:14:07 %Y-%m-%d

                        //
                        // get price
                        //
                        string pri = Nasd2.GetNodeString(Nasd2.nodeset()->nodeTab[i+1]->xmlChildrenNode, 1);
                        size_t pos = pri.find('$');
                        if (pos!=string::npos) pri.erase(pos, 1);
                        pri.erase(std::remove_if(pri.begin(), pri.end(),
                                [](unsigned char x) { return std::isspace(x); }), pri.end());
                        pri.erase(std::remove_if(pri.begin(), pri.end(),
                                [](unsigned char x) { return !std::isprint(x); }), pri.end());
                        //
                        // get shares
                        //
                        string sha = Nasd2.GetNodeString(Nasd2.nodeset()->nodeTab[i+2]->xmlChildrenNode, 1);
                        pos = sha.find(',');
                        if (pos!=string::npos) sha.erase(pos, 1);

                        //
                        // put time and sales into trades map
                        //
                        trds.emplace(pt, std::make_pair(pri, sha));
//                        cout<<symbol<<" "<<pt<<" "<<pri<<" "<<sha<<"\n";
                    }
                    auto t = *trds.rbegin();
                    std::cout << (i/3) << " trades for " << stock.first << " on page no " << ip << " and it = " << it
                              << endl;
                    std::cout << "Last Trade " << t.first << " " << t.second.first << " " << t.second.second << "\n";
//                    auto t = *trds.begin();
//                    cout<<symbol<<" "<<t.first<<" "<<t.second.first<<" "<<t.second.second<<"\n";
                }
                std::cout << trds.size() << " trades for " << stock.first << "\n" << endl;
            }
        }
    }
}

/* Symbol | TradeTime           | TradePrice | TradeSize
 *   MS   | YYYY-MM-DD HH:MM:SS | 55.43      | 100
 *   */

void scrape(const Stocks& stocks, Multi& mc)
{
    //
    // build base url for nasd Time and Sales
    // http://www.nasdaq.com/symbol/ms/time-sales?time=1&pageno=3
    //
    static const string root{"http://old.nasdaq.com/symbol/"};
    static const string tsls{"/time-sales?time="};
    static const string pgnos{"&pageno="};

    // step through stocks in the index
    //
    static const int ntest{2};
    static const int ptest{3};
    int itest{0};
    int iptest{0};
    for (auto& stock: stocks) {
        Trades trds;
        // static const string eloc{"http://www.nasdaq.com/symbol/SYMBOL/time-sales?time=1"};
        string symbol{stock.first};
        string eloc = root+symbol += tsls;

        // drop-down list of times of day
        // eleven by end of day
        for (int it = 1; it<=11; ++it) {
            string loc = eloc+std::to_string(it);
//            if (itest++ == ntest) return;
            //
            ////////////////////////////////////////////////////////////////////////////////////
            //
            /// For each time interval for each stock get the last page, which is the earliest
            /// time. Work backwards from there. Each page is retrieved by Curl, which attempts
            /// to reuse connections to the host.
            //
            ///////////////////////////////////////////////////////////////////////////////////

            URL nasdloc = eloc+std::to_string(it);
            int mxpno{0};
            URLVec uvec{nasdloc};
            mc.load(uvec);
            auto r = mc.do_perform(uvec);
            string doc_file = r.begin()->second;
            //
            // parse the file
            HtmlDoc NasdDoc{doc_file};
            // xpath for last page
            // <a href="http://www.nasdaq.com/symbol/ms/time-sales?time=1&pageno=32"
            // id="quotes_content_left_lb_LastPage" class="pagerlink">last &gt;&gt;</a>
            //
            static const string anch{"quotes_content_left_lb_LastPage"}; // anchor
            Xpath ancid = "//a[@id='"+anch+"']";
            NasdDoc.GetNodeset(ancid);
            string lstpage;
            if (NasdDoc.result()) {
                lstpage = NasdDoc.GetProp(string{"href"});
            }
            static const std::regex pat{R"(pageno=(\d+))"};
            std::smatch matches;
            bool foun = regex_search(lstpage, matches, pat);
            if (!foun) {
                mxpno = 1;
                cout << "Couldn't find last page number!!\n";
//                cout << NasdDoc.GetFirstPar();
//                throw std::runtime_error{"Couldn't find last page number"};
            } else {
                //
                // convert max page string to int
                mxpno = std::stoi(matches[1]);
            }
            std::cout << "Last page for " << symbol << " is " << mxpno << endl;

            //
            ///////////////////////////////////////////////////////////////////////////////////////////////////////
            //
            // Step through pages, retrieving HTML doc for each
            //
            ///////////////////////////////////////////////////////////////////////////////////////////////////////
            URLVec tim_urls;
            for (int ip = mxpno; ip>=1; --ip) {
//                if (iptest++==ptest) break;
                // URL for curl
                tim_urls.emplace_back(loc+pgnos+std::to_string(ip));
            }
            while (!tim_urls.empty()) {
                mc.load(tim_urls);
                ResVec tim_rslts{mc.do_perform(tim_urls)};
                for (auto &e : tim_rslts) {
                    doc_file = e.second;
                    HtmlDoc Nasd2{doc_file};
                    //
                    // Get the page date which will be used to create date-time for each trade
                    //
                    static const string span{"qwidget_markettime"};
                    Xpath spid = "//span[@id='" + span + "']";
                    Nasd2.GetNodeset(spid);
                    string pdate;
                    if (Nasd2.result()) {
                        pdate = Nasd2.GetNodeString(Nasd2.nodeset()->nodeTab[0]->xmlChildrenNode, 1);
                    }
                    else {
                        std::cerr << "Failed to get page date" << endl;
                        return;
                    }
                    cout << "Page Date = " << pdate << " for symbol " << symbol << " time interval " << it << endl;

                    ////////////////////////////////////////////////////////////////////////////////////////////////////
                    //
                    // get Time and Sales for this symbol and this interval and this page of 50
                    //
                    ///////////////////////////////////////////////////////////////////////////////////////////////////
                    static const string table_class = "AfterHoursPagingContents_Table";
                    string qs = "//table[@id='"+table_class+"']/tr/td";
                    Nasd2.GetNodeset(qs);
                    if (Nasd2.result()) {
                        int i{0};
                        for (; i<Nasd2.nodeset()->nodeNr; i += 3) {
                            string ti = Nasd2.GetNodeString(Nasd2.nodeset()->nodeTab[i]->xmlChildrenNode, 1);
                            ti.erase(std::remove_if(ti.begin(), ti.end(),
                                    [](unsigned char x) { return std::isspace(x); }), ti.end());
                            //
                            // construct timestamp
                            //
                            struct tm tm;
                            memset(&tm, 0, sizeof(struct tm));
                            string dattim = pdate+" " += ti;
                            // e.g. Nov. 7, 2017
                            //
                            strptime(dattim.c_str(), "%b. %d, %Y %H:%M:%S", &tm);
                            //
                            // use boost posix time as key to multimap to provide sorting
                            //
                            boopt::ptime pt(boopt::ptime_from_tm(tm));
//                        string dtim(boop::to_iso_extended_string(pt));
                            //
                            // Replace T in 2017-11-07T09:38:00
//                        std::replace(dtim.begin(), dtim.end(), 'T', ' ');

                            // MySQL TIMESTAMP 2038-01-19 03:14:07 %Y-%m-%d

                            //
                            // get price
                            //
                            string pri = Nasd2.GetNodeString(Nasd2.nodeset()->nodeTab[i+1]->xmlChildrenNode, 1);
                            size_t pos = pri.find('$');
                            if (pos!=string::npos) pri.erase(pos, 1);
                            pri.erase(std::remove_if(pri.begin(), pri.end(),
                                    [](unsigned char x) { return std::isspace(x); }), pri.end());
                            pri.erase(std::remove_if(pri.begin(), pri.end(),
                                    [](unsigned char x) { return !std::isprint(x); }), pri.end());
                            //
                            // get shares
                            //
                            string sha = Nasd2.GetNodeString(Nasd2.nodeset()->nodeTab[i+2]->xmlChildrenNode, 1);
                            pos = sha.find(',');
                            if (pos!=string::npos) sha.erase(pos, 1);

                            //
                            // put time and sales into trades map
                            //
                            trds.emplace(pt, std::make_pair(pri, sha));
                            cout << symbol << " " << pt << " " << pri << " " << sha << "\n";
                        }
//                        auto t = *trds.rbegin();
//                        std::cout << (i/3) << " trades for " << stock.first << " and it = " << it
//                                  << endl;
//                        std::cout << "Last Trade " << t.first << " " << t.second.first << " " << t.second.second << "\n";
//                    auto t = *trds.begin();
//                    cout<<symbol<<" "<<t.first<<" "<<t.second.first<<" "<<t.second.second<<"\n";
                    }
                    else {
                        auto trades{trds.size()};
//                        std::cout << trades << " trades for " << stock.first <<
//                                                                             " time "<< it<<"\n" << endl;
                        if (trades == 0) break;
                    }
                }
                auto trades{trds.size()};
//                std::cout << trades << " trades for " << stock.first <<  " page "<< "\n" << endl;
                if (trades == 0) break;
            }
            auto trades{trds.size()};
            std::cout << trades << " trades for " << stock.first << "\n" << endl;
            if (trades == 0) break;
        }
    }
}

