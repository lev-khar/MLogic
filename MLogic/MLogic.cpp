// MLogic.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#pragma comment(lib, "bdd.lib")
#include "bdd.h"
#include "json.hpp"
#include "math.h"
#include <iostream>
#include <fstream>
#include <vector>

using json = nlohmann::json;

int N_VAR = 144;
int N = 9;
int M = 4;
int LOG_N = 4;
int ROW_LENGTH = (int)sqrt(N);
int LAST_ELMNT_FIRST_ROW = N / ROW_LENGTH - 1;

bdd F;
std::vector<std::vector<std::vector<bdd>>> p;
char* var;

void fun(char* varset, int size); 
bdd right(int traitSelf, int valueSelf, int traitNeighbour, int valueNeighbour);
bdd left(int traitSelf, int valueSelf, int traitNeighbour, int valueNeighbour);
json readInitialValues();
json getContent();
void readBounds(json j);

int  main(void)
{
  json j = readInitialValues();
  LOG_N = (int)std::log2(N);
  if ((LOG_N & (LOG_N - 1)) != 0) {
    LOG_N++;
  }
  N_VAR = M * N * LOG_N;
  ROW_LENGTH = (int)sqrt(N);
  LAST_ELMNT_FIRST_ROW = N / ROW_LENGTH - 1;

  std::cout << "N = " << N << "\nM = " << M << "\nLOG_N = " << LOG_N << "\nN_VAR = " << N_VAR 
    << "\nLAST_ELMNT_FIRST_ROW = " << LAST_ELMNT_FIRST_ROW << "\nROW_LENGTH = " << ROW_LENGTH << std::endl;
  var = new char[N_VAR];

  bdd_init(10000000, 1000000);
  bdd_setvarnum(N_VAR);

  for (unsigned i = 0; i < M; i++) {
    std::vector<std::vector<bdd>> t1;
    for (unsigned j = 0; j < N; j++) {
      std::vector<bdd> t2;
      for (unsigned k = 0; k < N; k++) {
        bdd bdd;
        t2.push_back(bdd);
      }
      t1.push_back(t2);
    }
    p.push_back(t1);
  }

  for (unsigned m = 0; m < M; m++) {
    unsigned I = 0;

    for (unsigned i = 0; i < N; i++) {
      for (unsigned j = 0; j < N; j++) {
        p[m][i][j] = bddtrue;
        for (unsigned k = 0; k < LOG_N; k++) {
          p[m][i][j] &= ((j >> k) & 1) ? bdd_ithvar(I + LOG_N * m + k) : bdd_nithvar(I + LOG_N * m + k);
        }
      }
      I += LOG_N * M;
    }
  }

  F = bddtrue; 
  readBounds(j);

  // Bound 5
  for (unsigned m = 0; m < M; m++) {
    for (unsigned i = 0; i < N - 1; i++) {
      for (unsigned j = i + 1; j < N; j++) {
        for (unsigned k = 0; k < N; k++) {
          F &= bdd_apply(p[m][i][k], !p[m][j][k], bddop_imp);
        }
      }
    }
  }

  // Bound 6
  for (unsigned m = 0; m < M; m++) {
    for (unsigned i = 0; i < N; i++) {
      bdd temp = bddfalse;

      for (unsigned j = 0; j < N; j++) {
        temp |= p[m][i][j];
      }

      F &= temp;
    }
  }

  unsigned satcount = (unsigned)bdd_satcount(F);
  std::cout << satcount << " solutions\n";
  if (satcount) {
    bdd_allsat(F, fun);
  }
  bdd_done();

  return 0;
}

bdd left(int traitSelf, int valueSelf, int traitNeighbour, int valueNeighbour) {
  bdd temp = bddtrue;
  for (unsigned i = 1; i < N; i++) {
    if (i % N == 0 && N - i > 0) { //склейка
      temp &= bdd_apply(p[traitSelf][i][valueSelf], p[traitNeighbour][i + LAST_ELMNT_FIRST_ROW][valueNeighbour], bddop_biimp);
    }
    else if (i % ROW_LENGTH != 0) {
      temp &= bdd_apply(p[traitSelf][i][valueSelf], p[traitNeighbour][i - 1][valueNeighbour], bddop_biimp);
    }
    else {
      temp &= !p[traitSelf][i][valueSelf];
    }
  }

  return temp;
}

bdd right(int traitSelf, int valueSelf, int traitNeighbour, int valueNeighbour) {
  bdd temp = bddtrue;

  for (unsigned i = 0; i < N - 1; i++) {
    if (i % N == LAST_ELMNT_FIRST_ROW) { //склейка
      temp &= bdd_apply(p[traitSelf][i][valueSelf], p[traitNeighbour][i - LAST_ELMNT_FIRST_ROW][valueNeighbour], bddop_biimp);
    }
    else if ((i + 1) % ROW_LENGTH != 0) {
      temp &= bdd_apply(p[traitSelf][i][valueSelf], p[traitNeighbour][i + 1][valueNeighbour], bddop_biimp);
    }
    else {
      temp &= !p[traitSelf][i][valueSelf];
    }
  }

  return temp;
}

json readInitialValues() {
  json j = getContent();
  N = j.at("N");
  M = j.at("M");
  return j;
}

json getContent() {
  char filename[] = "C:\\Users\\Лев\\source\\repos\\MLogic\\MLogic\\params.json";
  FILE* file;
  errno_t err;
  err = fopen_s(&file, filename, "r");
  if (err != 0)
    exit(EXIT_FAILURE);
  json j = json::parse(file);
  return j;
}

void readBound1(json j) {
  std::vector<std::vector<int>> bound = j.at("bound1");
  for (int c = 0; c < bound.size(); c++) {
    F &= p[bound.at(c).at(0)][bound.at(c).at(1)][bound.at(c).at(2)];
  }
}

void readBound2(json j) {
  std::vector<std::vector<int>> bound = j.at("bound2");
  for (int c = 0; c < bound.size(); c++) {
    for (unsigned i = 0; i < N; i++) {
      F &= bdd_apply(p[bound.at(c).at(0)][i][bound.at(c).at(1)], p[bound.at(c).at(2)][i][bound.at(c).at(3)], bddop_biimp);
    }
  }
}

void readBound3(json j) {
  std::vector<std::vector<int>> bound = j.at("bound3");
  for (int c = 0; c < bound.size(); c++) {
  auto l = bound.at(c);
    switch (l.at(0)) {
    case 0:
      F &= right(l.at(1), l.at(2), l.at(3), l.at(4));
      break;
    case 1:
      F &= left(l.at(1), l.at(2), l.at(3), l.at(4));
      break;
    }
  }
}

void readBound4(json j) {
  std::vector<std::vector<int>> bound = j.at("bound4");
  for (int c = 0; c < bound.size(); c++) {
    auto l = bound.at(c);
    F &= (left(l.at(0), l.at(1), l.at(2), l.at(3)) | right(l.at(0), l.at(1), l.at(2), l.at(3)));
  }
}

void readBounds(json j) {
  readBound1(j);
  readBound2(j);
  readBound3(j);
  readBound4(j);
}

void print(void) {
  for (unsigned i = 0; i < N; i++) {
    std::cout << i << ": ";
    for (unsigned j = 0; j < M; j++) {
      unsigned J = i * M * LOG_N + j * LOG_N;
      unsigned num = 0;
      for (unsigned k = 0; k < LOG_N; k++) {
        num += (unsigned)(var[J + k] << k);
      }
      std::cout << num << ' ';
    }
    std::cout << "\n";
  }
  std::cout << "\n";
}

void build(char* varset, unsigned n, unsigned I) {
  if (I == n - 1) {
    if (varset[I] >= 0) {
      var[I] = varset[I];
      print();
      return;
    }
    var[I] = 0;
    print();
    var[I] = 1;
    print();
    return;
  }
  if (varset[I] >= 0) {
    var[I] = varset[I];
    build(varset, n, I + 1);
    return;
  }
  var[I] = 0;
  build(varset, n, I + 1);
  var[I] = 1;
  build(varset, n, I + 1);
}

void fun(char* varset, int size) {
  build(varset, size, 0);
}
