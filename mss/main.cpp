#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include <array>
#include <unordered_set>
#include <iomanip>
#include <memory>
#include <cmath>
#include <cassert>
#include <chrono>
#include <limits>
#include <functional>
#include <bit>


using namespace std;

#include "core.h"          // 数据结构
#include "basic.h"         // 基础逻辑分析算法（降低核心算法压力）
#include "struct.h"        // 建立棋盘的图论结构（连通块等）
#include "distrubution.h"  // 根据结构计算地雷分布
#include "probability.h"   // 根据地雷分布计算概率，生成随机分布等等
#include "zinialgo.h"      // 包含 Zini 算法的实现
#include "ioealgo.h"       // 包含 ioe 算法的实现
#include "analysiscache.h" // 管理分析结果的缓存，避免重复计算，一键导入上下文


int main() {
   int n, m, mines; char t;
   if (!(cin >> m >> t >> n >> t >> mines)) return 0;

   int R = n, C = m;
   vector<string> rows;
   rows.reserve(R);
   for (int i = 0; i < R; ++i) {
      string s; cin >> s;
      if ((int)s.size() < C) s.append(C - (int)s.size(), 'H');
      rows.push_back(s);
   }

   GameState gs(R, C, mines);
   地雷排布 pa;
   pa.dist = vector<vector<bool>>(R + 1, vector<bool>(C + 1, false));

   for (int i = 0; i < R; ++i) {
      for (int j = 0; j < C; ++j) {
         char ch = rows[i][j];
         if (ch == 'H' || ch == 'h') {
            gs.board[i + 1][j + 1] = GameState::Cell::H;
            pa.dist[i + 1][j + 1] = false;
         }
         else if (ch == 'F' || ch == 'f') {
            gs.flags[i + 1][j + 1] = true;
            gs.board[i + 1][j + 1] = GameState::Cell::H;
            pa.dist[i + 1][j + 1] = true; // F also sets mine distribution
         }
         else if (ch == 'M' || ch == 'm') {
            // New input: M marks a mine in distribution but GameState remains hidden
            gs.board[i + 1][j + 1] = GameState::Cell::H;
            pa.dist[i + 1][j + 1] = true;
         }
         else if (ch >= '0' && ch <= '8') {
            gs.board[i + 1][j + 1] = static_cast<GameState::Cell>(ch - '0');
            pa.dist[i + 1][j + 1] = false;
         }
         else {
            gs.board[i + 1][j + 1] = GameState::Cell::H;
            pa.dist[i + 1][j + 1] = false;
         }
      }
   }

   cerr << "[Mainboard]" << endl;
   for (int i = 1; i <= gs.rows; ++i) {
      for (int j = 1; j <= gs.cols; ++j) {
         cerr << static_cast<int> (gs.board[i][j]) << " ";
      }
      cerr << endl;
   }


   //AnalysisCache cache(gs);
   unsigned long long seed = 18075243459941470590;
   // 地雷排布 pa 已由输入指定; 不再用随机生成

   for (int i = 1; i <= R; ++i) {
      for (int j = 1; j <= C; ++j) {
         cerr << ".#"[pa.dist[i][j]];
      }
      cerr << endl;
   }

   ZiniAlgo zini;
   Zini结果 res = zini.ChainZini<false>(gs, pa, seed , 10000);
   cerr << res.bbbv << ' ' << res.Zini << endl; 

   for (int i = 1; i <= R; ++i) {
      for (int j = 1; j <= C; ++j) {
         if (pa.dist[i][j] == false) {
            Zini结果 tmp_res = zini.ChainZini<true>(gs, pa, seed, 10000 / R / C, i, j);
            cerr << setw(2) << res.Zini - tmp_res.Zini << ' ';

         }
         else cerr << setw(2) << "##" << ' ';
      }
      cerr << endl;
   }
}