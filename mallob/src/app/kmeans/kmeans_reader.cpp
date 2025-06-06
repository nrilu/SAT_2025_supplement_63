#include "kmeans_reader.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#include "util/sys/terminator.hpp"
bool KMeansReader::read(const std::string &filename, JobDescription &desc) {
    /*
    files have to be in format:
    k = k of kmeans
    dim = dimension of points
    col = number of columns (often there are more than wanted. (clo - dim) last colums are cut off)
    count = count of points

    k dim col count
    0.0 0.1 0.2
    1.0 1.1 1.2
    one point per row
    */

    // allocate necessary structs for the revision to read

    desc.beginInitialization(0);

    std::ifstream ifile(filename.c_str(), std::ios::in);

    // check to see that the file was opened correctly:
    if (!ifile.is_open()) {
        std::cerr << "There was a problem opening the input file!\n";
        return false;
    }

    int countClusters = 0;
    int dimension = 0;
    int columnsInFile = 0;
    int pointsCount = 0;

    ifile >> countClusters;
    ifile >> dimension;
    ifile >> columnsInFile;
    ifile >> pointsCount;

    int skipCols = columnsInFile - dimension;  // dont read the last few columns

    desc.addPermanentData(countClusters);
    desc.addPermanentData(dimension);
    desc.addPermanentData(pointsCount);
    //LOG(V5_DEBG, "                          countClusters: %i dimension %i pointsCount: %i columnsInFile: %i skipCols:%i\n", countClusters, dimension, pointsCount, columnsInFile, skipCols);
    float num = 0.0;
    for (int point = 0; point < pointsCount; ++point) {
        for (int entry = 0; entry < dimension; ++entry) {
            ifile >> num;
            desc.addPermanentData(num);
            //if (point < 10) {
            //    LOG(V5_DEBG, "                          num: %f\n", num);
            //}
        }
        if (Terminator::isTerminating()) return false;
        for (int skip = 0; skip < skipCols; ++skip) {
            ifile >> num;  // dont read the last few columns of each row
            // num wont be read until reset
            //if (point < 10) {
            //    LOG(V5_DEBG, "                          skip num: %f\n", num);
            //}
        }
    }

    ifile.close();
    desc.endInitialization();
    // success
    return true;
}
