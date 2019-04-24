#include <pcv/utils/benchmark_common.h>

namespace pcv {

BenchmarkStats::BenchmarkStats(size_t LOOKUP_CNT, bool dump_as_json,
		size_t real_rule_cnt, std::ostream & out) :
		LOOKUP_CNT(LOOKUP_CNT), dump_as_json(dump_as_json), out(out), construction_timer(
				nullptr), lookup_timer(nullptr), real_rule_cnt(real_rule_cnt), number_or_tries_or_tables(
				-1) {
}
void BenchmarkStats::construction_start() {
	construction_timer = new DebugTimer();
}
void BenchmarkStats::construction_stop() {
	construction_timer->stop();
}

void BenchmarkStats::lookup_start() {
	lookup_timer = new DebugTimer();
}
void BenchmarkStats::lookup_stop() {
	lookup_timer->stop();
}

void BenchmarkStats::set_number_or_tries_or_tables(
		int number_or_tries_or_tables) {
	this->number_or_tries_or_tables = number_or_tries_or_tables;
}

void BenchmarkStats::dump(std::function<void(std::ostream &)> json_extra,
		std::function<void(std::ostream &)> text_extra) {
	auto lookup_speed = (LOOKUP_CNT / double(lookup_timer->us()));
	if (dump_as_json) {
		out << "{ \"lookup_speed\":" << lookup_speed << "," << std::endl;
		out << "\"construction_time\":" << uint64_t(construction_timer->us())
				<< "," << std::endl;
		out << "\"real_rule_cnt\":" << real_rule_cnt << "," << std::endl;
		out << "\"number_or_tries_or_tables\":" << number_or_tries_or_tables;
		json_extra(out);
		out << "}";
	} else {
		out << "[INFO] lookup speed:" << lookup_speed << "MPkts/s" << std::endl;
		out << "[INFO] construction time:" << uint64_t(construction_timer->us())
				<< "us" << std::endl;
		out << "[INFO] real_rule_cnt:" << real_rule_cnt << std::endl;
		out << "[INFO] number_or_tries_or_tables:";
		if (number_or_tries_or_tables >= 0)
			out << number_or_tries_or_tables << std::endl;
		else
			out << "unspecified" << std::endl;

		text_extra(out);
	}
}

BenchmarkStats::~BenchmarkStats() {
	delete construction_timer;
	delete lookup_timer;
}

}
