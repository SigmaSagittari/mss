#pragma once

void 初级Zini测试() {
   int n = 6, m = 7, mines = 16;
   GameState gs(n, m, mines);
   for (int i = 1; i <= n; ++i)
	  for (int j = 1; j <= m; ++j)
		 gs.board[i][j] = GameState::Cell::H;

   AnalysisCache cache(gs);
   unsigned long long result = 0;

   // 统计 ioe = bbbv / Zini
   map<int, int> ioeCount;  // ioe * 100 的整数值 -> 出现次数
   int totalValid = 0;

   auto start = chrono::high_resolution_clock::now();

   for (int i = 1; i <= 100000000; ++i) {
	  unsigned long long seed = i;
	  地雷排布 t = cache.genRandom(seed);
	  auto res = ZiniAlgo().ChainZini(gs, t, seed);
	  volatile auto tmp = res;
	  result = splitmix64(result + (unsigned long long) tmp.Zini * i);

	  // 统计 ioe
	  if (tmp.Zini != 0) {
		 double ioe = (double)tmp.bbbv / tmp.Zini * 100.0;  // 百分比
		 int key = (int)round(ioe);  // 四舍五入到整数百分比
		 ioeCount[key]++;
		 totalValid++;
	  }
	  if (i % 1000000 == 0) cerr << i / 1000000 << '%' << endl;
   }

   auto end = chrono::high_resolution_clock::now();
   auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

   cout << "Test completed, result: " << result << endl;
   cout << "Time: " << duration.count() / 1000.0 << " seconds" << endl;
   cout << "\n=== ioe (bbbv/Zini) 统计 (精确到百分位) ===" << endl;
   cout << fixed << setprecision(8);  // 改为4位小数

   // 输出统计结果，累积百分比
   int cumulative = 0;
   for (const auto& [ioePercent, count] : ioeCount) {
	  cumulative += count;
	  double prefix = (double)cumulative / totalValid * 100.0;
	  double suffix = 100.0 - prefix;
	  cout << "ioe " << ioePercent << "%"
		 << "  前缀=" << prefix << "%"
		 << "  后缀=" << suffix << "%"
		 << "  (次数=" << count << ")" << endl;
   }
}


void 多线程Zini测试(int N,int M,int MINES) {

   FILE* __outFile = nullptr;
   {
	  string path = string("C:\\Users\\19429\\Downloads\\ioe_table\\") + to_string(N) + "x" + to_string(M) + "x" + to_string(MINES) + ".txt";
	  if (freopen_s(&__outFile, path.c_str(), "w", stdout) != 0) {
		 cerr << "无法重定向 stdout，继续使用默认 stdout" << endl;
	  }
   }

   cerr << N << "x" << M << "/" << MINES << "TESTING:" << endl;
   const long long ITERATIONS = 10000000;
   const unsigned int NUM_THREADS = 8;

   GameState gs(N, M, MINES);
   for (int i = 1; i <= N; ++i)
	  for (int j = 1; j <= M; ++j)
		 gs.board[i][j] = GameState::Cell::H;

   // 统计 ioe = bbbv / Zini
   map<int, long long> ioeCount;  // ioe * 100 的整数值 -> 出现次数
   long long totalValid = 0;
   mutex mtx;

   auto start = chrono::high_resolution_clock::now();

   auto worker = [&](unsigned long long start_seed, unsigned long long end_seed) {
	  AnalysisCache cache(gs);
	  map<int, long long> localIoeCount;
	  long long localValid = 0;

	  for (unsigned long long i = start_seed; i < end_seed; ++i) {
		 unsigned long long seed = i;
		 auto t = cache.genRandom(seed);
		 auto res = ZiniAlgo().ChainZini(gs, t, seed, 8);

		 if (res.Zini != 0) {
			double ioe = (double)res.bbbv / res.Zini * 100.0;
			int key = (int)round(ioe);
			localIoeCount[key]++;
			localValid++;
		 }
	  }

	  lock_guard<mutex> lock(mtx);
	  for (auto& [key, val] : localIoeCount) {
		 ioeCount[key] += val;
	  }
	  totalValid += localValid;
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

   cout << fixed << setprecision(8);

   long long cumulative = 0;
   for (const auto& [ioePercent, count] : ioeCount) {
	  cumulative += count;
	  double prefix = (double)cumulative / totalValid * 100.0;
	  double suffix = 100.0 - prefix;
	  cout << "ioe " << ioePercent << "%"
		 << "  前缀=" << prefix << "%"
		 << "  后缀=" << suffix << "%"
		 << "  (次数=" << count << ")" << endl;
   }

   cerr << "Time: " << duration.count() / 1000.0 << " seconds" << endl;
}


void ZNR算法测试() {

   int n, m, mines; char t;
   if (!(cin >> m >> t >> n >> t >> mines)) return;

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
   pa.dist.resize(R + 1, C + 1, false);

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


   unsigned long long seed = 18075243459941470590;


   // 由用户输入 znereq 和 cls，调用 AnalysisCache 的 get_ZNR 并格式化输出
   long double znereq = 0.0;
   int cls = 0;
   if (!(cin >> znereq >> cls)) {
	  cerr << "No znereq and cls provided, exiting." << endl;
	  return;
   }

   AnalysisCache cache(gs);

   地雷概率 ttt = cache.get_地雷概率();

   cerr << "[probability]" << endl;
   for (int i = 1; i <= gs.rows; ++i) {
	  for (int j = 1; j <= gs.cols; ++j) {
		 cerr << ttt.probability[i][j] << " ";
	  }
	  cerr << endl;
   }


   ZNR计算结果 znr = cache.get_ZNR_new(seed, znereq, cls, ioealgo::软指数加权, 1000000, 20);

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
   cout << "\nZNR 操作列表 (坐标 x,y ; 周围标记矩阵 3x3 ; weight):\n";
   stable_sort(znr.ZNR.begin(), znr.ZNR.end(), [](const auto& a, const auto& b) {
	  return a.weight > b.weight;
   }); // 排序
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
	  cout << "的 权重: " << item.weight << '\n';
   }
}

void Zini_test() {
   int n, m, mines; char t;
   if (!(cin >> m >> t >> n >> t >> mines)) return;

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
   pa.dist.resize(R + 1, C + 1, false);

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

   unsigned long long seed = 19260817;
   Zini结果 res = ZiniAlgo().ChainZini(gs, pa, seed, 100);
   cerr << res.bbbv << ' ' << res.Zini << endl;
}