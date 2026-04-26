#pragma once
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

#include "core.h"
#include "basic.h"
#include "struct.h"
#include "distrubution.h"
#include "probability.h"
#include "zinialgo.h"
#include "ioealgo.h"

class AnalysisCache {
   public:
   unique_ptr<GameState> state_;
   unique_ptr<基础逻辑结果> basic_;
   unique_ptr<棋盘结构> structure_;
   unique_ptr<vector<连通块地雷分布>> dists_[2];
   unique_ptr<高级分析结果> advanced_;
   unique_ptr<地雷概率> probability_;
   public:
   AnalysisCache(GameState gs) : state_(make_unique<GameState>(gs)) {}
   const 基础逻辑结果& get_基础逻辑结果() {
	  if (!basic_) basic_ = make_unique<基础逻辑结果>(基础逻辑分析().analyze(*state_));
	  return *basic_;
   }
   const 棋盘结构& get_棋盘结构() {
	  if (!structure_) structure_ = make_unique<棋盘结构>(连通块构造().brute_build_struct(*state_, get_基础逻辑结果()));
	  return *structure_;
   }
   const vector<连通块地雷分布>& get_地雷分布(bool deep) {
	  if (!dists_[deep]) dists_[deep] = make_unique<vector<连通块地雷分布>>(连通块地雷分布计算().analysis(get_棋盘结构(), deep));
	  return *dists_[deep];
   }
   const 高级分析结果& get_高级分析结果() {
	  if (!advanced_) advanced_ = make_unique<高级分析结果>(概率分析().analysis(*state_, get_基础逻辑结果(), get_棋盘结构(), get_地雷分布(false)));
	  return *advanced_;
   }
   const 地雷概率& get_地雷概率() {
	  if (!probability_) probability_ = make_unique<地雷概率>(概率分析().transfer(*state_, get_基础逻辑结果(), get_棋盘结构(), get_地雷分布(false), get_高级分析结果()));
	  return *probability_;
   }
   地雷排布 genRandom(unsigned long long& seed) {
	  return 概率分析().gen_random(*state_, get_基础逻辑结果(), get_棋盘结构(), get_地雷分布(true), seed);
   }
   template<typename Callback>
   void all_distrubte(Callback callback) {
	  概率分析().all_distrubte(*state_, get_基础逻辑结果(), get_棋盘结构(), get_地雷分布(true), callback);
   }
   高ZNE版面结果 get_highZNE(unsigned long long& seed, long double znereq, int cls, int algo_itr, int zini_itr = 20) {
	  return ioealgo().get_highZNE(*state_, get_基础逻辑结果(), get_棋盘结构(), get_地雷分布(true), get_高级分析结果(), seed, znereq, cls, algo_itr, zini_itr);
   }
   ZNR计算结果 get_ZNR(unsigned long long& seed, long double znereq, int cls, int algo_itr = 100000, int zini_itr = 20) {
	  return ioealgo().get_ZNR(*state_, get_基础逻辑结果(), get_棋盘结构(), get_地雷分布(true), get_高级分析结果(), seed, znereq, cls, algo_itr, zini_itr);
   }
};