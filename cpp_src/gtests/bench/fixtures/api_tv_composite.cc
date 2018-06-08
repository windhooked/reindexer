#include "api_tv_composite.h"
#include "core/keyvalue/keyvalue.h"
#include "core/query/query.h"
#include "core/query/queryresults.h"
#include "helpers.h"

using benchmark::RegisterBenchmark;
using std::placeholders::_1;

using reindexer::Query;
using reindexer::QueryResults;
using reindexer::KeyValue;

reindexer::Error ApiTvComposite::Initialize() {
	auto err = BaseFixture::Initialize();
	if (!err.ok()) return err;

	names_ = {"ox",   "ant",  "ape",  "asp",  "bat",  "bee",  "boa",  "bug",  "cat",  "cod",  "cow",  "cub",  "doe",  "dog",
			  "eel",  "eft",  "elf",  "elk",  "emu",  "ewe",  "fly",  "fox",  "gar",  "gnu",  "hen",  "hog",  "imp",  "jay",
			  "kid",  "kit",  "koi",  "lab",  "man",  "owl",  "pig",  "pug",  "pup",  "ram",  "rat",  "ray",  "yak",  "bass",
			  "bear", "bird", "boar", "buck", "bull", "calf", "chow", "clam", "colt", "crab", "crow", "dane", "deer", "dodo",
			  "dory", "dove", "drum", "duck", "fawn", "fish", "flea", "foal", "fowl", "frog", "gnat", "goat", "grub", "gull",
			  "hare", "hawk", "ibex", "joey", "kite", "kiwi", "lamb", "lark", "lion", "loon", "lynx", "mako", "mink", "mite",
			  "mole", "moth", "mule", "mutt", "newt", "orca", "oryx", "pika", "pony", "puma", "seal", "shad", "slug", "sole",
			  "stag", "stud", "swan", "tahr", "teal", "tick", "toad", "tuna", "wasp", "wolf", "worm", "wren", "yeti"};

	locations_ = {"mos", "ct", "dv", "sth", "vlg", "sib", "ural"};

	return 0;
}

reindexer::Item ApiTvComposite::MakeItem() {
	auto item = db_->NewItem(nsdef_.name);
	item.Unsafe(true);

	auto startTime = random<int64_t>(0, 50000);

	item["id"] = id_seq_->Next();
	item["name"] = names_.at(random<size_t>(0, names_.size() - 1));
	item["year"] = random<int>(2000, 2049);
	item["rate"] = random<double>(0, 10);
	item["age"] = random<int>(1, 5);
	item["location"] = locations_.at(random<size_t>(0, locations_.size() - 1));
	item["start_time"] = startTime;
	item["end_time"] = startTime + random<int64_t>(1, 5) * 1000;

	item.Unsafe(false);
	item["genre"] = std::to_string(random<int>(0, 49));
	item["sub_id"] = id_seq_->As<string>();

	return item;
}

void ApiTvComposite::RegisterAllCases() {
	// Skip BaseFixture::Update

	Register("Insert", &ApiTvComposite::Insert, this)->Iterations(id_seq_->Count());
	Register("WarmUpIndexes", &ApiTvComposite::WarmUpIndexes, this)->Iterations(1);
	Register("GetByCompositePK", &ApiTvComposite::GetByCompositePK, this);

	// Part I
	Register("RangeTreeInt", &ApiTvComposite::RangeTreeInt, this);
	Register("RangeTreeStrCollateNumeric", &ApiTvComposite::RangeTreeStrCollateNumeric, this);
	Register("RangeTreeDouble", &ApiTvComposite::RangeTreeDouble, this);
	Register("RangeTreeCompositeIntInt", &ApiTvComposite::RangeTreeCompositeIntInt, this);
	Register("RangeTreeCompositeIntStr", &ApiTvComposite::RangeTreeCompositeIntStr, this);

	// Part II
	Register("RangeHashInt", &ApiTvComposite::RangeHashInt, this);
	Register("RangeHashStringCollateASCII", &ApiTvComposite::RangeHashStringCollateASCII, this);
	Register("RangeHashStringCollateUTF8", &ApiTvComposite::RangeHashStringCollateUTF8, this);

	//	The test cases below fall with an error (It's can be fixed in the future)
	Register("RangeHashCompositeIntInt", &ApiTvComposite::RangeHashCompositeIntInt, this);
	Register("RangeHashCompositeIntStr", &ApiTvComposite::RangeHashCompositeIntStr, this);

	// Part III
	Register("RangeTreeIntSortByHashInt", &ApiTvComposite::RangeTreeIntSortByHashInt, this);
	Register("RangeTreeIntSortByTreeInt", &ApiTvComposite::RangeTreeIntSortByTreeInt, this);
	Register("RangeTreeStrSortByHashInt", &ApiTvComposite::RangeTreeStrSortByHashInt, this);
	Register("RangeTreeStrSortByTreeInt", &ApiTvComposite::RangeTreeStrSortByTreeInt, this);
	Register("RangeTreeDoubleSortByTreeInt", &ApiTvComposite::RangeTreeDoubleSortByTreeInt, this);
	Register("RangeTreeDoubleSortByHashInt", &ApiTvComposite::RangeTreeDoubleSortByHashInt, this);
	Register("RangeTreeStrSortByHashStrCollateASCII", &ApiTvComposite::RangeTreeStrSortByHashStrCollateASCII, this);
	Register("RangeTreeStrSortByHashStrCollateUTF8", &ApiTvComposite::RangeTreeStrSortByHashStrCollateUTF8, this);

	// Part IV
	Register("SortByHashInt", &ApiTvComposite::SortByHashInt, this);
	Register("SortByHashStrCollateASCII", &ApiTvComposite::SortByHashStrCollateASCII, this);
	Register("SortByHashStrCollateUTF8", &ApiTvComposite::SortByHashStrCollateUTF8, this);
	//	The test cases below fall with an error ((It's can be fixed in the future))
	Register("SortByHashCompositeIntInt", &ApiTvComposite::SortByHashCompositeIntInt, this);
	Register("SortByHashCompositeIntStr", &ApiTvComposite::SortByHashCompositeIntStr, this);

	Register("SortByTreeCompositeIntInt", &ApiTvComposite::SortByTreeCompositeIntInt, this);
	Register("SortByTreeCompositeIntStrCollateUTF8", &ApiTvComposite::SortByTreeCompositeIntStrCollateUTF8, this);
}

void ApiTvComposite::Insert(State& state) { BaseFixture::Insert(state); }

void ApiTvComposite::WarmUpIndexes(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		{
			Query q(nsdef_.name);
			q.Where("sub_id", CondLe, std::to_string(random<int>(id_seq_->Start(), id_seq_->End())));

			QueryResults qres;
			auto err = db_->Select(q, qres);
			if (!err.ok()) state.SkipWithError(err.what().c_str());
		}

		{
			Query q(nsdef_.name);
			auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);
			auto leftStartTime = random<int>(0, 24999);
			auto rightStartTime = random<int>(0, 50000);
			q.WhereComposite("id+start_time", CondRange,
							 {{KeyValue(idRange.first), KeyValue(leftStartTime)}, {KeyValue(idRange.second), KeyValue(rightStartTime)}})
				.Sort("start_time", false)
				.Limit(20);
		}

		{
			Query q(nsdef_.name);
			q.Where("location", CondSet, {"mos", "vlg", "ural"}).Sort("age", false);

			QueryResults qres;
			auto err = db_->Select(q, qres);
			if (!err.ok()) state.SkipWithError(err.what().c_str());
		}
	}
}

void ApiTvComposite::GetByCompositePK(State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		auto randId = random<int>(id_seq_->Start(), id_seq_->End());
		auto randSubId = std::to_string(randId);
		Query q(nsdef_.name);
		q.WhereComposite("id+sub_id", CondEq, {{KeyValue(randId), KeyValue(randSubId)}});

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());

		if (qres.empty()) state.SkipWithError("Results does not contain any value");
	}
}

void ApiTvComposite::RangeTreeInt(State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		auto leftYear = random<int>(2000, 2024);
		auto rightYear = random<int>(2025, 2049);

		Query q(nsdef_.name);
		q.Where("year", CondRange, {leftYear, rightYear}).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeStrCollateNumeric(State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		{
			Query q(nsdef_.name);
			auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);
			q.Where("sub_id", CondRange, {std::to_string(idRange.first), std::to_string(idRange.second)}).Limit(1);

			QueryResults qres;
			auto err = db_->Select(q, qres);
			if (!err.ok()) state.SkipWithError(err.what().c_str());
		}
	}
}

void ApiTvComposite::RangeTreeDouble(State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);
		auto firstRate = random<double>(1, 5);
		auto secondRate = random<double>(5, 10);

		q.Where("rate", CondRange, {firstRate, secondRate}).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());

		if (qres.empty()) state.SkipWithError("empty qres");
	}
}

void ApiTvComposite::RangeTreeCompositeIntInt(State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);
		auto leftYear = random<int>(2000, 2024);
		auto rightYear = random<int>(2025, 2049);

		q.WhereComposite("id+year", CondRange,
						 {{KeyValue(idRange.first), KeyValue(leftYear)}, {KeyValue(idRange.second), KeyValue(rightYear)}})
			.Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeCompositeIntStr(State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);
		auto randLeftStr = names_.at(random<size_t>(0, names_.size() - 1));
		auto randRightStr = names_.at(random<size_t>(0, names_.size() - 1));

		q.WhereComposite("id+name", CondRange,
						 {{KeyValue(idRange.first), KeyValue(randLeftStr)}, {KeyValue(idRange.second), KeyValue(randRightStr)}})
			.Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeHashInt(State& state) {
	AllocsTracker AllocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);
		q.Where("id", CondRange, {idRange.first, idRange.second}).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeHashStringCollateASCII(State& state) {
	AllocsTracker AllocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);
		auto leftLoc = locations_.at(random<size_t>(0, locations_.size() - 1));
		auto rightLoc = locations_.at(random<size_t>(0, locations_.size() - 1));

		q.Where("location", CondRange, {leftLoc, rightLoc}).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeHashStringCollateUTF8(State& state) {
	AllocsTracker AllocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto leftName = names_.at(random<size_t>(0, names_.size() - 1));
		auto rightName = names_.at(random<size_t>(0, names_.size() - 1));

		q.Where("name", CondRange, {leftName, rightName}).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeHashCompositeIntInt(State& state) {
	AllocsTracker AllocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);
		auto leftStartTime = random<int64_t>(0, 24999);
		auto rightStartTime = random<int64_t>(25000, 50000);

		q.WhereComposite("id+start_time", CondRange,
						 {{KeyValue(idRange.first), KeyValue(leftStartTime)}, {KeyValue(idRange.second), KeyValue(rightStartTime)}})
			.Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeHashCompositeIntStr(benchmark::State& state) {
	AllocsTracker AllocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);
		auto leftGenre = std::to_string(random<int>(0, 24));
		auto rightGenre = std::to_string(random<int>(25, 49));

		q.WhereComposite("id+genre", CondRange,
						 {{KeyValue(idRange.first), KeyValue(leftGenre)}, {KeyValue(idRange.second), KeyValue(rightGenre)}})
			.Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeIntSortByHashInt(State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);

		q.Where("id", CondRange, {idRange.first, idRange.second}).Sort("age", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeIntSortByTreeInt(State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);

		q.Where("id", CondRange, {idRange.first, idRange.second}).Sort("year", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeStrSortByHashInt(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);

		q.Where("id", CondRange, {std::to_string(idRange.first), std::to_string(idRange.second)}).Sort("age", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeStrSortByTreeInt(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);

		q.Where("id", CondRange, {std::to_string(idRange.first), std::to_string(idRange.second)}).Sort("year", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeDoubleSortByTreeInt(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto leftRate = random<double>(0.0, 4.99);
		auto rightRate = random<double>(5.0, 10.0);

		q.Where("rate", CondRange, {leftRate, rightRate}).Sort("year", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeDoubleSortByHashInt(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto leftRate = random<double>(0.0, 4.99);
		auto rightRate = random<double>(5.0, 10.0);

		q.Where("rate", CondRange, {leftRate, rightRate}).Sort("age", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeStrSortByHashStrCollateASCII(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);

		q.Where("id", CondRange, {std::to_string(idRange.first), std::to_string(idRange.second)}).Sort("location", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::RangeTreeStrSortByHashStrCollateUTF8(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		auto idRange = id_seq_->GetRandomIdRange(id_seq_->Count() * 0.02);

		q.Where("id", CondRange, {std::to_string(idRange.first), std::to_string(idRange.second)}).Sort("name", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::SortByHashInt(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		q.Sort("id", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::SortByHashStrCollateASCII(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		q.Sort("location", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::SortByHashStrCollateUTF8(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		q.Sort("name", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::SortByHashCompositeIntInt(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		q.Sort("id+start_time", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::SortByHashCompositeIntStr(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		q.Sort("id+genre", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::SortByTreeCompositeIntInt(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		q.Sort("id+year", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}

void ApiTvComposite::SortByTreeCompositeIntStrCollateUTF8(benchmark::State& state) {
	AllocsTracker allocsTracker(state);
	for (auto _ : state) {
		Query q(nsdef_.name);

		q.Sort("id+name", false).Limit(20);

		QueryResults qres;
		auto err = db_->Select(q, qres);
		if (!err.ok()) state.SkipWithError(err.what().c_str());
	}
}