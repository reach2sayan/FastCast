//
// Created by sayan on 10/4/25.
//

#include "fastcast.hpp"
#include "utilities.hpp"
#include <benchmark/benchmark.h>
#include <thread>

constexpr size_t DefaultInnerIterations = 2000000;

static inline size_t inner_iters(const benchmark::State &state) {
  if (state.range(0) != 0)
    return static_cast<size_t>(state.range(0));
  return DefaultInnerIterations;
}

static void BM_DynamicCast_Simple(benchmark::State &state) {
  const size_t iters = inner_iters(state);
  for (auto _ : state) {
    volatile size_t accumulated = 0;
    for (size_t i = 0; i < iters; ++i) {
      SimpleB b;
      SimpleA &a = b;
      // dynamic_cast to reference (like your original)
      accumulated += dynamic_cast<SimpleB &>(a).method_b_only();
    }
    benchmark::DoNotOptimize(accumulated);
  }
}
BENCHMARK(BM_DynamicCast_Simple)
    ->Arg(DefaultInnerIterations)
    ->Unit(benchmark::kMillisecond);

static void BM_FastDynamicCast_Simple(benchmark::State &state) {
  const size_t iters = inner_iters(state);
  for (auto _ : state) {
    volatile size_t accumulated = 0;
    for (size_t i = 0; i < iters; ++i) {
      SimpleB b;
      SimpleA &a = b;
      accumulated += fast_dynamic_cast<SimpleB &>(a).method_b_only();
    }
    benchmark::DoNotOptimize(accumulated);
  }
}
BENCHMARK(BM_FastDynamicCast_Simple)
    ->Arg(DefaultInnerIterations)
    ->Unit(benchmark::kMillisecond);

static void BM_DynamicCast_Complex(benchmark::State &state) {
  const size_t iters = inner_iters(state);
  for (auto _ : state) {
    volatile size_t accumulated = 0;
    for (size_t i = 0; i < iters; ++i) {
      ComplexG g;
      ComplexA &a = g;
      accumulated += dynamic_cast<ComplexG &>(a).method_g_only();
    }
    benchmark::DoNotOptimize(accumulated);
  }
}
BENCHMARK(BM_DynamicCast_Complex)
    ->Arg(DefaultInnerIterations)
    ->Unit(benchmark::kMillisecond);

static void BM_FastDynamicCast_Complex(benchmark::State &state) {
  const size_t iters = inner_iters(state);
  for (auto _ : state) {
    volatile size_t accumulated = 0;
    for (size_t i = 0; i < iters; ++i) {
      ComplexG g;
      ComplexA &a = g;
      accumulated += fast_dynamic_cast<ComplexG &>(a).method_g_only();
    }
    benchmark::DoNotOptimize(accumulated);
  }
}

static void BM_DynamicCast_Ptr_Success(benchmark::State &state) {
  SimpleB b;
  SimpleA *a = &b;
  for (auto _ : state) {
    benchmark::DoNotOptimize(dynamic_cast<SimpleB *>(a));
  }
}
BENCHMARK(BM_DynamicCast_Ptr_Success);

static void BM_FastDynamicCast_Ptr_Success(benchmark::State &state) {
  SimpleB b;
  SimpleA *a = &b;
  for (auto _ : state) {
    benchmark::DoNotOptimize(fast_dynamic_cast<SimpleB *>(a));
  }
}
BENCHMARK(BM_FastDynamicCast_Ptr_Success);

static void BM_DynamicCast_Ptr_Failure(benchmark::State &state) {
  SimpleA a;
  SimpleA *ap = &a;
  for (auto _ : state) {
    benchmark::DoNotOptimize(dynamic_cast<SimpleB *>(ap));
  }
}
BENCHMARK(BM_DynamicCast_Ptr_Failure);

static void BM_FastDynamicCast_Ptr_Failure(benchmark::State &state) {
  SimpleA a;
  SimpleA *ap = &a;
  for (auto _ : state) {
    benchmark::DoNotOptimize(fast_dynamic_cast<SimpleB *>(ap));
  }
}
BENCHMARK(BM_FastDynamicCast_Ptr_Failure);

static void BM_DynamicCast_Reused(benchmark::State &state) {
  SimpleB b;
  SimpleA &a = b;
  for (auto _ : state) {
    benchmark::DoNotOptimize(dynamic_cast<SimpleB &>(a));
  }
}
BENCHMARK(BM_DynamicCast_Reused);

static void BM_FastDynamicCast_Reused(benchmark::State &state) {
  SimpleB b;
  SimpleA &a = b;
  for (auto _ : state) {
    benchmark::DoNotOptimize(fast_dynamic_cast<SimpleB &>(a));
  }
}
BENCHMARK(BM_FastDynamicCast_Reused);

// Multi-thread stress
BENCHMARK(BM_DynamicCast_Reused)->Threads(2)->Threads(4)->Threads(8);
BENCHMARK(BM_FastDynamicCast_Reused)->Threads(2)->Threads(4)->Threads(8);

BENCHMARK(BM_DynamicCast_Ptr_Success);
BENCHMARK(BM_FastDynamicCast_Complex)
    ->Arg(DefaultInnerIterations)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_DynamicCast_Complex)
    ->Arg(DefaultInnerIterations)
    ->Threads(2)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_FastDynamicCast_Complex)
    ->Arg(DefaultInnerIterations)
    ->Threads(2)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();