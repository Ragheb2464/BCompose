#ifndef INTERFACE_FL_UTILS_DATA_H
#define INTERFACE_FL_UTILS_DATA_H

#include <vector>
using namespace std;

// origin = tail,,, destination =head
class Data_S {
private:
  int numFacilityNode, numCustomers, n_sc, n_scCap, n_scC;
  double *f, ***c, **d, **u;
  vector<string> *test_data;

public:
  Data_S(int n_sc0, int n_scCap0, int n_scC0) {
    test_data = new vector<string>();
    n_sc = n_sc0;
    n_scCap = n_scCap0;
    n_scC = n_scC0;
  }
  /////////////////////////////////Info Function: to read the data//////////////
  void readData(string instName) {
    const char *instancefilepath = instName.c_str(); // script
    test_data->clear();
    // reading data into the test_data vector
    fstream Data_file;
    Data_file.open(instancefilepath, ios::in);
    string value;
    while (Data_file >>
           value) // it starts from the begining of the file and starts reading
                  // data ... value corresponds to something without space
      test_data->push_back(value);
    Data_file.close();
    //
    int t = 0, tt = 0;
    //
    numFacilityNode = atoi((*test_data)[t++].c_str());
    numCustomers = atoi((*test_data)[t++].c_str());
    // f
    f = new double[numFacilityNode];
    for (int i = 0; i < numFacilityNode; i++)
      f[i] = atof((*test_data)[t++].c_str());
    // c
    c = new double **[n_sc];
    for (int s = 0; s < n_sc; s++) {
      c[s] = new double *[numFacilityNode];
      for (int i = 0; i < numFacilityNode; i++)
        c[s][i] = new double[numCustomers];
    }
    for (int s = 0; s < n_sc; s++)
      for (int i = 0; i < numFacilityNode; i++)
        for (int j = 0; j < numCustomers; j++)
          c[s][i][j] = atof((*test_data)[t++].c_str());
    //
    t += (1500 - n_sc) * numFacilityNode * numCustomers;
    //
    // t += (numFacilityNode+numCustomers)*numFacilityNode;
    //
    // t += (numFacilityNode+numCustomers)*numFacilityNode*numCustomers;
    // demands
    // t++;
    tt = t;
    d = new double *[n_sc];
    for (int s = 0; s < n_sc; s++)
      d[s] = new double[numCustomers];
    for (int s = 0; s < n_sc; s++) {
      for (int j = 0; j < numCustomers; j++)
        d[s][j] = atof((*test_data)[tt++].c_str());
      tt += numFacilityNode;
    }
    t += 5000 * (numFacilityNode + numCustomers);
    //
    t += numFacilityNode;
    // capacity
    u = new double *[n_sc];
    for (int s = 0; s < n_sc; s++)
      u[s] = new double[numFacilityNode];
    for (int s = 0; s < n_sc; s++)
      for (int i = 0; i < numFacilityNode; i++)
        u[s][i] = atof((*test_data)[t++].c_str());
  }
  ///////////////////////////////////////////////////////
  // int getN_nodes()				{return n_nodes;}
  inline int getNumFacilityNode() { return numFacilityNode; }
  inline int getNumCustomers() { return numCustomers; }
  inline int getN_sc() { return n_sc; }
  inline double getF(int i) { return f[i]; }
  inline double getP(int s) { return (double)1.0 / n_sc; }
  inline double getU(int i, int s) {
    if (n_scCap > 1)
      return u[s][i];
    else
      return u[0][i];
  }
  inline double getD(int s, int j) { return d[s][j]; }
  inline double getC(int i, int j, int s) {
    if (n_scC > 1)
      return c[s][i][j];
    else
      return c[0][i][j];
  }
  ///////////////////////////////////////////////////////
  ~Data_S() {}
};

#endif
