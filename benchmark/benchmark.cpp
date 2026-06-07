//
// Created by sayan on 10/4/25.
//

#include "../fastcast.hpp"
#include "../tests/utilities.hpp"
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
      accumulated += fast_cast<SimpleB &>(a).method_b_only();
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
      accumulated += fast_cast<ComplexG &>(a).method_g_only();
    }
    benchmark::DoNotOptimize(accumulated);
  }
}
BENCHMARK(BM_FastDynamicCast_Complex)
    ->Arg(DefaultInnerIterations)
    ->Unit(benchmark::kMillisecond);

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
    benchmark::DoNotOptimize(fast_cast<SimpleB *>(a));
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
    benchmark::DoNotOptimize(fast_cast<SimpleB *>(ap));
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
    benchmark::DoNotOptimize(fast_cast<SimpleB &>(a));
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

static constexpr size_t kColdTypes = 5;

template <typename Cast>
static inline void run_rotating(benchmark::State &state, bool rotate,
                                Cast &&cast) {
  ComplexC c;
  ComplexD d;
  ComplexE e;
  ComplexF f;
  ComplexG g;
  ComplexA *objs[kColdTypes] = {&c, &d, &e, &f, &g};
  size_t i = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(cast(objs[i]));
    i = rotate ? (i + 1) % kColdTypes : 0;
  }
}

static void BM_FastDynamicCast_Cold(benchmark::State &state) {
  run_rotating(state, /*rotate=*/true,
               [](ComplexA *p) { return fast_cast<ComplexB *>(p); });
}
BENCHMARK(BM_FastDynamicCast_Cold);

static void BM_FastDynamicCast_Hot(benchmark::State &state) {
  run_rotating(state, /*rotate=*/false,
               [](ComplexA *p) { return fast_cast<ComplexB *>(p); });
}
BENCHMARK(BM_FastDynamicCast_Hot);

static void BM_DynamicCast_Cold(benchmark::State &state) {
  run_rotating(state, /*rotate=*/true,
               [](ComplexA *p) { return dynamic_cast<ComplexB *>(p); });
}
BENCHMARK(BM_DynamicCast_Cold);

static void BM_DynamicCast_Hot(benchmark::State &state) {
  run_rotating(state, /*rotate=*/false,
               [](ComplexA *p) { return dynamic_cast<ComplexB *>(p); });
}
BENCHMARK(BM_DynamicCast_Hot);

static void BM_DerivedToBase_FastCast(benchmark::State &state) {
  Derived d;
  Derived *dp = &d;
  for (auto _ : state) {
    auto r = fastcast::fast_cast<Base *>(dp);
    benchmark::DoNotOptimize(r);
  }
}
BENCHMARK(BM_DerivedToBase_FastCast)->Arg(DefaultInnerIterations);

static void BM_DerivedToBase_StaticCast(benchmark::State &state) {
  Derived d;
  Derived *dp = &d;
  for (auto _ : state) {
    auto r = static_cast<Base *>(dp);
    benchmark::DoNotOptimize(r);
  }
}
BENCHMARK(BM_DerivedToBase_StaticCast)->Arg(DefaultInnerIterations);

static void BM_DerivedToBase_DynamicCast(benchmark::State &state) {
  Derived d;
  Derived *dp = &d;
  for (auto _ : state) {
    auto r = dynamic_cast<Base *>(dp);
    benchmark::DoNotOptimize(r);
  }
}
BENCHMARK(BM_DerivedToBase_DynamicCast)->Arg(DefaultInnerIterations);

BENCHMARK_MAIN();