// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "iaccelerated.h"
#include "generic.h"
#ifdef __x86_64__
#include "avx2.h"
#include "avx512.h"
#endif
#include <vespa/vespalib/util/memory.h>
#include <cstdio>
#include <vector>

#include <vespa/log/log.h>
LOG_SETUP(".vespalib.hwaccelerated");

namespace vespalib::hwaccelerated {

namespace {

IAccelerated::UP create_accelerator() {
#ifdef __x86_64__
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx512f")) {
        return std::make_unique<Avx512Accelrator>();
    }
    if (__builtin_cpu_supports("avx2")) {
        return std::make_unique<Avx2Accelrator>();
    }
#endif
    return std::make_unique<GenericAccelrator>();
}

template<typename T>
std::vector<T> createAndFill(size_t sz) {
    std::vector<T> v(sz);
    for (size_t i(0); i < sz; i++) {
        v[i] = rand()%100;
    }
    return v;
}

template<typename T>
void
verifyDotproduct(const IAccelerated & accel)
{
    const size_t testLength(255);
    srand(1);
    std::vector<T> a = createAndFill<T>(testLength);
    std::vector<T> b = createAndFill<T>(testLength);
    for (size_t j(0); j < 0x20; j++) {
        T sum(0);
        for (size_t i(j); i < testLength; i++) {
            sum += a[i]*b[i];
        }
        T hwComputedSum(accel.dotProduct(&a[j], &b[j], testLength - j));
        if (sum != hwComputedSum) {
            fprintf(stderr, "Accelrator is not computing dotproduct correctly.\n");
            LOG_ABORT("should not be reached");
        }
    }
}

template<typename T>
void
verifyEuclideanDistance(const IAccelerated & accel) {
    const size_t testLength(255);
    srand(1);
    std::vector<T> a = createAndFill<T>(testLength);
    std::vector<T> b = createAndFill<T>(testLength);
    for (size_t j(0); j < 0x20; j++) {
        T sum(0);
        for (size_t i(j); i < testLength; i++) {
            sum += (a[i] - b[i]) * (a[i] - b[i]);
        }
        T hwComputedSum(accel.squaredEuclideanDistance(&a[j], &b[j], testLength - j));
        if (sum != hwComputedSum) {
            fprintf(stderr, "Accelrator is not computing euclidean distance correctly.\n");
            LOG_ABORT("should not be reached");
        }
    }
}

void
verifyPopulationCount(const IAccelerated & accel)
{
    const uint64_t words[7] = {0x123456789abcdef0L,  // 32
                               0x0000000000000000L,  // 0
                               0x8000000000000000L,  // 1
                               0xdeadbeefbeefdeadUL, // 48
                               0x5555555555555555L,  // 32
                               0x00000000000000001,  // 1
                               0xffffffffffffffff};  // 64
    constexpr size_t expected = 32 + 0 + 1 + 48 + 32 + 1 + 64;
    size_t hwComputedPopulationCount = accel.populationCount(words, VESPA_NELEMS(words));
    if (hwComputedPopulationCount != expected) {
        fprintf(stderr, "Accelrator is not computing populationCount correctly.Expected %zu, computed %zu\n", expected, hwComputedPopulationCount);
        LOG_ABORT("should not be reached");
    }
}

void
fill(std::vector<uint64_t> & v, size_t n) {
    v.reserve(n);
    for (size_t i(0); i < n; i++) {
        v.emplace_back(random());
    }
}

void
simpleAndWith(std::vector<uint64_t> & dest, const std::vector<uint64_t> & src) {
    for (size_t i(0); i < dest.size(); i++) {
        dest[i] &= src[i];
    }
}

void
simpleOrWith(std::vector<uint64_t> & dest, const std::vector<uint64_t> & src) {
    for (size_t i(0); i < dest.size(); i++) {
        dest[i] |= src[i];
    }
}

std::vector<uint64_t>
simpleInvert(const std::vector<uint64_t> & src) {
    std::vector<uint64_t> inverted;
    inverted.reserve(src.size());
    for (unsigned long i : src) {
        inverted.push_back(~i);
    }
    return inverted;
}

std::vector<uint64_t>
optionallyInvert(bool invert, std::vector<uint64_t> v) {
    return invert ? simpleInvert(v) : std::move(v);
}

bool shouldInvert(bool invertSome) {
    return invertSome ? (random() & 1) : false;
}

void
verifyOr64(const IAccelerated & accel, const std::vector<std::vector<uint64_t>> & vectors,
           size_t offset, size_t num_vectors, bool invertSome)
{
    std::vector<std::pair<const void *, bool>> vRefs;
    for (size_t j(0); j < num_vectors; j++) {
        vRefs.emplace_back(&vectors[j][0], shouldInvert(invertSome));
    }

    std::vector<uint64_t> expected = optionallyInvert(vRefs[0].second, vectors[0]);
    for (size_t j = 1; j < num_vectors; j++) {
        simpleOrWith(expected, optionallyInvert(vRefs[j].second, vectors[j]));
    }

    uint64_t dest[16] __attribute((aligned(64)));
    accel.or128(offset * sizeof(uint64_t), vRefs, dest);
    int diff = memcmp(&expected[offset], dest, sizeof(dest));
    if (diff != 0) {
        LOG_ABORT("Accelerator fails to compute correct 128 bytes OR");
    }
}

void
verifyAnd64(const IAccelerated & accel, const std::vector<std::vector<uint64_t>> & vectors,
           size_t offset, size_t num_vectors, bool invertSome)
{
    std::vector<std::pair<const void *, bool>> vRefs;
    for (size_t j(0); j < num_vectors; j++) {
        vRefs.emplace_back(&vectors[j][0], shouldInvert(invertSome));
    }
    std::vector<uint64_t> expected = optionallyInvert(vRefs[0].second, vectors[0]);
    for (size_t j = 1; j < num_vectors; j++) {
        simpleAndWith(expected, optionallyInvert(vRefs[j].second, vectors[j]));
    }

    uint64_t dest[16] __attribute((aligned(64)));
    accel.and128(offset * sizeof(uint64_t), vRefs, dest);
    int diff = memcmp(&expected[offset], dest, sizeof(dest));
    if (diff != 0) {
        LOG_ABORT("Accelerator fails to compute correct 128 bytes AND");
    }
}

void
verifyOr64(const IAccelerated & accel) {
    std::vector<std::vector<uint64_t>> vectors(3) ;
    for (auto & v : vectors) {
        fill(v, 32);
    }
    for (size_t offset = 0; offset < 16; offset++) {
        for (size_t i = 1; i < vectors.size(); i++) {
            verifyOr64(accel, vectors, offset, i, false);
            verifyOr64(accel, vectors, offset, i, true);
        }
    }
}

void
verifyAnd64(const IAccelerated & accel) {
    std::vector<std::vector<uint64_t>> vectors(3);
    for (auto & v : vectors) {
        fill(v, 32);
    }
    for (size_t offset = 0; offset < 16; offset++) {
        for (size_t i = 1; i < vectors.size(); i++) {
            verifyAnd64(accel, vectors, offset, i, false);
            verifyAnd64(accel, vectors, offset, i, true);
        }
    }
}

class RuntimeVerificator
{
public:
    RuntimeVerificator();
private:
    static void verify(const IAccelerated & accelerated) {
        verifyDotproduct<float>(accelerated);
        verifyDotproduct<double>(accelerated);
        verifyDotproduct<int32_t>(accelerated);
        verifyDotproduct<int64_t>(accelerated);
        verifyEuclideanDistance<float>(accelerated);
        verifyEuclideanDistance<double>(accelerated);
        verifyPopulationCount(accelerated);
        verifyAnd64(accelerated);
        verifyOr64(accelerated);
    }
};

RuntimeVerificator::RuntimeVerificator()
{
    verify(GenericAccelrator());
    verify(*create_accelerator());
}

}

const IAccelerated &
IAccelerated::getAccelerator()
{
    static RuntimeVerificator verifyAccelrator_once;
    static IAccelerated::UP accelrator = create_accelerator();
    return *accelrator;
}

}
