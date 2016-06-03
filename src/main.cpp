#include <algorithm>>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <mutex>
#include <string>
#include <sstream>
#include <thread>
#include <tuple>
#include <vector>
#include "hst_header.h"

namespace {
	const auto BUFFER_COUNT = 128 * 1024;
	const auto BUFFER_SIZE = sizeof(RateInfo) * BUFFER_COUNT;

	const char* TIME_FORMAT = "%Y/%m/%d %T";

	auto conv(double v) {
		auto h = static_cast <int> (v);
		auto l = static_cast <int> (v * 1000) % 1000;
		return std::to_string(h) + "." + std::to_string(l);
	}

	std::ostream& operator<<(std::ostream& o, RateInfo r) {
		time_t tt = r.ctm;
		o << std::put_time(std::localtime(&tt), TIME_FORMAT)
			<< "," << r.open
			<< "," << r.low
			<< "," << r.high
			<< "," << r.close
			<< "," << r.vol
			<< "\n";
		return o;
	}
}

void usage() {
	std::cout << "Hst2Csv hst-source-file-path csv-dest-file-path" << std::endl;
}

auto convert(const char* p, int64_t size) {
	std::vector <RateInfo> r;
	if (p) {
		auto wp = p;
		RateInfo ri{};

		r.reserve(BUFFER_COUNT);
		while (0 < size) {
			memcpy(&ri, wp, sizeof(RateInfo));
			wp += sizeof(RateInfo);
			size -= sizeof(RateInfo);
			r.emplace_back(ri);
		}
	}
	return r;
}

auto convert(std::string hstPath, std::string csvPath, std::function <void(HistoryHeader)> f, int core = std::thread::hardware_concurrency()) {
	std::vector <char> buf(BUFFER_SIZE);
	std::vector <std::thread> thr;

	std::ifstream hst(hstPath, std::ios::in | std::ios::binary);
	if (!hst.is_open()) {
		std::cout << "failed open file: " << hstPath << std::endl;
		return false;
	}

	std::ofstream csv(csvPath, std::ios::out | std::ios::trunc);
	if (!csv.is_open()) {
		std::cout << "failed create file: " << hstPath << std::endl;
		return false;
	}

	HistoryHeader hh{};
	hst.read(reinterpret_cast <char *> (&hh), sizeof(hh));

	//History Header Notify.
	f(hh);

	std::atomic <int64_t> counterR{ 0 };
	std::atomic <int64_t> counterW{ 0 };
	std::atomic <int64_t> total{ 0 };
	std::mutex mR;
	std::mutex mW;

	auto f_read = [&hst, &mR, &counterR](std::vector <char>& refBuf) {
		std::lock_guard <decltype(mR)> l(mR);
		if (hst.eof()) {
			return std::make_tuple(INT64_C(-1), INT64_C(-1));
		}

		return std::make_tuple(
			counterR++,
			static_cast <int64_t> (hst.read(refBuf.data(), refBuf.size()).gcount())
		);
	};

	auto f_write = [&csv, &mW, &counterW](std::ostringstream& str, int64_t index) {
		if (index < counterW) {
			// 書き込む位置が手前なら書けないので捨てる。
			return false;
		}

		// 書き込む位置に来るまで待機。
		while (counterW != index) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		{
			std::lock_guard <decltype(mW)> l(mW);
			// CSV出力。
			csv << str.rdbuf()->str();
		}

		++counterW;
		return true;
	};

	std::generate_n(
		std::back_inserter(thr),
		core,
		[&total, &f_read, &f_write] {
		return std::thread{
			[&total, &f_read, &f_write] {
				std::vector <char> buf(BUFFER_SIZE);

				for (;;) {
					// 読み取り。
					int64_t index;
					int64_t readSize;
					std::tie(index, readSize) = f_read(buf);

					if (INT64_C(0) <= index) {
						std::ostringstream ss;
						for (auto&& each : convert(buf.data(), readSize)) {
							ss << each;
						}

						f_write(ss, index);
						total += readSize;
					} else {
						break;
					}
				}
		}
		};
	}
	);

	for (auto&& each : thr) {
		each.join();
	}

	return true;
}

int main(int argc, char** argv) {
	if (3 != argc) {
		usage();
		return 0;
	}

	auto now = std::chrono::high_resolution_clock::now();

	convert(
		argv[1],
		argv[2],
		[](HistoryHeader h) {
			std::ostringstream ss;
			time_t lastSync = h.last_sync;
			time_t timeSign = h.timesign;

			ss << "------------------------------------------" << "\n";
			ss << "  version: " << h.version << "\n";
			ss << "copyright: " << h.copyright << "\n";
			ss << "   symbol: " << h.symbol << "\n";
			ss << "   period: " << h.period << "\n";
			ss << "   digits: " << h.digits << "\n";
			ss << " lastSync: " << std::put_time(std::localtime(&lastSync), TIME_FORMAT) << "\n";
			ss << " timesign: " << std::put_time(std::localtime(&timeSign), TIME_FORMAT) << "\n";
			ss << "------------------------------------------" << "\n";

			std::cout << ss.rdbuf()->str();
		}
	);

	auto diff = std::chrono::duration_cast <std::chrono::milliseconds> (std::chrono::high_resolution_clock::now() - now);

	std::cout << "processingTime(ms): " << diff.count() << std::endl;
}
