//
// Created by kwoodle on 1/10/18.
//

#include "Scrape.h"

Stocks getstocks(const string& loc, const string& enc)
{
    string symb{"AAPL"};
    string descrip{"Apple Inc."};
    Stocks out;
    out.emplace_back(make_pair(symb, descrip));
    return out;
}