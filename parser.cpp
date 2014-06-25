#include "json/json.h"
#include <iostream>
#include <fstream>
// #include <pqxx/pqxx>
#include <string>
#include <vector>
#include <boost/regex.hpp>

using namespace std;

struct PathPart {
    string string_; 
    int int_;
    bool isString_;
};

typedef vector<PathPart> PathVec;

struct ColSpec {
    string name_;
    PathVec pathVec_;
    bool hasRegex_;
    boost::regex regex_;
    string replaceString_;
};

typedef vector<ColSpec> ColVec;

struct DataSpec {
    string tableName_;
    ColVec colVec_;
};


int parseSpec (const string &fileName, DataSpec &dataSpec) {
    ifstream specFile (fileName.c_str());
    string line;
    if (getline(specFile,line)) {
        specFile.close();
        Json::Value root;
        Json::Reader reader;
        bool parseSuccessful = reader.parse(line, root);
        if (!parseSuccessful)
            return 0;
        dataSpec.tableName_ = root["table"].asString();
        Json::Value cols = root["cols"];
        for (int c = 0; c != cols.size(); c++) {
            ColSpec colSpec;
            colSpec.name_ = cols[c]["var"].asString();
            if (cols[c].isMember("regex")) {
                colSpec.hasRegex_ = true;
                //colSpec.regex_ = new boost::regex(cols["regex"].asString());
                colSpec.regex_ = cols[c]["regex"]["search"].asString();
                colSpec.replaceString_ = cols[c]["regex"]["replace"].asString();
            }
            else
                colSpec.hasRegex_ = false;


            Json::Value path = cols[c]["path"];
            for (int p = 0; p != path.size(); ++p) {
                PathPart pathPart;
                if (path[p].isInt()) {
                    pathPart.isString_ = false;
                    pathPart.int_ = path[p].asInt();
                }
                else {
                    pathPart.isString_ = true;
                    pathPart.string_ = path[p].asString();
                }
                colSpec.pathVec_.push_back(pathPart); 
            }
            dataSpec.colVec_.push_back(colSpec);
        }

    }
    return 1;
}

int parseFile (const string &fileName, const char delim, const DataSpec &dataSpec) {
    string inFileName = fileName + ".json";
    string outFileName = fileName + ".csv";
    ifstream inFile (inFileName.c_str());
    ofstream outFile (outFileName.c_str());
    int numCols = dataSpec.colVec_.size();
    string line;
    size_t lineNo = 0;

    while (getline(inFile, line)) { 
        Json::Value root;
        Json::Reader reader;
        bool parseSuccessful = reader.parse(line, root);
        if (!parseSuccessful) {
            //cout << "Failed parse: " << lineNo << endl;
            continue;
        }
        for (int c = 0; c != numCols; ++c) {
            int pathLength = dataSpec.colVec_[c].pathVec_.size();
            Json::Value val = root; 
            for (int p = 0; p != pathLength; ++p) { 
                if (dataSpec.colVec_[c].pathVec_[p].isString_) {
                    val = val[dataSpec.colVec_[c].pathVec_[p].string_];
                }
                else {
                val = val[dataSpec.colVec_[c].pathVec_[p].int_];
                }
            }

            if (dataSpec.colVec_[c].hasRegex_) {
                outFile << boost::regex_replace(val.asString(),dataSpec.colVec_[c].regex_,dataSpec.colVec_[c].replaceString_);
            }
            else
                outFile << val.asString();
            
            if (c < numCols - 1)
                outFile << delim;
            else
                outFile << "\n";
        }
        ++lineNo;
    }
    inFile.close();
    outFile.close();
}



int main(int argc, char *argv[]) {
    string fileName = argv[1];
    string tableName;
    DataSpec dataSpec;
    int parsed = parseSpec("dataspec.json",dataSpec);
    cout << "Parsed: " << parsed << endl;
    cout << "Table name: " << dataSpec.tableName_ << endl;
    if (parsed) {
        parseFile(fileName,'\t', dataSpec);
    }
}

