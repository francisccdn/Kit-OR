#include "readData.h"
#include "MinSpanningTree.h"
#include "CandidateList.h"
#include "SolutionUtils.h"
#include "Construction.h"
#include "LinKerninghan.h"

#include <fstream>
#include <iostream>

#include <vector>
#include <map>
#include <algorithm>
#include <time.h>
#include <cmath>
#include <limits>
#include <chrono>

using namespace std;

int dimension;
double **matrizAdj;

int main(int argc, char **argv)
{
  int x = time(NULL);
  // std::cout << x;
  // srand(1568068984);
  srand(x);

  readData(argc, argv, &dimension, &matrizAdj);
  printData(matrizAdj, dimension);

  vector<vector<int>> neighbourSet;
  generateCandidateList(neighbourSet, matrizAdj, dimension);

  double alpha = (double) rand() / RAND_MAX;
  std::vector<int> s = construction(alpha, dimension, matrizAdj);
  double objValue = calcularValorObj(s, matrizAdj);
  std::cout << "Initial obj value: " << objValue << "\n\n";
  for(int i = 0; i < s.size() - 1; i++)
  {
    std::cout << s[i] << " " << s[i+1] << ": "  << matrizAdj[s[i]][s[i+1]] << "\n";
  }

  // printSolucao(s);
  // std::cout << objValue << "\n";

  Tour T(s, dimension, objValue);

  // T.print();
  // T.flip(1, 6);
  // T.print();

  lkStep(T, matrizAdj, neighbourSet);

  std::list<double> a;
  return 0;
}
