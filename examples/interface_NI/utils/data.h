#ifndef CLASS_DATA_S1_H
#define CLASS_DATA_S1_H

#include <vector>
using namespace std;

// origin = tail,,, destination =head
class Data {
private:
  int *tailAD, *headAD, sizeAD, sizeD, *headD, *tailD, numOD, *origins,
      *destinations, n_sc, n_nodes, n_nodes2;
  double *rD, *q, *r, *p, **c, b, **phi, snipno;
  vector<string> *test_data;

public:
  Data(int bVal, int snipnoVal) {
    test_data = new vector<string>();
    b = bVal;
    n_nodes = 783;
    n_nodes2 = 862;
    n_sc = 456;
    snipno = snipnoVal;
  }
  /////////////////////////////////Info Function: to read the data//////////////
  void readArcGain(string instNameArcGain) {
    const char *instancefilepath = instNameArcGain.c_str(); // script
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
    // AD and r
    sizeAD = test_data->size() / 3;
    tailAD = new int[sizeAD]();
    headAD = new int[sizeAD]();
    r = new double[sizeAD]();
    //
    int t = 0, tt = 0;
    while (t < test_data->size()) {
      tailAD[tt] = atoi((*test_data)[t++].c_str());
      headAD[tt] = atoi((*test_data)[t++].c_str());
      r[tt] = atof((*test_data)[t++].c_str());
      tt++;
    }
  }
  /////////////////////////////////Info Function: to read the data
  ///////////////////////////////////////////////////////////////////////////////////
  void readIntdArc(string instNameIntdArc) {
    const char *instancefilepath = instNameIntdArc.c_str(); // script
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
    // D and rD and q
    sizeD = test_data->size() / 4;
    tailD = new int[sizeD]();
    headD = new int[sizeD]();
    rD = new double[sizeD]();
    q = new double[sizeD]();
    //
    int t = 0, tt = 0;
    while (t < test_data->size()) {
      tailD[tt] = atoi((*test_data)[t++].c_str());
      headD[tt] = atoi((*test_data)[t++].c_str());
      rD[tt] = atof((*test_data)[t++].c_str());
      q[tt] = atof((*test_data)[t++].c_str());
      tt++;
    }
  }
  /////////////////////////////////Info Function: to read the data
  ///////////////////////////////////////////////////////////////////////////////////
  void readScenario(string instNameScenario) {
    const char *instancefilepath = instNameScenario.c_str(); // script
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
    // OD
    numOD = test_data->size() / 3;
    origins = new int[numOD]();
    destinations = new int[numOD]();
    p = new double[numOD]();
    //
    int t = 0, tt = 0;
    while (t < test_data->size()) {
      origins[tt] = atoi((*test_data)[t++].c_str());
      destinations[tt] = atoi((*test_data)[t++].c_str());
      p[tt] = atof((*test_data)[t++].c_str());
      tt++;
    }
  }
  /////////////////////////////////Info Function: to read the data
  ///////////////////////////////////////////////////////////////////////////////////
  void readPsi(string instNamePsi) {
    const char *instancefilepath = instNamePsi.c_str(); // script
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
    // OD
    phi = new double *[n_sc];
    for (int s = 0; s < n_sc; s++)
      phi[s] = new double[n_nodes2]();
    //
    int t = 0, s = 0;
    while (t < test_data->size()) {
      for (int i = 0; i < n_nodes2; i++) {
        if (i <= 37)
          phi[s][i] = atof((*test_data)[t++].c_str());
        if (i >= 90 && i <= 182)
          phi[s][i] = atof((*test_data)[t++].c_str());
        if (i >= 190 && i <= 829)
          phi[s][i] = atof((*test_data)[t++].c_str());
        if (i >= 850)
          phi[s][i] = atof((*test_data)[t++].c_str());
      }
      s++;
    }
  }
  ///////////////////////////////////////////////////////
  // int getN_nodes()				{return n_nodes;}
  int getN_nodes2() { return n_nodes2; }

  int getN_sc() { return n_sc; }

  int getOrigin(int s) { return origins[s] - 1; }
  int getDestinations(int s) { return destinations[s] - 1; }
  int getNumOD() { return numOD; }
  double getP(int s) { return p[s]; }

  int getSizeD() { return sizeD; }
  int getTailD(int a) { return tailD[a] - 1; }
  int getHeadD(int a) { return headD[a] - 1; }
  double getRD(int a) { return rD[a]; }
  double getQ(int a) {
    if (snipno == 3)
      return 0.1 * getRD(a);
    else if (snipno == 4)
      return 0;
    else {
      cout << "\nNo right Inst\n";
      abort();
    }
  }

  int getSizeAD() { return sizeAD; }
  int getTailAD(int a) { return tailAD[a] - 1; }
  int getHeadAD(int a) { return headAD[a] - 1; }
  double getR(int a) { return r[a]; }

  double getB() { return b; }
  double getC(int a) { return 1; }

  double getPhi(int s, int j) { return phi[s][j]; }
  ///////////////////////////////////////////////////////
  ~Data() {}
};

#endif
