// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <map>
#include "tables.h"
#include "input.h"
#include "dict.h"

enum output_dialect { SWIPL, XSB, SOUFFLE, CSV, NONE };

struct prog_data {
	strs_t strs;
//	std::unordered_map<int_t, term> strtrees;
//	std::vector<term> out;
//	matpairs r;
//	matrix goals, tgoals;
	bool bwd = false;
};

class driver {
	dict_t dict;
	std::set<int_t> builtin_rels;//, builtin_symbdds;

	int_t nums = 0, chars = 0, syms = 0;
//	bool mult = false;

	lexeme get_var_lexeme(int_t i);
	lexeme get_num_lexeme(int_t i);
	lexeme get_char_lexeme(wchar_t i);

	std::wstring directive_load(const directive& d);
	void directives_load(raw_prog& p, lexeme& trel);
	void transform(raw_progs& rp, size_t n, const strs_t& strtrees);
	void transform_len(raw_term& r, const strs_t& s);
	raw_prog transform_bwd(raw_prog& p);
	void transform_proofs(raw_prog& r, const lexeme& rel);
	void transform_string(const std::wstring&, raw_prog&, int_t);
	void transform_grammar(raw_prog& r, lexeme rel, size_t len);
	raw_term from_grammar_elem(const elem& v, int_t v1, int_t v2);
	raw_term from_grammar_elem_nt(const lexeme& r, const elem& c,
		int_t v1, int_t v2);
	raw_term from_grammar_elem_builtin(const lexeme& r,const std::wstring&b,
		int_t v);
	raw_term prepend_arg(raw_term t, lexeme s);
//	void get_trees(std::wostream& os, const term& root,
//		const std::map<term, std::vector<term>>&, std::set<term>& done);
//	std::wstring get_trees(const term& roots,const db_t& t,size_t bits);
	void progs_read(wstr s);
	void prog_run(raw_progs& rp, size_t n, strs_t& strtrees);
	driver(int argc, char** argv, raw_progs, output_dialect dialect,
		bool print_transformed);
	size_t load_stdin();
	bool pfp();
	std::wstring std_input;
	int argc;
	char** argv;
	prog_data pd;
	std::set<int_t> transformed_strings;
	tables tbl;
	output_dialect dialect;
	bool print_transformed;
	void output_pl(const raw_prog& p) const;
public:
	bool result = true;
	driver(int argc, char** argv, FILE *f, output_dialect dialect,
			bool print_transformed = false);
	driver(int argc, char** argv, std::wstring, output_dialect dialect,
			bool print_transformed = false);
	driver(int argc, char** argv, char *s, output_dialect dialect,
			bool print_transformed = false);

//	std::wostream& printbdd(std::wostream& os, spbdd t, size_t bits,
//		const prefix&) const;
//	std::wostream& printbdd_one(std::wostream& os, spbdd t, size_t bits,
//		const prefix&) const;
	std::wostream& print_prolog(std::wostream& os, const raw_prog& p,
		const output_dialect dialect) const;
	std::wostream& print_xsb(std::wostream& os, const raw_prog& rp) const {
		return print_prolog(os, rp, XSB); }
	std::wostream& print_swipl(std::wostream& os, const raw_prog& rp) const{
		return print_prolog(os, rp, SWIPL); }
	std::wostream& print_souffle(std::wostream& os, const raw_prog& rp)
		const;
//	std::wostream& print_term_xsb(std::wostream& os, const term& t) const;
//	std::wostream& print_term_souffle(std::wostream& os, const term& t)
//		const;
//	std::wostream& print_term_csv(std::wostream& os, const term& t) const;
//	void save_csv(lp *p) const;
//	void save_csv(const db_t& db, size_t bits) const;
};

#ifdef DEBUG
extern driver* drv;
//std::wostream& printdb(std::wostream& os, lp *p);
std::wostream& printbdd(std::wostream& os, spbdd t, size_t bits, ints ar,
	int_t rel);
std::wostream& printbdd_one(std::wostream& os, spbdd t, size_t bits, ints ar,
	int_t rel);
//std::wostream& printbdd(std::wostream& os, size_t t, size_t bits, ints ar,
//	int_t rel);
#endif
std::wstring _unquote(std::wstring str);
