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
#include <thread>
#include <mutex>

bool GLOBAL_DEBUG = false;

using namespace std;

#include "core.h"          // 数据结构
#include "basic.h"         // 基础逻辑分析算法（降低核心算法压力）
#include "struct.h"        // 建立棋盘的图论结构（连通块等）
#include "distrubution.h"  // 根据结构计算地雷分布
#include "probability.h"   // 根据地雷分布计算概率，生成随机分布等等
#include "zinialgo.h"      // 包含 Zini 算法的实现
#include "ioealgo.h"       // 包含 ioe 算法的实现
#include "analysiscache.h" // 管理分析结果的缓存，避免重复计算，一键导入上下文

void test() {
   GameState gs(9, 9, 10);
   for (int i = 1; i <= 9; ++i)
      for (int j = 1; j <= 9; ++j)
         gs.board[i][j] = GameState::Cell::H;

   AnalysisCache cache(gs);
   unsigned long long result = 0;

   auto start = chrono::high_resolution_clock::now();

   for (int i = 1; i <= 1000000; ++i) { //  一百万
      unsigned long long seed = i;
      地雷排布 t = cache.genRandom(seed);
      auto res = ZiniAlgo().ChainZini<false>(gs, t, seed);
      volatile auto tmp = res;
      result = splitmix64(result + (unsigned long long) tmp.Zini * i);
   }

   auto end = chrono::high_resolution_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

   cout << "Test completed, result: " << result << endl;
   cout << "Should be " << 6734981085506469534 << endl;
   cout << "Time: " << duration.count() / 1000.0 << " seconds" << endl;

   
}

void test_multithread() {
   const int N = 9;
   const int M = 9;
   const int MINES = 10;
   const long long ITERATIONS = 100000000;
   const unsigned int NUM_THREADS = 6;

   GameState gs(N, M, MINES);
   for (int i = 1; i <= N; ++i)
      for (int j = 1; j <= M; ++j)
         gs.board[i][j] = GameState::Cell::H;

   map<pair<int, int>, long long> cnt;
   mutex mtx;

   auto start = chrono::high_resolution_clock::now();

   auto worker = [&](unsigned long long start_seed, unsigned long long end_seed) {
      AnalysisCache cache(gs);
      map<pair<int, int>, long long> local;

      for (unsigned long long i = start_seed; i < end_seed; ++i) {
         unsigned long long seed = i;  // 关键：每次重新赋值，传副本进去
         auto t = cache.genRandom(seed);
         auto res = ZiniAlgo().ChainZini<false>(gs, t, seed);
         local[{res.Zini, res.bbbv}]++;
      }

      lock_guard<mutex> lock(mtx);
      for (auto& [key, val] : local) {
         cnt[key] += val;
      }
   };

   vector<thread> threads;
   unsigned long long step = ITERATIONS / NUM_THREADS;
   for (unsigned int i = 0; i < NUM_THREADS; ++i) {
      unsigned long long start = i * step + 1;
      unsigned long long end = (i == NUM_THREADS - 1) ? ITERATIONS + 1 : (i + 1) * step + 1;
      threads.emplace_back(worker, start, end);
   }

   for (auto& th : threads) {
      th.join();
   }

   auto end = chrono::high_resolution_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

   long long total = 0;
   for (auto& [key, val] : cnt) {
      cout << key.first << ' ' << key.second << ' ' << val << endl;
      total += val;
   }

   cout << "Total iterations: " << total << endl;
   cout << "Time: " << duration.count() / 1000.0 << " seconds" << endl;
}

int main() {
   //test_multithread();
   //return 0;
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


   unsigned long long seed = 18075243459941470590;


   // 由用户输入 znereq 和 cls，调用 AnalysisCache 的 get_ZNR 并格式化输出
   long double znereq = 0.0;
   int cls = 0;
   if (!(cin >> znereq >> cls)) {
      cerr << "No znereq and cls provided, exiting." << endl;
      return 0;
   }

   AnalysisCache cache(gs);

   ZNR计算结果 znr = cache.get_ZNR(seed, znereq, cls);

   // 输出 ZNE 版面统计
   cout << "ZNE版面数量: " << znr.ZNE_result.count << " in All " << znr.ZNE_result.total << "maps (" << (long double)znr.ZNE_result.count / znr.ZNE_result.total * 100 << "%)" << '\n';
   cout << fixed << setprecision(4);
   cout << "ZNE 版面平均地雷概率分布:\n";
   for (int i = 1; i <= R; ++i) {
      for (int j = 1; j <= C; ++j) {
         cout << setw(8) << znr.ZNE_result.dist.probability[i][j] << ' ';
      }
      cout << '\n';
   }
   // 输出每个 ZNR 操作及其概率
   cout << "\nZNR 操作列表 (坐标 x,y ; 周围标记矩阵 3x3 ; probability):\n";
   for (const auto& item : znr.ZNR) {
      const auto& op = item.operation;
      cout << "(" << op.x << "," << op.y << ") ";
      // 输出周围 3x3 标记，按行
      for (int dx = 0; dx < 3; ++dx) {
         for (int dy = 0; dy < 3; ++dy) {
            cout << (op.fl[dx][dy] ? '#' : '.');
         }
         if (dx < 2) cout << ";";
      }
      cout << "  ";
      cout << item.cnt << "项 中 的 权重: " << item.weight << '\n';
   }
}