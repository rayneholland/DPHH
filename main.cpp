#include <cfloat>
#include <cstdint>
#include <immintrin.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <numeric>
#include <iomanip>

#include "heavy/CMSSOHH.h"
#include "heavy/CSSOHH.h"
#include "heavy/MGSO.h"
#include "heavy/SSSO.h"

using namespace std;

static constexpr int NUM_REPEATS = 20;

struct AggStats {
    double mean;
    double p5;
    double p95;
};

AggStats computeStats(std::vector<double>& v) {
    std::sort(v.begin(), v.end());
    AggStats s;
    s.mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();

    auto pct = [&](double q) {
        size_t idx = static_cast<size_t>(q * (v.size() - 1));
        return v[idx];
    };

    s.p5  = pct(0.05);
    s.p95 = pct(0.95);
    return s;
}

std::vector<int> parseIp(const std::string& ip) {
    std::vector<int> result;
    std::stringstream ss(ip);
    std::string token;
    while (std::getline(ss, token, '.')) {
        result.push_back(std::stoi(token));
    }
    return result;
}

vector<int> generateRandomItems(int length, int min_val, int max_val, double skew) {
    random_device rd;
    mt19937 gen(rd());
    vector<double> probabilities(max_val - min_val + 1);
    double sum = 0.0;

    for (int i = 0; i < probabilities.size(); ++i) {
        probabilities[i] = 1.0 / pow(i + 1, skew);
        sum += probabilities[i];
    }

    for (double &p : probabilities) {
        p /= sum;
    }

    discrete_distribution<int> dis(probabilities.begin(), probabilities.end());
    vector<int> items;
    for (int i = 0; i < length; ++i) {
        items.push_back(min_val + dis(gen));
    }
    return items;
}

struct HHTestResult {
    double updateTime;
    double ARE;
    double precision;
    double recall;
};

HHTestResult testHH(SketchHH& algo, const vector<int>& stream, int k) {
    unordered_map<int, int> exact_counts;
    auto start = chrono::high_resolution_clock::now();

    for (int item : stream) {
        exact_counts[item]++;
        algo.update(item);
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double, std::micro> duration = end - start;
    double update_time_per_item = duration.count() / stream.size();

    // Compute threshold F_1 / k
    int F1 = 0;
    for (const auto& pair : exact_counts) {
        F1 += pair.second;
    }
    double hh_threshold = static_cast<double>(F1) / k;

    // True heavy hitters
    unordered_map<int, int> true_hh;
    for (const auto& pair : exact_counts) {
        if (pair.second >= hh_threshold) {
            true_hh[pair.first] = pair.second;
        }
    }

    // Retrieved heavy hitters
    const auto& reported = algo.query();

    // Track ARE and coverage
    double total_relative_error = 0.0;
    int matched = 0;
    unordered_set<int> reported_ids;
    for (const auto& pair : reported) {
        reported_ids.insert(pair.first);
    }

    for (const auto& pair : reported) {
        int item = pair.first;
        double est_freq = pair.second;

        auto it = exact_counts.find(item);
        if (it != exact_counts.end()) {
            double exact_freq = it->second;
            total_relative_error += abs(est_freq - exact_freq) / exact_freq;
        }
    }

    for (const auto& hh : true_hh) {
        if (reported_ids.count(hh.first)) {
            matched++;
        }
    }

    double ARE = reported.empty() ? 0.0 : total_relative_error / reported.size();
    double recall = true_hh.empty() ? 1.0 : static_cast<double>(matched) / true_hh.size();
    double precision = reported.empty() ? 1.0 : static_cast<double>(matched) / reported.size();

    return {update_time_per_item, ARE, precision, recall};
}

bool ipStringToInt(const std::string& input, int& output) {
    // Remove quotes
    std::string s = input;
    if (!s.empty() && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
    }

    // Quick heuristic: IPv4 must have 3 dots
    if (std::count(s.begin(), s.end(), '.') != 3) {
        return false;  // Not IPv4, ignore
    }

    std::istringstream ss(s);
    std::string token;
    uint32_t result = 0;

    for (int i = 0; i < 4; ++i) {
        if (!std::getline(ss, token, '.')) {
            return false;  // invalid format
        }
        try {
            int part = std::stoi(token);
            if (part < 0 || part > 255) return false;  // invalid octet
            result = (result << 8) | part;
        } catch (...) {
            return false;  // stoi failed -> ignore
        }
    }

    output = result;
    return true;
}


static constexpr size_t DEFAULT_STREAM_LENGTH = (1u << 24);
static constexpr int DEFAULT_MIN_VAL = 0;
static constexpr int DEFAULT_MAX_VAL = 100000;

static constexpr double TILDE_K_FACTOR = 2;

static constexpr int DEFAULT_K   = 128;
static constexpr int DEFAULT_K_CAIDA   = 256;
static constexpr int DEFAULT_K_CAIDA_FIXED = 512;
static constexpr double DEFAULT_EPS  = 0.1;
static constexpr double DEFAULT_SKEW = 1.1;
static constexpr double DEFAULT_SKEW_EPS = 1.1;

static constexpr double DEFAULT_DELTA = 0.001;
static constexpr int    DEFAULT_DEPTH = 32;
static constexpr uint32_t DEFAULT_SEED = 42;

void runHHAlgorithmsAgg(std::ofstream& ofs,
                        const std::vector<int>& stream,
                        int k,
                        size_t tilde_k,
                        double eps,
                        double skew,
                        size_t stream_length)
{
    auto run_and_aggregate =
        [&](const std::string& name,
            auto&& make_algo,
            size_t tilde_k_local,
            double delta,
            int depth,
            int seed)
    {
        std::vector<double> times, are, prec, rec;

        for (int r = 0; r < NUM_REPEATS; ++r) {
            auto algo = make_algo();
            HHTestResult res = testHH(*algo, stream, k);

            times.push_back(res.updateTime);
            are.push_back(res.ARE);
            prec.push_back(res.precision);
            rec.push_back(res.recall);
        }

        auto t = computeStats(times);
        auto a = computeStats(are);
        auto p = computeStats(prec);
        auto r = computeStats(rec);

        ofs << name << "," << k << "," << tilde_k_local << ","
            << eps << "," << delta << ","
            << depth << "," << seed << ","
            << skew << "," << stream_length << ","
            << std::fixed << std::setprecision(6)
            << t.mean << "," << t.p5 << "," << t.p95 << ","
            << a.mean << "," << a.p5 << "," << a.p95 << ","
            << p.mean << "," << p.p5 << "," << p.p95 << ","
            << r.mean << "," << r.p5 << "," << r.p95 << "\n";

        std::cout << name << " | k=" << k
                  << " eps=" << eps
                  << " skew=" << skew
                  << " | update(us/item)=" << t.mean
                  << " update(5th)=" << t.p5
                    << " update(95th)=" << t.p95
                  << " | ARE=" << a.mean
                  << " P=" << p.mean
                  << " R=" << r.mean << std::endl;
    };


    // -------- MGSO --------
    run_and_aggregate(
        "MGSO",
        [&]() {
            return std::make_unique<MGSO>(k, tilde_k, eps, DEFAULT_DELTA);
        },
        0, DEFAULT_DELTA, 0, 0
    );

    // -------- SSSO --------
    run_and_aggregate(
        "SSSO",
        [&]() {
            return std::make_unique<SSSO>(k, tilde_k, eps, DEFAULT_DELTA);
        },
        tilde_k, DEFAULT_DELTA, 0, 0
    );

    // -------- CMSSOHH --------
    run_and_aggregate(
        "CMSSOHH",
        [&]() {
            int seed = rand();
            return std::make_unique<CMSSOHH>(
                DEFAULT_DEPTH, eps, DEFAULT_DELTA,
                k, 2*tilde_k, seed, stream_length
            );
        },
        tilde_k, 0.0, DEFAULT_DEPTH, DEFAULT_SEED
    );

    // -------- CSSOHH --------
    run_and_aggregate(
        "CSSOHH",
        [&]() {
            return std::make_unique<CSSOHH>(
                DEFAULT_DEPTH, eps, DEFAULT_DELTA,
                k, 2*tilde_k, rand(), rand(), stream_length
            );
        },
        tilde_k, 0.0, DEFAULT_DEPTH, DEFAULT_SEED
    );

}

void runHHExperiments(
    size_t stream_length = DEFAULT_STREAM_LENGTH,
    int min_val = DEFAULT_MIN_VAL,
    int max_val = DEFAULT_MAX_VAL,
    const std::string& out_csv = "hh_experiments_results.csv"
) {
    // Parameter grids (provided)
    const std::vector<int>    k_grid    = {32, 64, 128, 256, 512, 1024};
    const std::vector<double> eps_grid  = {0.001, 0.01, 0.1, 1.0, 10.0};
    const std::vector<double> skew_grid = {1.1,1.4,1.7,2.0,2.3,2.6};

    // Prepare CSV
    std::ofstream ofs(out_csv);
    if (!ofs) {
        std::cerr << "[ERROR] Could not open output file: " << out_csv << std::endl;
        return;
    }

    ofs << "algo,k,tilde_k,eps,delta,depth,seed,skew,stream_len,"
       "update_mean,update_p5,update_p95,"
       "ARE_mean,ARE_p5,ARE_p95,"
       "precision_mean,precision_p5,precision_p95,"
       "recall_mean,recall_p5,recall_p95\n";


    std::cout << "=== Heavy-Hitter Experiments ===\n"
              << "Stream length: " << stream_length << "\n"
              << "Domain: [" << min_val << ", " << max_val << "]\n"
              << "Defaults => k=" << DEFAULT_K
              << ", eps=" << DEFAULT_EPS
              << ", skew=" << DEFAULT_SKEW << "\n\n";

    // ============================================================
    // 1) Sweep over k ONLY: eps = DEFAULT_EPS, skew = DEFAULT_SKEW
    // ============================================================
    {
        const double eps  = DEFAULT_EPS;
        const double skew = DEFAULT_SKEW;

        std::vector<int> stream = generateRandomItems(
            static_cast<int>(stream_length),
            min_val, max_val,
            skew
        );

        for (int k : k_grid) {
            auto tilde_k = static_cast<size_t>(k*TILDE_K_FACTOR);
            runHHAlgorithmsAgg(ofs, stream, k, tilde_k, eps, skew, stream_length);
        }

        std::cout << "[INFO] Completed k-sweep.\n\n";
    }

    // ============================================================
    // 2) Sweep over eps ONLY: k = DEFAULT_K, skew = DEFAULT_SKEW
    // ============================================================
    {
        const int    k    = DEFAULT_K;
        const double skew = DEFAULT_SKEW_EPS;

        std::vector<int> stream = generateRandomItems(
            static_cast<int>(stream_length),
            min_val, max_val,
            skew
        );

        for (double eps : eps_grid) {
            auto tilde_k = static_cast<size_t>(DEFAULT_K*TILDE_K_FACTOR);
            runHHAlgorithmsAgg(ofs, stream, DEFAULT_K, tilde_k, eps,
                                         DEFAULT_SKEW, stream_length);
        } // eps

        std::cout << "[INFO] Completed eps-sweep.\n\n";
    }

    // ============================================================
    // 3) Sweep over skew ONLY: k = DEFAULT_K, eps = DEFAULT_EPS
    // ============================================================
    {
        const int    k   = DEFAULT_K;
        const double eps = DEFAULT_EPS;

        for (double skew : skew_grid) {
            std::vector<int> stream = generateRandomItems(
                static_cast<int>(stream_length),
                min_val, max_val,
                skew
            );
            auto tilde_k = static_cast<size_t>(DEFAULT_K*TILDE_K_FACTOR);
            runHHAlgorithmsAgg(ofs, stream, DEFAULT_K, tilde_k, DEFAULT_EPS,
                                         skew, stream_length);
        }
    }

    ofs.close();
    std::cout << "\n[DONE] Results written to: " << out_csv << std::endl;
}

void runHHExperimentsCaida(
    const std::string& caida_csv =
        "C:/Users/HOL446/CLionProjects/CODPSketches/data/packet_capture.csv",
    const std::string& out_csv = "hh_experiments_caida.csv"
) {
    // Parameter grids (same as synthetic experiments)
    const std::vector<int>    k_grid   = {128, 256, 512, 1024, 2048, 4096};
    const std::vector<double> eps_grid = {0.001, 0.01, 0.1, 1.0, 10.0};
    const std::vector<int> k_tilde_factor_grid = {1,2,4,8,16};

    // ------------------------------------------------------------
    // Load CAIDA stream (source IPs)
    // ------------------------------------------------------------
    std::ifstream file(caida_csv);
    if (!file) {
        std::cerr << "[ERROR] Could not open CAIDA file: "
                  << caida_csv << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); // skip header

    std::vector<int> stream;
    std::unordered_set<int> unique_ips;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        int fieldIndex = 0;
        std::string sourceIP;

        while (std::getline(ss, field, ',')) {
            if (fieldIndex == 2) { // Source IP column
                sourceIP = field;
                break;
            }
            ++fieldIndex;
        }

        int ipInt;
        if (ipStringToInt(sourceIP, ipInt)) {
            ipInt = abs(ipInt);
            stream.push_back(ipInt);
            unique_ips.insert(ipInt);
        }
    }

    file.close();

    const size_t stream_length = stream.size();

    std::cout << "=== CAIDA Heavy-Hitter Experiments ===\n"
              << "Stream length: " << stream_length << "\n"
              << "Distinct IPs:  " << unique_ips.size() << "\n"
              << "Skew: REAL (fixed)\n\n";

    // ------------------------------------------------------------
    // Prepare CSV
    // ------------------------------------------------------------
    std::ofstream ofs(out_csv);
    if (!ofs) {
        std::cerr << "[ERROR] Could not open output file: "
                  << out_csv << std::endl;
        return;
    }

    ofs << "algo,k,tilde_k,eps,delta,depth,seed,skew,stream_len,"
           "update_mean,update_p5,update_p95,"
           "ARE_mean,ARE_p5,ARE_p95,"
           "precision_mean,precision_p5,precision_p95,"
           "recall_mean,recall_p5,recall_p95\n";

    // ============================================================
    // 1) Sweep over k ONLY (ε fixed)
    // ============================================================
    {
        const double eps  = DEFAULT_EPS;
        const double skew = 0.0; // real stream (placeholder)

        for (int k : k_grid) {
            auto tilde_k = static_cast<size_t>(k*TILDE_K_FACTOR);
            runHHAlgorithmsAgg(
                ofs,
                stream,
                k,
                tilde_k,
                eps,
                skew,
                stream_length
            );
        }

        std::cout << "[INFO] Completed k-sweep (CAIDA).\n\n";
    }

    // ============================================================
    // 2) Sweep over eps ONLY (k fixed)
    // ============================================================
    {
        const int    k    = DEFAULT_K_CAIDA;
        const double skew = 0.0; // real stream

        for (double eps : eps_grid) {
            auto tilde_k = static_cast<size_t>(DEFAULT_K*TILDE_K_FACTOR);
            runHHAlgorithmsAgg(
                ofs,
                stream,
                k,
                tilde_k,
                eps,
                skew,
                stream_length
            );
        }

        std::cout << "[INFO] Completed eps-sweep (CAIDA).\n\n";
    }

    // ============================================================
    // 3) Sweep over tilde_k (k fixed)
    // ============================================================
    {
        const int    k    = DEFAULT_K_CAIDA_FIXED;
        const double eps = DEFAULT_EPS;
        const double skew = 0.0; // real stream

        for (int factor : k_tilde_factor_grid) {
            size_t tilde_k = (factor*k);
            runHHAlgorithmsAgg(
                ofs,
                stream,
                k,
                tilde_k,
                eps,
                skew,
                stream_length
            );
        }

        std::cout << "[INFO] Completed eps-sweep (CAIDA).\n\n";
    }

    ofs.close();
    std::cout << "[DONE] CAIDA results written to: "
              << out_csv << std::endl;
}

int main() {

    runHHExperiments();

    runHHExperimentsCaida();

    return 0;
}


