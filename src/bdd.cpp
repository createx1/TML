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
#include <cassert>
#include <algorithm>
#include "bdd.h"
using namespace std;

#define MEMO

vector<unordered_map<bdd_key, int_t>> bdd::Mp, bdd::Mn;
vector<bdd> bdd::V;
unordered_map<ite_memo, int_t> bdd::C;
unordered_map<bdds, int_t> bdd::AM;
unordered_set<int_t> bdd::S;
unordered_map<int_t, weak_ptr<bdd_handle>> bdd_handle::M;
spbdd_handle bdd_handle::T, bdd_handle::F;
map<bools, unordered_map<int_t, int_t>> memos_ex;
map<uints, unordered_map<int_t, int_t>> memos_perm;
map<pair<uints, bools>, unordered_map<int_t, int_t>> memos_perm_ex;

auto am_cmp = [](int_t x, int_t y) {
	bool s = x < y;
	x = abs(x), y = abs(y);
	return x < y ? true : x == y ? s : false;
};

void bdd::init() {
	S.insert(0), S.insert(1), V.emplace_back(0, 0, 0), // dummy
	V.emplace_back(0, 1, 1), Mp.resize(1),
	Mp[0].emplace(bdd_key(hash_pair(0, 0), 0, 0), 0),
	Mp[0].emplace(bdd_key(hash_pair(1, 1), 1, 1), 1),
	bdd_handle::T = bdd_handle::get(T), bdd_handle::F = bdd_handle::get(F);
}

int_t bdd::from_bit(uint_t b, bool v) {
	return v ? add(b + 1, T, F) : add(b + 1, F, T);
}

int_t bdd::bdd_and1(int_t x, int_t y) {
	DBG(assert(x && y);)
	if (x == F || y == F) return F;
	if (x == T || x == y) return y;
	if (y == T) return x;
	if (x == -y) return F;
	if (x > y) swap(x, y);
#ifdef MEMO
	if (C.size() >= 1e+7) {
		const bdd bx = get(x), by = get(y);
		if (bx.v < by.v)
			return add(bx.v, bdd_and1(bx.h, y), bdd_and1(bx.l, y));
		else if (bx.v > by.v)
			return add(by.v, bdd_and1(x, by.h), bdd_and1(x, by.l));
		return add(bx.v, bdd_and1(bx.h, by.h), bdd_and1(bx.l, by.l));
	}
	ite_memo m = { x, y, F };
	auto it = C.find(m);
	if (it != C.end()) return it->second;
#endif
	const bdd bx = get(x), by = get(y);
	int_t r;
	if (bx.v < by.v) r = add(bx.v, bdd_and1(bx.h, y), bdd_and1(bx.l, y));
	else if (bx.v > by.v) r = add(by.v, bdd_and1(x, by.h), bdd_and1(x, by.l));
	else r = add(bx.v, bdd_and1(bx.h, by.h), bdd_and1(bx.l, by.l));
#ifdef MEMO
	C.emplace(m, r);
#endif		
	return r;
}

int_t bdd::bdd_and(int_t x, int_t y) {
//	set<int_t> sx, sy;
//	bdd_sz(x, sx), bdd_sz(y, sy);
//	wcerr << "bdd_and sz1: " << sx.size() << " sz2: " << sy.size() <<endl;
//	sx.clear(), sy.clear();
	int_t r = bdd_and1(x, y);
//	bdd_sz(r, sx);
//	wcerr<<"done "<<sx.size()<<endl;
	return r;
}
int_t bdd::bdd_ite_var(uint_t x, int_t y, int_t z) {
	return bdd_ite(from_bit(x, true), y, z);
}

int_t bdd::bdd_ite(int_t x, int_t y, int_t z) {
	DBG(assert(x && y && z);)
	if (x < 0) return bdd_ite(-x, z, y);
	if (x == F) return z;
	if (x == T || y == z) return y;
	if (x == -y || x == z) return F;
	if (y == T) return bdd_or(x, z);
	if (y == F) return bdd_and(-x, z);
	if (z == F) return bdd_and(x, y);
	if (z == T) return bdd_or(-x, y);
	auto it = C.find({x, y, z});
	if (it != C.end()) return it->second;
	int_t r;
	const bdd bx = get(x), by = get(y), bz = get(z);
	const int_t s = min(bx.v, min(by.v, bz.v));
	if (bx.v == by.v && by.v == bz.v)
		r =	add(bx.v, bdd_ite(bx.h, by.h, bz.h),
				bdd_ite(bx.l, by.l, bz.l));
	else if (s == bx.v && s == by.v)
		r =	add(bx.v, bdd_ite(bx.h, by.h, z),
				bdd_ite(bx.l, by.l, z));
	else if (s == by.v && s == bz.v)
		r =	add(by.v, bdd_ite(x, by.h, bz.h),
				bdd_ite(x, by.l, bz.l));
	else if (s == bx.v && s == bz.v)
		r =	add(bx.v, bdd_ite(bx.h, y, bz.h),
				bdd_ite(bx.l, y, bz.l));
	else if (s == bx.v)
		r =	add(bx.v, bdd_ite(bx.h, y, z), bdd_ite(bx.l, y, z));
	else if (s == by.v)
		r =	add(by.v, bdd_ite(x, by.h, z), bdd_ite(x, by.l, z));
	else	r =	add(bz.v, bdd_ite(x, y, bz.h), bdd_ite(x, y, bz.l));
	return C.emplace(ite_memo{x, y, z}, r), r;
}

void am_sort(bdds& b) {
	sort(b.begin(), b.end(), am_cmp);
	for (size_t n = 0; n < b.size();)
		if (b[n] == T) b.erase(b.begin() + n);
		else if (b[n] == F) { b = {F}; return; }
		else if (!n) { ++n; continue; }
		else if (b[n] == b[n-1]) b.erase(b.begin() + n);
		else if (b[n] == -b[n-1]) { b = {F}; return; }
		else ++n;
}

size_t bdd::bdd_and_many_iter(bdds v, bdds& h, bdds& l, int_t &res, size_t &m) {
	size_t i;
	bool b, flag;
/*	if (v.empty()) return res = T, 1;
	if (v.size() == 2) return res = bdd_and(v[0], v[1]), 1;
	for (size_t n = 0; n < v.size();)
		if (!leaf(v[n])) ++n;
		else if (!trueleaf(v[n])) return res = F, 1;
		else if (v.erase(v.begin()+n), v.size()==1) return res=v[0], 1;
		else if (v.size() == 2) return res = bdd_and(v[0], v[1]), 1;
		else if (v.empty()) return res = T, 1;
		else ++n;*/
	m = var(v[0]), b = flag = false;
	for (i = 1; i != v.size(); ++i)
		if (!leaf(v[i])) {
			b |= var(v[i]) != m;
			if (var(v[i]) < m) m = var(v[i]);
		} else if (!trueleaf(v[i])) return res = F, 1;
	h.reserve(v.size()), l.reserve(v.size());
	for (i = 0; i != v.size(); ++i)
		if (/*b && */var(v[i]) != m) h.push_back(v[i]);
		else if (!leaf(hi(v[i]))) h.push_back(hi(v[i]));
		else if (!trueleaf(hi(v[i]))) { flag = true; break; }
	for (i = 0; i != v.size(); ++i)
		if (/*b && */var(v[i]) != m) l.push_back(v[i]);
		else if (!leaf(lo(v[i]))) l.push_back(lo(v[i]));
		else if (!trueleaf(lo(v[i]))) return flag ? res = F, 1 : 2;
	am_sort(h), am_sort(l);
	if (!flag) { if (h.size() && h[0] == F) flag = true; }
	if (l.size() && l[0] == F) return flag ? 3 : 2;
	if (flag) return 3;
	bdds x;
	set_intersection(h.begin(),h.end(),l.begin(),l.end(),back_inserter(x),
			am_cmp);
	am_sort(x);
	if (x.size() > 1) {
		for (size_t n = 0; n < h.size();)
			if (hasbc(x, h[n], am_cmp)) h.erase(h.begin() + n);
			else ++n;
		for (size_t n = 0; n < l.size();)
			if (hasbc(x, l[n], am_cmp)) l.erase(l.begin() + n);
			else ++n;
		h.shrink_to_fit(), l.shrink_to_fit(), x.shrink_to_fit();
		int_t r = bdd_and_many(move(x));
		if (r == F) return res = F, 1;
		if (r != T) {
			if (!hasbc(h, r, am_cmp)) h.push_back(r), am_sort(h);
			if (!hasbc(l, r, am_cmp)) l.push_back(r), am_sort(l);
		}
	}
	return 0;
}

bool subset(const bdds& small, const bdds& big) {
	if (	big.size() < small.size() ||
		am_cmp(abs(big[big.size()-1]), abs(small[0])) ||
		am_cmp(abs(small[small.size()-1]), abs(big[0])))
		return false;
	for (const int_t& t : small) if (!hasbc(big, t, am_cmp)) return false;
	return true;
}

size_t simps = 0;

bool bdd::am_simplify(bdds& v, const unordered_map<bdds, int_t>& memo) {
	for (auto x : memo)
		if (subset(x.first, v)) {
			if (x.second == F) return v={F}, true;
			for (size_t n = 0; n < v.size();)
				if (!hasbc(x.first, v[n], am_cmp)) ++n;
				else v.erase(v.begin() + n);
			if (!hasbc(v, x.second, am_cmp)) v.push_back(x.second);
			return true;
		}
	return false;
}

int_t bdd::bdd_and_many(bdds v) {
#ifdef MEMO	
	static unordered_map<ite_memo, int_t>::const_iterator jt;
	for (size_t n = 0; n < v.size(); ++n)
		for (size_t k = 0; k < n; ++k) {
			int_t x, y;
			if (v[n] < v[k]) x = v[n], y = v[k];
			else x = v[k], y = v[n];
			if ((jt = C.find({x, y, F})) != C.end()) {
				v.erase(v.begin()+k), v.erase(v.begin()+n-1),
				v.push_back(jt->second), n = k = 0;
				break;
			}
		}
#endif
	if (v.empty()) return T;
	if (v.size() == 1) return v[0];
	simps = 0;
	static bdds v1;
	do {
		if (v1 = v, am_simplify(v), ++simps, v.size() == 1) return v[0];
	} while (v1 != v);
	if (v.empty()) return T;
	if (v.size() == 1) return v[0];
	auto it = AM.find(v);
	if (it != AM.end()) return it->second;
//	for (int_t x : v) wcout << x << ' ';
//	wcout << simps << endl;
	if (v.size() == 2)
		return AM.emplace(v, bdd_and(v[0], v[1])).first->second;
	int_t res = F, h, l;
	size_t m = 0;
	bdds vh, vl;
	switch (bdd_and_many_iter(v, vh, vl, res, m)) {
		case 0: l = bdd_and_many(move(vl)),
			h = bdd_and_many(move(vh));
			break;
		case 1: return AM.emplace(v, res), res;
		case 2: h = bdd_and_many(move(vh)), l = F; break;
		case 3: h = F, l = bdd_and_many(move(vl)); break;
		default: throw 0;
	}
	return AM.emplace(v, bdd::add(m, h, l)).first->second;
}

int_t bdd::bdd_and_ex(int_t x, int_t y, const bools& ex) {
	assert(0);
	return x;
}

size_t bdd::bdd_and_many_ex_iter(bdds v, bdds& h, bdds& l, int_t &res,
	size_t &m, const bools& ex, unordered_map<bdds, int_t>& memo) {
	size_t i;
	bool flag = false;
	if (v.empty()) return res = T, 1;
	if (v.size() == 2) return res = bdd_and_ex(v[0], v[1], ex), 1;
	for (size_t n = 0; n < v.size();)
		if (!leaf(v[n])) ++n;
		else if (!trueleaf(v[n])) return res = F, 1;
		else if (v.erase(v.begin()+n), v.size()==1) return res=v[0], 1;
		else if (v.size()==2) return res=bdd_and_ex(v[0], v[1], ex), 1;
		else if (v.empty()) return res = T, 1;
		else ++n;
	m = var(v[0]);
	for (i = 1; i != v.size(); ++i)
		if (!leaf(v[i])) {
			if (var(v[i]) < m) m = var(v[i]);
		} else if (!trueleaf(v[i])) return res = F, 1;
	if (ex[m]) {
		for (i = 0; i != v.size(); ++i)
			if (var(v[i]) == m)
				v[i] = bdd_or(hi(v[i]), lo(v[i]));
		am_sort(v);
		return bdd_and_many_ex_iter(move(v), h, l, res, m, ex, memo);
	}
	h.reserve(v.size()), l.reserve(v.size());
	for (i = 0; i != v.size(); ++i)
		if (var(v[i]) != m) h.push_back(v[i]);
		else if (!leaf(hi(v[i]))) h.push_back(hi(v[i]));
		else if (!trueleaf(hi(v[i]))) { flag = true; break; }
	for (i = 0; i != v.size(); ++i)
		if (var(v[i]) != m) l.push_back(v[i]);
		else if (!leaf(lo(v[i]))) l.push_back(lo(v[i]));
		else if (!trueleaf(lo(v[i]))) return flag ? res = F, 1 : 2;
	am_sort(h), am_sort(l);
	if (!flag) { if (h.size() && h[0] == F) flag = true; }
	if (l.size() && l[0] == F) return flag ? 3 : 2;
	if (flag) return 3;
	bdds x;
	set_intersection(h.begin(),h.end(),l.begin(),l.end(),back_inserter(x),
			am_cmp);
	if (x.size() > 1) {
		am_sort(x);
		for (size_t n = 0; n < h.size();)
			if (hasbc(x, h[n], am_cmp)) h.erase(h.begin() + n);
			else ++n;
		for (size_t n = 0; n < l.size();)
			if (hasbc(x, l[n], am_cmp)) l.erase(l.begin() + n);
			else ++n;
		h.shrink_to_fit(), l.shrink_to_fit(), x.shrink_to_fit();
		int_t r = bdd_and_many_ex(move(x), ex, memo);
		if (r == F) return res = F, 1;
		h.push_back(r), l.push_back(r);
	}
	return 0;
}

int_t bdd::bdd_and_many_ex(bdds v, const bools& ex,
	unordered_map<bdds, int_t>& memo) {
/*#ifdef MEMO	
	static unordered_map<ite_memo, int_t>::const_iterator jt;
	for (size_t n = 0; n < v.size(); ++n)
		for (size_t k = 0; k < n; ++k) {
			int_t x, y;
			if (v[n] < v[k]) x = v[n], y = v[k];
			else x = v[k], y = v[n];
			if ((jt = C.find({x, y, F})) != C.end()) {
				v.erase(v.begin()+k), v.erase(v.begin()+n-1),
				v.push_back(jt->second), n = k = 0;
				break;
			}
		}
#endif*/
	if (v.empty()) return T;
	if (v.size() == 1) return bdd_ex(v[0], ex);
	simps = 0;
	static bdds v1;
	do {
		if (v1 = v, am_simplify(v, memo), ++simps, v.size() == 1)
			return v[0];
	} while (v1 != v);
	if (v.size() == 1) return bdd_ex(v[0], ex);
	auto it = memo.find(v);
	if (it != memo.end()) return it->second;
	if (v.size() == 2)
		return memo.emplace(v, bdd_and_ex(v[0],v[1],ex)).first->second;
	int_t res = F, h, l;
	size_t m = 0;
	bdds vh, vl;
	switch (bdd_and_many_ex_iter(v, vh, vl, res, m, ex, memo)) {
		case 0: l = bdd_and_many_ex(move(vl), ex, memo),
			h = bdd_and_many_ex(move(vh), ex, memo);
			break;
		case 1: return memo.emplace(v, res), res;
		case 2: h = bdd_and_many_ex(move(vh), ex, memo), l = F; break;
		case 3: h = F, l = bdd_and_many_ex(move(vl), ex, memo); break;
		default: throw 0;
	}
	return memo.emplace(v, bdd::add(m, h, l)).first->second;
}

void bdd::mark_all(int_t i) {
	DBG(assert((size_t)abs(i) < V.size());)
	if ((i = abs(i)) >= 2 && !has(S, i))
		mark_all(hi(i)), mark_all(lo(i)), S.insert(i);
}

void bdd::gc() {
	S.clear();
	for (auto x : bdd_handle::M) mark_all(x.first);
/*	for (auto x : AM) {
		mark_all(x.second);
		for (auto y : x.first) mark_all(y);
	}*/
//	if (V.size() < S.size() << 3) return;
	const size_t pvars = Mp.size(), nvars = Mn.size();
	Mp.clear(), Mn.clear(), S.insert(0), S.insert(1);
//	if (S.size() >= 1e+6) { wcerr << "out of memory" << endl; exit(1); }
	vector<int_t> p(V.size(), 0);
	vector<bdd> v1;
	v1.reserve(S.size());
	for (size_t n = 0; n < V.size(); ++n)
		if (has(S, n)) p[n] = v1.size(), v1.emplace_back(move(V[n]));
	wcerr << "S: " << S.size() << " V: "<< V.size() << " AM: " <<
		AM.size() << " C: "<< C.size() << endl;
	V = move(v1);
#define f(i) (i = (i >= 0 ? p[i] ? p[i] : i : p[-i] ? -p[-i] : i))
	for (size_t n = 2; n < V.size(); ++n) {
		DBG(assert(p[abs(V[n].h)] && p[abs(V[n].l)] && V[n].v);)
		f(V[n].h), f(V[n].l);
	}
	unordered_map<ite_memo, int_t> c;
	unordered_map<bdds, int_t> am;
	for (pair<ite_memo, int_t> x : C)
		if (	has(S, abs(x.first.x)) &&
			has(S, abs(x.first.y)) &&
			has(S, abs(x.first.z)) &&
			has(S, abs(x.second)))
			f(x.first.x), f(x.first.y), f(x.first.z),
			x.first.rehash(), c.emplace(x.first, f(x.second));
	C = move(c);
	unordered_map<int_t, int_t> q;
	map<bools, unordered_map<int_t, int_t>> mex;
	for (const auto& x : memos_ex) {
		for (pair<int_t, int_t> y : x.second)
			if (has(S, abs(y.first)) && has(S, abs(y.second)))
				q.emplace(f(y.first), f(y.second));
		if (!q.empty()) mex.emplace(x.first, move(q));
	}
	memos_ex = move(mex);
	map<uints, unordered_map<int_t, int_t>> mp;
	for (const auto& x : memos_perm) {
		for (pair<int_t, int_t> y : x.second)
			if (has(S, abs(y.first)) && has(S, abs(y.second)))
				q.emplace(f(y.first), f(y.second));
		if (!q.empty()) mp.emplace(x.first, move(q));
	}
	memos_perm = move(mp);
	map<pair<uints, bools>, unordered_map<int_t, int_t>> mpe;
	for (const auto& x : memos_perm_ex) {
		for (pair<int_t, int_t> y : x.second)
			if (has(S, abs(y.first)) && has(S, abs(y.second)))
				q.emplace(f(y.first), f(y.second));
		if (!q.empty()) mpe.emplace(x.first, move(q));
	}
	memos_perm_ex = move(mpe);
	bool b;
	for (pair<bdds, int_t> x : AM) {
		b = false;
		for (int_t& i : x.first)
			if ((b |= !has(S, abs(i)))) break;
			else f(i);
		if (!b&&has(S,abs(x.second))) am.emplace(x.first, f(x.second));
	}
	AM=move(am), bdd_handle::update(p), Mp.resize(pvars), Mn.resize(nvars);
	p.clear(), S.clear();
	for (size_t n = 0; n < V.size(); ++n)
		if (V[n].v < 0)
			Mn[-V[n].v].emplace(bdd_key(hash_pair(V[n].h, V[n].l),
				V[n].h, V[n].l), n);
		else Mp[V[n].v].emplace(bdd_key(hash_pair(V[n].h, V[n].l),
				V[n].h, V[n].l), n);
	wcerr <<"AM: " << AM.size() << " C: "<< C.size() << endl;
}

void bdd_handle::update(const vector<int_t>& p) {
	std::unordered_map<int_t, std::weak_ptr<bdd_handle>> m;
	for (pair<int_t, std::weak_ptr<bdd_handle>> x : M) {
		DBG(assert(!x.second.expired());)
		spbdd_handle s = x.second.lock();
		f(s->b), m.emplace(f(x.first), x.second);
	}
	M = move(m);
}
#undef f

spbdd_handle bdd_handle::get(int_t b) {
	DBG(assert((size_t)abs(b) < bdd::V.size());)
	auto it = M.find(b);
	if (it != M.end()) return it->second.lock();
	spbdd_handle h(new bdd_handle(b));
	return	M.emplace(b, std::weak_ptr<bdd_handle>(h)), h;
}

void bdd::bdd_sz(int_t x, set<int_t>& s) {
	if (!s.emplace(x).second) return;
	bdd b = get(x);
	bdd_sz(b.h, s), bdd_sz(b.l, s);
}

spbdd_handle operator&&(cr_spbdd_handle x, cr_spbdd_handle y) {
	spbdd_handle r = bdd_handle::get(bdd::bdd_and(x->b, y->b));
	return r;
}

spbdd_handle operator%(cr_spbdd_handle x, cr_spbdd_handle y) {
	return bdd_handle::get(bdd::bdd_and(x->b, -y->b));
}

spbdd_handle operator||(cr_spbdd_handle x, cr_spbdd_handle y) {
	return bdd_handle::get(bdd::bdd_or(x->b, y->b));
}

spbdd_handle bdd_impl(cr_spbdd_handle x, cr_spbdd_handle y) {
	return bdd_handle::get(bdd::bdd_or(-x->b, y->b));
}

spbdd_handle bdd_ite(cr_spbdd_handle x, cr_spbdd_handle y, cr_spbdd_handle z) {
	return bdd_handle::get(bdd::bdd_ite(x->b, y->b, z->b));
}

spbdd_handle bdd_ite_var(uint_t x, cr_spbdd_handle y, cr_spbdd_handle z) {
	return bdd_handle::get(bdd::bdd_ite_var(x, y->b, z->b));
}

spbdd_handle bdd_and_many(bdd_handles v) {
/*	int_t r = T;
	for (auto x : v) r = bdd::bdd_and(r, x->b);
	return bdd_handle::get(r);*/
	if (bdd::V.size() >= 1e+7) bdd::gc();
/*	if (v.size() > 16) {
		bdd_handles x, y;
		spbdd_handle r;
		for (size_t n = 0; n != v.size(); ++n)
			(n < v.size()>>1 ? x : y).push_back(v[n]);
		v.clear();
		r = bdd_and_many(move(x));
		return r && bdd_and_many(move(y));
	}*/
	bdds b;
	b.reserve(v.size());
	for (size_t n = 0; n != v.size(); ++n) b.push_back(v[n]->b);
	am_sort(b);
//	DBG( wcout<<"am begin"<<endl;
	auto r = bdd_handle::get(bdd::bdd_and_many(move(b)));
//	DBG( wcout<<"am end"<<endl;
	return r;
}

spbdd_handle bdd_or_many(const bdd_handles& v) {
	int_t r = F;
	for (auto x : v) r = bdd::bdd_or(r, x->b);
	return bdd_handle::get(r);
	bdds b(v.size());
	for (size_t n = 0; n != v.size(); ++n) b[n] = -v[n]->b;
	return bdd_handle::get(-bdd::bdd_and_many(move(b)));
}

void bdd::sat(uint_t v, uint_t nvars, int_t t, bools& p, vbools& r) {
	if (t == F) return;
	if (!leaf(t) && v < var(t))
		p[v - 1] = true, sat(v + 1, nvars, t, p, r),
		p[v - 1] = false, sat(v + 1, nvars, t, p, r);
	else if (v != nvars) {
		p[v - 1] = true, sat(v + 1, nvars, hi(t), p, r),
		p[v - 1] = false, sat(v + 1, nvars, lo(t), p, r);
	} else	r.push_back(p);
}

vbools bdd::allsat(int_t x, uint_t nvars) {
	bools p(nvars);
	vbools r;
	return sat(1, nvars + 1, x, p, r), r;
}

vbools allsat(cr_spbdd_handle x, uint_t nvars) {
	return bdd::allsat(x->b, nvars);
}

void allsat_cb::sat(int_t x) {
	if (x == F) return;
	const bdd bx = bdd::get(x);
	if (!bdd::leaf(x) && v < bdd::var(x)) {
		DBG(assert(bdd::var(x) <= nvars);)
		p[++v-2] = true, sat(x), p[v-2] = false, sat(x), --v;
	} else if (v != nvars + 1)
		p[++v-2] = true, sat(bx.h),
		p[v-2] = false, sat(bx.l), --v;
	else f(p, x);
}

int_t bdd::bdd_ex(int_t x, const bools& b, unordered_map<int_t, int_t>& memo){
	if (leaf(x)) return x;
	auto it = memo.find(x);
	if (it != memo.end()) return it->second;
	int_t r, y = x;
	while (var(y) - 1 < b.size() && b[var(y) - 1]) {
		r = bdd_or(hi(y), lo(y));
		if (leaf(r)) return memo.emplace(y, r), r;
		y = r;
	}
	if (var(y) - 1 >= b.size()) return y;
	return memo.emplace(y, bdd::add(var(y), bdd_ex(hi(y), b, memo),
				bdd_ex(lo(y), b, memo))).first->second;
}

int_t bdd::bdd_ex(int_t x, const bools& b) {
	return bdd_ex(x, b, memos_ex[b]);
}

spbdd_handle operator/(cr_spbdd_handle x, const bools& b) {
	return bdd_handle::get(bdd::bdd_ex(x->b, b));
}

int_t bdd::bdd_permute(const int_t& x, const uints& m,
		unordered_map<int_t, int_t>& memo) {
	if (leaf(x)) return x;
	auto it = memo.find(x);
	if (it != memo.end()) return it->second;
	return memo.emplace(x, bdd_ite_var(m[var(x)-1],
		bdd_permute(hi(x), m, memo),
		bdd_permute(lo(x), m, memo))).first->second;
}

spbdd_handle operator^(cr_spbdd_handle x, const uints& m) {
	return bdd_handle::get(bdd::bdd_permute(x->b, m, memos_perm[m]));
}

int_t bdd::bdd_permute_ex(int_t x, const bools& b, const uints& m, size_t last,
	unordered_map<int_t, int_t>& memo) {
	if (leaf(x) || var(x) > last + 1) return x;
	auto it = memo.find(x);
	if (it != memo.end()) return it->second;
	int_t t = x, y = x;
	DBG(assert(b.size() >= var(x));)
	for (int_t r; var(y)-1 < b.size() && b[var(y)-1]; y = r)
		if (leaf((r = bdd_or(hi(y), lo(y)))))
			return memo.emplace(t, r), r;
		DBG(else assert(b.size() >= var(r));)
	DBG(assert(!leaf(y) && m.size() >= var(y));)
	return	memo.emplace(t, bdd_ite_var(m[var(y)-1],
		bdd_permute_ex(hi(y), b, m, last, memo),
		bdd_permute_ex(lo(y), b, m, last, memo))).first->second;
}

int_t bdd::bdd_permute_ex(int_t x, const bools& b, const uints& m) {
	size_t last = b.size();
	for (size_t n = 0; n != b.size(); ++n) if (b[n] || (m[n]!=n)) last = n;
	return bdd_permute_ex(x, b, m, last, memos_perm_ex[{m,b}]);
}

spbdd_handle bdd_permute_ex(cr_spbdd_handle x, const bools& b, const uints& m) {
	return bdd_handle::get(bdd::bdd_permute_ex(x->b, b, m));
}

/*spbdd_handle bdd_handle::get(uint_t v, cr_spbdd_handle h, cr_spbdd_handle l) {
	return get(bdd::add(v, h->b, l->b));
}*/

spbdd_handle from_bit(uint_t b, bool v) {
	return bdd_handle::get(bdd::from_bit(b, v));
}

spbdd_handle from_eq(uint_t x, uint_t y) {
	return bdd_ite(from_bit(x,true), from_bit(y,true), from_bit(y,false));
}

bool leaf(cr_spbdd_handle h) { return bdd::leaf(h->b); }
bool trueleaf(cr_spbdd_handle h) { return bdd::trueleaf(h->b); }
wostream& out(wostream& os, cr_spbdd_handle x) { return bdd::out(os, x->b); }

size_t std::hash<ite_memo>::operator()(const ite_memo& m) const {
	return m.hash;
}

size_t std::hash<bdd_key>::operator()(const bdd_key& k) const {return k.hash;}

size_t std::hash<bdds>::operator()(const bdds& b) const {
	size_t r = 0;
	for (int_t i : b) r ^= i;
	return r;
}

bdd::bdd(int_t v, int_t h, int_t l) : h(h), l(l), v(v) {
//	DBG(assert(V.size() < 2 || (v && h && l));)
}

wostream& bdd::out(wostream& os, int_t x) {
	if (leaf(x)) return os << (trueleaf(x) ? L'T' : L'F');
	if (x < 0) x = -x, os << L'~';
	const bdd& b = V[x];
	return out(out(os << b.v << L" ? ", b.h) << L" : ", b.l);
}

/*wostream& operator<<(wostream& os, const bools& x) {
	for (auto y : x) os << (y ? 1 : 0);
	return os;
}

wostream& operator<<(wostream& os, const vbools& x) {
	for (auto y : x) os << y << endl;
	return os;
}

spbdd_handle rand_bdd(int_t n = 5) {
	if (!n) return bdd_ite(
			from_bit(random()%10, random()&1),
			from_bit(random()%10, random()&1),
			from_bit(random()%10, random()&1));
	return bdd_ite(rand_bdd(n-1), rand_bdd(n-1), rand_bdd(n-1));
}

void test_and_many() {
	set<spbdd_handle> s;
	for (size_t k = 0; k != 100; ++k) {
		bdd_handles b;
		for (size_t n = 0; n != 8; ++n) b.push_back(rand_bdd());
		spbdd_handle r = bdd_handle::T;
		for (cr_spbdd_handle i : b) r = r && i;
		assert(r == bdd_and_many(b));
		cout<<k<<endl;
		if (random()&1) s.insert(r);
	}
}*/
