
#pragma once

#include "app/sat/data/clause.hpp"
#include "app/sat/sharing/filter/generic_clause_filter.hpp"
#include "app/sat/sharing/store/generic_clause_store.hpp"

// This filter does nothing but attempt to add each clause to the clause store directly.
// No clauses are filtered out and only ADMITTED and DROPPED are possible results of
// tryRegisterAndInsert.
class NoopClauseFilter : public GenericClauseFilter {

public:
	NoopClauseFilter(GenericClauseStore& clauseStore) : 
		GenericClauseFilter(clauseStore) {}
	virtual ~NoopClauseFilter() {}

	ExportResult tryRegisterAndInsert(ProducedClauseCandidate&& pcc, GenericClauseStore* storeOrNullptr = nullptr) override {
		Mallob::Clause c(pcc.begin, pcc.size, pcc.lbd);
		auto clauseStore = storeOrNullptr ? storeOrNullptr : &_clause_store;
		if (clauseStore->addClause(c)) {
			return ADMITTED;
		} else return DROPPED;
	}

    cls_producers_bitset confirmSharingAndGetProducers(Mallob::Clause& c, int epoch) override {
		cls_producers_bitset result = 0;
		return result;
	}
    
	bool admitSharing(Mallob::Clause& c, int epoch) override {
		return true;
	}
    
	size_t size(int clauseLength) const override {
		return 0;
	}
};
