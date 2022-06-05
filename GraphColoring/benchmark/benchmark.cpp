#include "benchmark.h"

std::vector<Benchmark*> Benchmark::_instances = std::vector<Benchmark*>(0);

Benchmark::Benchmark() {
	this->timeMap = new std::unordered_map<int, long long int>();
	this->prev = {};
	this->now = {};

	this->clear(0);	// File load
	this->clear(1);	// Preprocess
	this->clear(2);	// Process
	this->clear(3);	// Postprocess
}

double Benchmark::countToMicroseconds(long long int count) {
	return (double)count;
}

double Benchmark::countToMilliseconds(long long int count) {
	return Benchmark::countToMicroseconds(count) / 1000;
}

double Benchmark::countToSeconds(long long int count) {
	return Benchmark::countToMilliseconds(count) / 1000;
}

Benchmark* Benchmark::getInstance(int z) {
	if (Benchmark::_instances.size() <= z) {
		int old_size = Benchmark::_instances.size();
		int new_size = z + 1;
		Benchmark::_instances.resize(new_size);
		std::fill(Benchmark::_instances.begin() + old_size, Benchmark::_instances.end(), new Benchmark());
	}

	return Benchmark::_instances[z];
}

double Benchmark::getAvgOfFlag(int flagId) {
	long long int sum = 0;

	for (auto& inst : Benchmark::_instances) {
		sum += inst->timeMap->at(flagId);
	}

	return Benchmark::countToSeconds(sum) / Benchmark::_instances.size();
}

double Benchmark::getAvgOfTotal() {
	long long int sum = 0;

	for (auto& inst : Benchmark::_instances) {
		for (auto& it : *inst->timeMap) {
			sum += it.second;
		}
	}

	return Benchmark::countToSeconds(sum) / Benchmark::_instances.size();
}

double Benchmark::getEffectiveAvg() {
	double avg = Benchmark::getAvgOfFlag(1)
		+ Benchmark::getAvgOfFlag(2)
		+ Benchmark::getAvgOfFlag(3);
	long long int load_time = Benchmark::_instances[0]->timeMap->at(0);

	return avg + Benchmark::countToSeconds(load_time);
}

void Benchmark::clear(const int flagId) {
	this->timeMap->emplace(flagId, 0.0);
}

void Benchmark::sampleTime() {
	this->prev = this->now;
	now = std::chrono::steady_clock::now();
}

void Benchmark::sampleTimeToFlag(const int flagId) {
	this->sampleTime();
	this->timeMap->at(flagId) += std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count();
}

double Benchmark::getTimeOfFlag(const int flagId) {
	return Benchmark::countToSeconds(this->timeMap->at(flagId));
}

double Benchmark::getTotalTime() {
	long long int sum = 0;
	for (auto& it : *this->timeMap) {
		sum += it.second;
	}
	return  Benchmark::countToSeconds(sum);
}